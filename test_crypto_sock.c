/*
 * A simple kernel crypto API test.
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_alg.h>

#define		SECT_SIZE	4096

static double l_time(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + 1e-9*ts.tv_nsec;
}

static int test_decrypt(int fd2,
			const char *iv, int iv_len,
			const char *in, int in_len,
			char *out, int out_len)
{
	struct iovec iov = {
		.iov_base = (void*)in,
		.iov_len = in_len,
	};

	uint32_t *type;
	struct af_alg_iv *alg_iv;
	int iv_msg_size = CMSG_SPACE(sizeof(*alg_iv) + iv_len);

	char buffer[CMSG_SPACE(sizeof(*type)) + iv_msg_size];
	memset(buffer, 0, sizeof(buffer));

	struct msghdr msg = {
		.msg_control = buffer,
		.msg_controllen = sizeof(buffer),
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};

	struct cmsghdr *header = CMSG_FIRSTHDR(&msg);
	if (!header)
		return -EINVAL;

	header->cmsg_level = SOL_ALG;
	header->cmsg_type = ALG_SET_OP;
	header->cmsg_len = CMSG_LEN(sizeof(*type));

	type = (void *)CMSG_DATA(header);
	*type = ALG_OP_DECRYPT;

	header = CMSG_NXTHDR(&msg, header);
	if (!header)
		return -EINVAL;

	header->cmsg_level = SOL_ALG;
	header->cmsg_type = ALG_SET_IV;
	header->cmsg_len = iv_msg_size;

	alg_iv = (void *)CMSG_DATA(header);
	alg_iv->ivlen = iv_len;
	memcpy(alg_iv->iv, iv, iv_len);

	int r = sendmsg(fd2, &msg, 0);
	if (r != (ssize_t)in_len) {
		printf("cannot sendmsg: %s\n", strerror(errno));
		return -EIO;
	}

	r = read(fd2, out, out_len);
	if (r != (ssize_t)out_len) {
		printf("cannot read: %s\n", strerror(errno));
		return -EIO;
	}

	return 0;
}

static int get_alignment(void *p)
{
	size_t v = (size_t) p;
	int i, mask = 0;
	for (i = 0; i < 32; i++) {
		mask |= 1 << i;
		if ((v & mask) != 0)
			break;
		//printf("i %i, mask 0x%x\n", i, mask);
	}

	return i;
}

int main(void)
{
	int fd = socket(AF_ALG, SOCK_SEQPACKET, 0);
	if (fd < 0) {
		printf("cannot socket fd %i: %s\n", fd, strerror(errno));
		return ENOMEM;
	}

	const char *mode = "xts";
	const char *name = "aes";
	struct sockaddr_alg sa = {
		.salg_family = AF_ALG,
		.salg_type = "skcipher",
	};
	snprintf((char *)sa.salg_name, sizeof(sa.salg_name), "%s(%s)", mode, name);

	//printf("%s salg_name %s\n", __func__, sa.salg_name);

	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		printf("cannot bind: %s\n", strerror(errno));
		return ENOENT;
	}

	const int key_length = 64;
	const char key[] = {
		0x32, 0x9d, 0xc4, 0x67, 0x0c, 0x15, 0x31, 0x82,
		0x6c, 0x54, 0x3d, 0xe7, 0xde, 0x57, 0xd1, 0x61,
		0x24, 0x5e, 0x02, 0xd5, 0x23, 0x3d, 0xe0, 0x04,
		0xe7, 0x9b, 0x48, 0xbd, 0xbd, 0x1e, 0xd6, 0x40,
		0x26, 0x46, 0xbc, 0x7b, 0xf2, 0x2c, 0x79, 0xce,
		0xa4, 0xb6, 0xc5, 0x60, 0xd0, 0xde, 0xed, 0x08,
		0x46, 0xeb, 0x42, 0xac, 0xea, 0xb1, 0xe6, 0x07,
		0x6b, 0x5e, 0xfd, 0x49, 0x36, 0x49, 0x8f, 0xa6,
	};

	if (setsockopt(fd, SOL_ALG, ALG_SET_KEY, key, key_length) < 0) {
		printf("cannot setsockopt: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	int fd2 = accept(fd, NULL, 0);
	if (fd2 < 0) {
		printf("cannot accept: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	char buf[SECT_SIZE];
	memset(buf, 0xff, SECT_SIZE);

	int size = 3*SECT_SIZE;
	int np = size / SECT_SIZE;
	int cont_len = np*SECT_SIZE;
	char *cont_buf = mmap(NULL, cont_len, PROT_READ | PROT_WRITE,
			      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (!cont_buf) {
		printf("cannot malloc: %s\n", strerror(errno));
		return 0;
	}

	int align = get_alignment(cont_buf);

	printf("size %i, SECT_SIZE %i, np %i, p %p, align %i\n",
		size, SECT_SIZE, np, cont_buf, align);

	for (int k = 0; k < cont_len / SECT_SIZE; k++) {
		memcpy(cont_buf + k*SECT_SIZE, buf, SECT_SIZE);
	}

	const int iv_len = 16;
	const char iv[] = {
		0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};
	const int exp_len = 8;
	const char exp_buf[] = {
#if SECT_SIZE == 256
		0x76, 0x5e, 0xdc, 0x40, 0x59, 0x66, 0x51, 0x56,
#elif SECT_SIZE == 512
		0x9f, 0xe4, 0xb1, 0xd5, 0x53, 0xa3, 0x35, 0x07,
#elif SECT_SIZE == 1024
		0x53, 0x74, 0x81, 0x82, 0xe8, 0xc0, 0xdb, 0xa8,
#elif SECT_SIZE == 2048
		0x38, 0x56, 0xbe, 0x19, 0xe9, 0x29, 0xd6, 0xb1,
#elif SECT_SIZE == 4096
		0x25, 0x0f, 0x26, 0x09, 0x91, 0xbc, 0x69, 0x1a,
#endif
	};

	for (int k = 0, k_prev = k; k < 2*SECT_SIZE; k++) {
		memcpy(cont_buf + k, buf, SECT_SIZE);

		double t = l_time();
		test_decrypt(fd2, iv, iv_len, cont_buf + k, SECT_SIZE, cont_buf + k, SECT_SIZE);
		t = l_time() - t;

		int r = memcmp((cont_buf + k) + (SECT_SIZE - exp_len), exp_buf, exp_len);
		if (r) {
			printf("Hit r %i, k %i, t %g\t", r, k, t);
			int i;
			//for (i = 0; i < SECT_SIZE; i++)
			for (i = SECT_SIZE - 32; i < SECT_SIZE; i++) {
				printf("%02x ", 0xff & cont_buf[k + i]);
			}
			printf("\n");
		} else {
			printf("OK  error %i, offset %i, delta %i, t %g\n",
				r, k, k - k_prev, t);
			k_prev = k;
		}
	}

	munmap(cont_buf, cont_len);

	return EXIT_SUCCESS;
}
