/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fanotify.h>
#include <sys/stat.h>
#include <sys/types.h>

#define CHK(expr, errcode) if((expr)==errcode) perror(#expr), exit(EXIT_FAILURE)

/*
 * See https://stackoverflow.com/questions/1835947
 */

int main(int argc, char **argv)
{
	int r, fd, fan;
	char buf[4096];
	char fdpath[32];
	char path[PATH_MAX + 1];
	char cmdline[PATH_MAX + 1];
	ssize_t buflen, linklen;
	struct fanotify_event_metadata *metadata;

	if (argc > 1) {
		printf("Watching '%s'\n", argv[1]);

		CHK(fan = fanotify_init(FAN_CLASS_NOTIF, O_RDONLY), -1);
		CHK(fanotify_mark(fan, FAN_MARK_ADD | FAN_MARK_DONT_FOLLOW,
					FAN_OPEN,
					AT_FDCWD, argv[1]), -1);
		for (;;) {
			CHK(buflen = read(fan, buf, sizeof(buf)), -1);
			metadata = (struct fanotify_event_metadata*)&buf;

			while (FAN_EVENT_OK(metadata, buflen)) {
				if (metadata->mask & FAN_Q_OVERFLOW) {
					printf("Queue overflow!\n");
					continue;
				}
				sprintf(fdpath, "/proc/self/fd/%d", metadata->fd);
				CHK(linklen = readlink(fdpath, path, sizeof(path) - 1), -1);
				path[linklen] = '\0';
				printf("%s opened by process %d\t", path, (int)metadata->pid);
				close(metadata->fd);

				snprintf(path, PATH_MAX, "/proc/%i/cmdline", (int)metadata->pid);
				fd = open(path, O_RDONLY);
				CHK(r = read(fd, cmdline, PATH_MAX), -1);
				close(fd);
				printf("'%s'\n", cmdline);

				metadata = FAN_EVENT_NEXT(metadata, buflen);
			}
		}
	}

	return 0;
}
