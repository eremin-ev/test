/*
 * Test self/comm overwriting.
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <linux/capability.h>

#define BUF_SIZE 4096

#define ARRAY_SIZE(a)	(sizeof(a)/sizeof(a[0]))

void *thread_func(void *notused)
{
	const char *s = "ReplaceNameLong";
	char path[BUF_SIZE];
	char buf[BUF_SIZE];
	int len;
	int fd;
	int r;

	printf("%s notused %p\n", s, notused);

	memset(path, 0, BUF_SIZE);

	len = snprintf(path, BUF_SIZE, "/proc/self/comm");

	fd = open(path, O_RDWR);
	if (fd != -1) {
		printf("OK fd %i, strlen %i\n", fd, len);

		r = write(fd, s, len);
		if (r > 0 && r == len) {
			printf("OK wrote %i bytes '%s', checking ...\n",
				r, s);

			r = read(fd, buf, BUF_SIZE);
			if (r > 0) {
				printf("OK read %i bytes\n", r);
				/* remove trailing '\n' pasted by the kernel */
				if (r == len + 1 && buf[len] == '\n') {
					r--;
					buf[len] = '\0';
					printf("OK remove trailing newline\n");
				}

				if (0 == strcmp(s, buf)) {
					printf("OK %i bytes match: '%s'\n",
						r, buf);
				}
			} else if (r == -1) {
				printf("cannot read '%s': %s\n",
					path, strerror(errno));
			}
		} else if (r == -1) {
			printf("cannot write '%s': %s\n", path, strerror(errno));
		}

		close(fd);
	} else {
		printf("cannot open '%s' O_RDWR: %s\n", path, strerror(errno));
	}

	return NULL;
}

int main(void)
{
	const int caps[] = {
		CAP_CHOWN,
		CAP_DAC_OVERRIDE,
		//CAP_FOWNER,
		CAP_FSETID,
		CAP_KILL,
		CAP_SETPCAP,
		CAP_LINUX_IMMUTABLE,
		CAP_NET_BIND_SERVICE,
		CAP_NET_BROADCAST,
		CAP_NET_ADMIN,
		CAP_NET_RAW,
		CAP_IPC_LOCK,
		CAP_IPC_OWNER,
		CAP_SYS_MODULE,
		CAP_SYS_RAWIO,
		CAP_SYS_CHROOT,
		CAP_SYS_PTRACE,
		CAP_SYS_PACCT,
		CAP_SYS_ADMIN,
		CAP_SYS_BOOT,
		CAP_SYS_NICE,
		CAP_SYS_RESOURCE,
		CAP_SYS_TIME,
		CAP_SYS_TTY_CONFIG,
		CAP_MKNOD,
		CAP_LEASE,
		CAP_AUDIT_WRITE,
		CAP_AUDIT_CONTROL,
		CAP_SETFCAP,
		CAP_MAC_OVERRIDE,
		CAP_MAC_ADMIN,
		CAP_SYSLOG,
		CAP_WAKE_ALARM,
	};

	int r1, r2, r3;
	size_t i;

	printf("dropping caps:");
	for (i = 0; i < ARRAY_SIZE(caps); i++) {
		r1 = prctl(PR_CAPBSET_READ, caps[i]);
		r2 = prctl(PR_CAPBSET_DROP, caps[i]);
		r3 = prctl(PR_CAPBSET_READ, caps[i]);
		printf("cap %i before drop %i, drop %i, after drop %i\n",
			caps[i], r1, r2, r3);
		if (r2 == -1) {
			printf("cannot drop: %s\n", strerror(errno));
		}
	}

	r1 = setuid(777);
	r1 = setuid(1001);
	r1 = setuid(778);
	r1 = setuid(779);
	r1 = setuid(780);

	pthread_t thread_id;
	void *res;

	pthread_create(&thread_id, NULL, thread_func, NULL);

	r1 = pthread_join(thread_id, &res);
	if (r1) {
		printf("pthread_join: %s\n", strerror(errno));
	} else {
		printf("joined OK, return value is %p\n", res);
	}

	return 0;
}
