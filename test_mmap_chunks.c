/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <openssl/sha.h>
#include <openssl/err.h>

struct m_buf {
	void *m_buf;
	size_t m_size;
};

struct m_desc {
	struct m_buf *mb;
	int mb_nr;
};

static int hash_to_str_r(uint8_t *in_buf, int in_size, char *o_str, int o_size)
{
	static char hexd[] = "0123456789abcdef";
	int i, b;
	char *o = o_str;

#if 1
	if (o_size <= 2*in_size) {
		return 0;
	}

	for (i = 0; i < in_size; i++) {
		b = in_buf[i];
		*o++ = hexd[0xf & (b >> 4)];
		*o++ = hexd[0xf & b];
	}

	*o++ = '\0';
#else
	for (i = 0; i < h_size && o - h_str < h_len - 1; i++) {
		b = h_bin[i];
		*o++ = hexd[0xf & (b >> 4)];
		*o++ = hexd[0xf & b];
	}
#endif
	return o - o_str;
}

int calc_digest1(const struct m_desc *md)
{
	int i, r;
	SHA256_CTX sha256;
	uint8_t digest[SHA256_DIGEST_LENGTH];
	char digest_str[2*SHA256_DIGEST_LENGTH + 1] = "";
	long e;
	const int e_size = 4096;
	char e_buf[e_size];

	if (!md) {
		printf("%s empty md\n", __func__);
		return 0;
	}

	r = SHA256_Init(&sha256);
	if (!r) {
		e = ERR_get_error();
		ERR_error_string_n(e, e_buf, e_size);
		printf("%s SHA256_Init error: %s\n", __func__, e_buf);
		return 0;
	}

	for (i = 0; i < md->mb_nr; i++) {
		printf("%s updating %i %p size %zu\n", __func__, i,
			md->mb[i].m_buf, md->mb[i].m_size);
		r = SHA256_Update(&sha256, md->mb[i].m_buf, md->mb[i].m_size);
		if (!r) {
			e = ERR_get_error();
			ERR_error_string_n(e, e_buf, e_size);
			printf("%s SHA256_Update(%zi) error: %s\n", __func__, md->mb[i].m_size, e_buf);
			return 0;
		}
	}

	r = SHA256_Final(digest, &sha256);
	if (!r) {
		e = ERR_get_error();
		ERR_error_string_n(e, e_buf, e_size);
		printf("%s SHA256_Final error: %s\n", __func__, e_buf);
		return 0;
	}

	r = hash_to_str_r(digest, SHA256_DIGEST_LENGTH,
			  digest_str, 2*SHA256_DIGEST_LENGTH + 1);

	printf("%s Ok r %i, digest '%s'\n", __func__, r, digest_str);

	return 1;
}

int mmap_path(struct m_desc *md, const char *path, size_t chunk_max, unsigned flags)
{
	int i, r, fd;
	struct stat st;
	int chunk_nr, chunk_last;
	size_t chunk_last_size;

	printf("%s flags %x\n", __func__, flags);

	r = stat(path, &st);
	if (r == -1) {
		printf("%s cannot stat '%s': %s\n", __func__,
		       path, strerror(errno));
		return 0;
	}

	if (!st.st_size) {
		fprintf(stderr, "%s: '%s' is empty\n", __func__, path);
		return 0;
	}

	chunk_nr = st.st_size / chunk_max;
	chunk_last_size = st.st_size % chunk_max;
	chunk_last = chunk_last_size != 0;

	printf("%s size %" PRIu64 " = %i x %zu + %zu\n", __func__,
		st.st_size, chunk_nr, chunk_max, chunk_last_size);

	if (!chunk_nr) {
		md->mb = NULL;
		md->mb_nr = 0;
		return 0;
	}

	md->mb = malloc((chunk_nr + chunk_last)* sizeof(*md->mb));
	if (!md->mb) {
		printf("%s: cannot allocate md->mb: %s\n", __func__,
			strerror(errno));
		return 0;
	}

	memset(md->mb, 0, (chunk_nr + chunk_last) * sizeof(*md->mb));

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		printf("%s: cannot open '%s': %s\n", __func__,
			path, strerror(errno));
		return 0;
	}

	for (i = 0; i < chunk_nr; i++) {
		md->mb[i].m_buf = mmap(NULL, chunk_max, PROT_READ,
				       MAP_SHARED, fd, (unsigned long long)i*chunk_max);

		if (md->mb[i].m_buf == MAP_FAILED) {
			printf("%s cannot mmap '%s' offset %zu: %s\n", __func__,
				path, i*chunk_max, strerror(errno));
			sleep(100);
			/* cleanup later */
			break;
		}

		md->mb[i].m_size = chunk_max;
	}

	/* cleanup in case of error */
	if (i != chunk_nr) {
		printf("%s some chunks were not mapped, cleaning up\n", __func__);
		for (i = 0; i < md->mb_nr; i++) {
			if (md->mb[i].m_size) {
				r = munmap(md->mb[i].m_buf, md->mb[i].m_size);
				if (r != 0) {
					printf("%s cannot munmap '%s' %s\n", __func__,
						path, strerror(errno));
				}
			}
		}

		close(fd);
		free(md->mb);
		md->mb = NULL;
		md->mb_nr = 0;
		return 0;
	}

	/* handle the last chunk if any, its size will be less than chunk_max */
	if (chunk_last) {
		md->mb[chunk_nr].m_buf = mmap(NULL, chunk_last_size,
					      PROT_READ, MAP_SHARED,
					      fd, chunk_nr*chunk_max);

		if (md->mb[i].m_buf == MAP_FAILED) {
			printf("%s cannot mmap '%s' offset %zu: %s\n", __func__,
				path, i*chunk_max, strerror(errno));
			/* cleanup */
			close(fd);
			free(md->mb);
			md->mb = NULL;
			md->mb_nr = 0;
			return 0;
		}

		md->mb[chunk_nr].m_size = chunk_last_size;
	}
	md->mb_nr = chunk_nr + chunk_last;

	close(fd);

	return md->mb_nr;
}

int main(int argc, char **argv)
{
	struct m_desc md;
	unsigned flags = 0;

	int r, i = 1;
	while (i < argc) {
		printf("argv[%i] '%s'\n", i, argv[i]);
		r = mmap_path(&md, argv[i], 512*1024*1024, flags);
		if (r) {
			calc_digest1(&md);
		}
		i++;
	}

	return 0;
}
