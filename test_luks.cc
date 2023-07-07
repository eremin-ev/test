/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * See cryptsetup/src/cryptsetup.c
 */

#include <libcryptsetup.h>
#include <libdevmapper.h>

#include <errno.h>
#include <inttypes.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/loop.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>     /* for major, minor */
#include <unistd.h>

namespace {

const int LOOP_DEV_MAJOR = 7;

static char *ioctl_backing_file(const char *loop)
{
	struct loop_info64 lo64 = {};
	int loop_fd;

	loop_fd = open(loop, O_RDONLY);
	if (loop_fd < 0)
		return NULL;

	if (ioctl(loop_fd, LOOP_GET_STATUS64, &lo64) < 0) {
		close(loop_fd);
		return NULL;
	}

	lo64.lo_file_name[LO_NAME_SIZE-2] = '*';
	lo64.lo_file_name[LO_NAME_SIZE-1] = 0;

	close(loop_fd);

	return strdup((char*)lo64.lo_file_name);
}

static char *sysfs_backing_file(const char *loop)
{
    struct stat st;
    char buf[PATH_MAX];
    size_t len;
    int fd;

    if (stat(loop, &st) || !S_ISBLK(st.st_mode))
        return NULL;

    snprintf(buf, sizeof(buf), "/sys/dev/block/%d:%d/loop/backing_file",
             major(st.st_rdev), minor(st.st_rdev));

    fd = open(buf, O_RDONLY);
    if (fd < 0)
        return NULL;

    len = read(fd, buf, PATH_MAX);
    close(fd);
    if (len < 2)
        return NULL;

    buf[len - 1] = '\0';
    return strdup(buf);
}

static int crypt_loop_device(const char *loop)
{
    if (!loop) {
        return 0;
    }

    struct stat st;
    int r = stat(loop, &st);
    if (r < 0) {
        return 0;
    }

    if (!S_ISBLK(st.st_mode) || major(st.st_rdev) != LOOP_DEV_MAJOR) {
        return 0;
    }

    return 1;
}

static char *crypt_loop_backing_file(const char *devname)
{
    if (!crypt_loop_device(devname))
        return NULL;

    char *bf = sysfs_backing_file(devname);
    return bf ?: ioctl_backing_file(devname);
}

static int luks_status(const char *devname)
{
    crypt_reencrypt_info ri;
    struct crypt_active_device cad;
    struct crypt_params_integrity ip = {};
    struct crypt_device *cd = NULL;
    const char *device;
    int path = 0, r = 0;

    /* perhaps a path, not a dm device name */
    if (strchr(devname, '/'))
        path = 1;

    crypt_status_info ci = crypt_status(NULL, devname);
    switch (ci) {
    case CRYPT_INVALID:
        r = -EINVAL;
        break;
    case CRYPT_INACTIVE:
        if (path)
            printf("%s is inactive\n", devname);
        else
            printf("%s/%s is inactive\n", crypt_get_dir(), devname);
        r = -ENODEV;
        break;
    case CRYPT_ACTIVE:
    case CRYPT_BUSY:
        if (path)
            printf("%s is active%s\n", devname,
                    ci == CRYPT_BUSY ? " and is in use" : "");
        else
            printf("%s/%s is active%s\n", crypt_get_dir(), devname,
                    ci == CRYPT_BUSY ? " and is in use" : "");

        r = crypt_init_by_name_and_header(&cd, devname, NULL);
        if (r < 0)
            goto out;

        printf("  type:    %s\n", crypt_get_type(cd) ?: "n/a");

        /* Print only CRYPT type devices */
        if (!crypt_get_cipher(cd))
            goto out;

        ri = crypt_reencrypt_status(cd, NULL);
        if (ri > CRYPT_REENCRYPT_NONE && ri < CRYPT_REENCRYPT_INVALID)
            printf("  reencryption:  in-progress\n");

        r = crypt_get_active_device(cd, devname, &cad);
        if (r < 0)
            goto out;

        r = crypt_get_integrity_info(cd, &ip);
        if (r < 0 && r != -ENOTSUP)
            goto out;

        printf("  cipher:  %s-%s\n", crypt_get_cipher(cd),
                crypt_get_cipher_mode(cd));
        printf("  keysize: %d bits\n", crypt_get_volume_key_size(cd) * 8);
        printf("  key location: %s\n",
                (cad.flags & CRYPT_ACTIVATE_KEYRING_KEY) ? "keyring" : "dm-crypt");

        if (ip.integrity)
            printf("  integrity: %s\n", ip.integrity);

        if (ip.integrity_key_size)
            printf("  integrity keysize: %d bits\n", ip.integrity_key_size * 8);

        device = crypt_get_device_name(cd);
        printf("  device:  %s\n", device);

        char *backing_file = crypt_loop_backing_file(device);
        if (backing_file) {
            printf("  loop:    %s\n", backing_file);
            free(backing_file);
        }

        printf("  sector size:  %d\n", crypt_get_sector_size(cd));
        printf("  offset:  %" PRIu64 " sectors\n", cad.offset);
        printf("  size:    %" PRIu64 " sectors\n", cad.size);

        if (cad.iv_offset)
            printf("  skipped: %" PRIu64 " sectors\n", cad.iv_offset);

        printf("  mode:    %s%s\n",
                cad.flags & CRYPT_ACTIVATE_READONLY ? "readonly" : "read/write",
                (cad.flags & CRYPT_ACTIVATE_SUSPENDED) ? " (suspended)" : "");

        if (cad.flags &
                (CRYPT_ACTIVATE_ALLOW_DISCARDS | CRYPT_ACTIVATE_SAME_CPU_CRYPT |
                 CRYPT_ACTIVATE_SUBMIT_FROM_CRYPT_CPUS |
                 CRYPT_ACTIVATE_NO_READ_WORKQUEUE | CRYPT_ACTIVATE_NO_WRITE_WORKQUEUE))
            printf(
                    "  flags:   %s%s%s%s%s\n",
                    (cad.flags & CRYPT_ACTIVATE_ALLOW_DISCARDS) ? "discards " : "",
                    (cad.flags & CRYPT_ACTIVATE_SAME_CPU_CRYPT) ? "same_cpu_crypt " : "",
                    (cad.flags & CRYPT_ACTIVATE_SUBMIT_FROM_CRYPT_CPUS)
                    ? "submit_from_crypt_cpus "
                    : "",
                    (cad.flags & CRYPT_ACTIVATE_NO_READ_WORKQUEUE) ? "no_read_workqueue "
                    : "",
                    (cad.flags & CRYPT_ACTIVATE_NO_WRITE_WORKQUEUE) ? "no_write_workqueue"
                    : "");
    }

out:
    crypt_free(cd);

    if (r == -ENOTSUP)
        r = 0;

    return r;
}

static bool luks_is_active(const char *devname)
{
    crypt_status_info ci;
    bool r = false;

    ci = crypt_status(NULL, devname);
    switch (ci) {
    case CRYPT_INVALID:
        printf("%s is invalid\n", devname);
        break;
    case CRYPT_INACTIVE:
        printf("%s is inactive\n", devname);
        break;
    case CRYPT_ACTIVE:
    case CRYPT_BUSY:
        printf("%s is active%s\n", devname,
                ci == CRYPT_BUSY ? " and is in use" : "");
        r = true;
    }

    return r;
}

static bool get_uuid(const char *devname, char *dst, int dst_max)
{
    struct crypt_device *cd = NULL;

    int r = crypt_init(&cd, devname);
    if (r != 0)
        return false;

    r = crypt_load(cd, CRYPT_LUKS1, NULL);
    if (r != 0) {
        crypt_free(cd);
        return false;
    }

    const char *uuid = crypt_get_uuid(cd);
    if (uuid) {
        memcpy(dst, uuid, dst_max);
    }

    crypt_free(cd);

    return true;
}

static void test_show_uuid(const char *devname)
{
    /*
     * Max uuid lenght is 32 hex digits + 4 dashes + '\0'.  See [1, 2]
     *
     * [1] RFC 4122 A Universally Unique IDentifier (UUID) URN Namespace
     *     https://www.rfc-editor.org/rfc/rfc4122#section-3
     * [2] UUID max character length
     *     https://stackoverflow.com/questions/13397038/uuid-max-character-length
     */
    const int max_uuid = 32 + 4 + 1;
    char uuid[max_uuid];
    bool success = get_uuid(devname, uuid, max_uuid);
    if (!success) {
        printf("Couldn't get uuid of '%s'\n", devname);
        return;
    }

    printf("%s: uuid-%s\n", devname, uuid);

    /*
     * max_uuid + 'luks' + '-' (dash)
     */
    const int max_dm_name = max_uuid + 5;
    char dm_name[max_dm_name];
    snprintf(dm_name, max_dm_name, "luks-%s", uuid);

    luks_status(dm_name);
    luks_is_active(dm_name);

    printf("\n");
}

enum class LuksResult {
    NotFound = 1,
    Valid,
    Invalid,
};

static struct crypt_device *get_crypt_device(const char *path)
{
    struct crypt_device *cd = nullptr;

