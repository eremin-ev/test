/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * See cryptsetup/src/cryptsetup.c
 */

#include <libcryptsetup.h>

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
    struct stat st;

    if (!loop)
        return 0;

    if (stat(loop, &st) || !S_ISBLK(st.st_mode) ||
            major(st.st_rdev) != LOOP_DEV_MAJOR)
        return 0;

    return 1;
}

static char *crypt_loop_backing_file(const char *devname)
{
    char *bf;

    if (!crypt_loop_device(devname))
        return NULL;

    bf = sysfs_backing_file(devname);
    return bf ?: ioctl_backing_file(devname);
}

static int luks_status(const char *devname)
{
    crypt_status_info ci;
    crypt_reencrypt_info ri;
    struct crypt_active_device cad;
    struct crypt_params_integrity ip = {};
    struct crypt_device *cd = NULL;
    const char *device;
    int path = 0, r = 0;

    /* perhaps a path, not a dm device name */
    if (strchr(devname, '/'))
        path = 1;

    ci = crypt_status(NULL, devname);
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

static void test1(const char *devname)
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
        printf("Cannot get uuid of '%s'\n", devname);
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

} // namespace

int main(int argc, const char **argv)
{
    for (int i = 1; i < argc; ++i) {
        test1(argv[i]);
    }

    return EXIT_SUCCESS;
}
