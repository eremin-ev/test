/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

struct File final {
    explicit File(const char *path);
    ~File();

    int write(const char *line);
private:
    int m_fd;
};

File::File(const char *path)
    : m_fd{-1}
{
    m_fd = open(path, O_CREAT | O_WRONLY, 0644);
    if (m_fd == -1) {
        printf("cannot open '%s' O_WRONLY: %s\n", path, strerror(errno));
    }
    printf("%s path %s fd %d\n", __func__, path, m_fd);
}

int File::write(const char *line)
{
    if (m_fd == -1) {
        return 0;
    }

    int len = strlen(line);
    int r = ::write(m_fd, line, len);
    if (r == -1) {
        printf("cannot write '%s' to %d: %s\n", line, m_fd, strerror(errno));
    }

    return r;
}

File::~File()
{
    if (m_fd == -1) {
        return;
    }

    printf("%s closing %d\n", __func__, m_fd);
    int r = close(m_fd);
    if (r == -1) {
        printf("cannot close %d: %s\n", m_fd, strerror(errno));
    }
}

static int test_path(const char *path)
{
    int fd = open(path, O_CREAT | O_WRONLY);
    if (fd == -1) {
        printf("cannot open '%s' O_WRONLY: %s\n", path, strerror(errno));
        return 0;
    }

    const char s[] = "attempts=5\n";
    int len = sizeof(s) - 1;

    printf("%s Ok, press Return before write\n", __func__);
    char buffer[10];
    int r = read(STDIN_FILENO, buffer, 1);

    printf("len %d\n", len);

    r = write(fd, s, len);
    if (r == -1) {
        printf("cannot write '%s' to %s: %s\n", s, path, strerror(errno));
        close(fd);
        return 0;
    }

    printf("OK wrote %i bytes '%s', checking ...\n", r, s);

    printf("%s Ok, press Return before close\n", __func__);
    r = read(STDIN_FILENO, buffer, 1);

    close(fd);

    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s <path>\n", argv[0]);
        return 0;
    }

    if (0) {
        printf("testing %s...\n", argv[1]);
        test_path(argv[1]);
    }

    File file{argv[1]};
    file.write("a test line\n");
    file.write("the second line\n");
}