    int err = crypt_init(&cd, path);
    if (err < 0) {
        return nullptr;
    }

    err = crypt_load(cd, NULL, NULL);
    if (err < 0) {
        crypt_free(cd);
        return nullptr;
    }

    return cd;
}

static struct crypt_device *get_dm_name(const char *dev_name, char *dm_name, int dm_len)
{
    struct crypt_device *cd = get_crypt_device(dev_name);
    if (!cd) {
        printf("%s Could not init crypt device %s\n", __func__, dev_name);
        return nullptr;
    }

    const char *uuid = crypt_get_uuid(cd);
    if (!uuid) {
        crypt_free(cd);
        return nullptr;
    }

    snprintf(dm_name, dm_len, "luks-%s", uuid);

    return cd;
}

static LuksResult luks_unlock(struct crypt_device *cd,
                              int slot,
                              const char *dev_name,
                              const char *pass_str,
                              int pass_len,
                              bool discard)
{
    if (!pass_str || *pass_str == '\0' || !pass_len) {
        printf("%s No passphrase\n", __func__);
        return LuksResult::Invalid;
    }

    crypt_keyslot_info status = crypt_keyslot_status(cd, slot);
    if (status != CRYPT_SLOT_ACTIVE && status != CRYPT_SLOT_ACTIVE_LAST) {
        printf("%s keyslot %d is not active\n", __func__, slot);
        return LuksResult::NotFound;
    }

    unsigned flags = 0;

    if (discard) {
        flags |= CRYPT_ACTIVATE_ALLOW_DISCARDS;
    }

    int r = crypt_activate_by_passphrase(cd, dev_name, slot, pass_str, pass_len, flags);

    return r >= 0 ? LuksResult::Valid
                  : LuksResult::Invalid;
}

static bool test_unlock(const char *path, bool discard, bool unlock)
{
    printf("%s path %s\n", __func__, path);

    constexpr int dm_len = BUFSIZ;
    char dm_name[dm_len];
    struct crypt_device *cd = get_dm_name(path, dm_name, dm_len);
    if (!cd) {
        printf("%s Couldn't get uuid of %s\n", __func__, path);
        return false;
    }

    int slot = 0;
    const char pass_str[] = "123";
    constexpr int pass_len = sizeof(pass_str) - 1;

    const char *target_name = nullptr;
    if (unlock) {
        target_name = dm_name;
    }

    LuksResult r = luks_unlock(cd, slot, target_name, pass_str, pass_len, discard);

    crypt_free(cd);

    printf("%s '%s' uuid: %s, unlock result %i %s\n", __func__, path, dm_name,
           static_cast<int>(r), r == LuksResult::Valid ? "Ok" : "failed");

    return true;
}

static int get_luks_type_id(const char *luks_type)
{
    // compare with CRYPT_LUKS1/CRYPT_LUKS2
    if (luks_type[0] == 'L' &&
        luks_type[1] == 'U' &&
        luks_type[2] == 'K' &&
        luks_type[3] == 'S') {
        if (luks_type[4] == '1' && luks_type[5] == '\0') {
            return 1;
        } else if (luks_type[4] == '2' && luks_type[5] == '\0') {
            return 2;
        }
    }

    return 0;
}

static int get_field_nr(const char *src, int nr, char c, char *dst)
{
    int idx = 0;
    const char *s = src;
    while (*s && idx < nr) {
        if (*s == c) {
            ++idx;
        }
        s++;
    }

    printf("%s '%s'\n", __func__, s);

    if (!*s) {
        return 0;
    }

    const char *dst0 = dst;
    while (*s && *s != c) {
        *dst++ = *s++;
    }

    return dst - dst0;
}

static int get_volume_key(const char *dm_name, char *volume_key)
{
    struct dm_task *dmt = dm_task_create(DM_DEVICE_TABLE);
    if (!dm_task_set_name(dmt, dm_name)) {
        printf("%s Could not find %s\n", __func__, dm_name);
        dm_task_destroy(dmt);
        return 0;
    }

    dm_task_run(dmt);

    uint64_t start, length;
    char *target_type;
    char *params;
    dm_get_next_target(dmt, NULL, &start, &length, &target_type, &params);
    int volume_len = get_field_nr(params, 1, ' ', volume_key);

    printf("%s dm_type %p:'%s', key %p:'%s'\n", __func__,
           target_type, target_type, params, params);

    dm_task_destroy(dmt);

    return volume_len;
}

static int ascii_char_to_hex(char c)
{
    switch (c) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'A':
    case 'a': return 0xa;
    case 'B':
    case 'b': return 0xb;
    case 'C':
    case 'c': return 0xc;
    case 'D':
    case 'd': return 0xd;
    case 'E':
    case 'e': return 0xe;
    case 'F':
    case 'f': return 0xf;
    }

    return 0;
}

static int ascii_str_to_hex(const char *str, char *hex)
{
    const char *s = str;
    char *o = hex;
    bool hi = true;
    while (*s) {
        if (hi) {
            *o = ascii_char_to_hex(*s) << 4;
            hi = !hi;
        } else {
            *o |= ascii_char_to_hex(*s);
            hi = !hi;
            o++;
        }

        s++;
    }
    return o - hex;
}

static void dump_hex(const char *hex)
{
    const char *s = hex;
    printf("%s: ", __func__);
    while (*s) {
        printf("%x", *s);
        s++;
    }
    printf("\n");
}

static void test_volume_key(const char *path)
{
    constexpr int dm_len = BUFSIZ;
    char dm_name[dm_len];
    struct crypt_device *cd = get_dm_name(path, dm_name, dm_len);
    if (!cd) {
        printf("%s Couldn't get uuid of %s\n", __func__, path);
        return;
    }

    const char *luks_type = crypt_get_type(cd);
    int luks_type_id = get_luks_type_id(luks_type);

    printf("%s luks_type %s (%d)\n", __func__, luks_type, luks_type_id);

    char volume_key[BUFSIZ];
    int volume_len = get_volume_key(dm_name, volume_key);
    char hex_key[volume_len];
    int hex_len = ascii_str_to_hex(volume_key, hex_key);

    dump_hex(hex_key);

    printf("%s volume_key '%s:%d', hex_len %d\n", __func__, volume_key, volume_len, hex_len);

    int r = -1;
    if (volume_len) {
        r = crypt_volume_key_verify(cd, hex_key, hex_len);
    }

    if (r < 0) {
        printf("%s Cannot obtain luks volume key\n", __func__);
    } else {
        printf("%s key verified ok\n", __func__);
    }

    crypt_free(cd);
}

} // anonymous namespace

int main(int argc, const char **argv)
{
    for (int i = 1; i < argc; ++i) {
        if (0) {
            test_show_uuid(argv[i]);
        }

        if (0 && !test_unlock(argv[i], false, false)) {
            break;
        }

        test_volume_key(argv[i]);
    }

    return EXIT_SUCCESS;
}
