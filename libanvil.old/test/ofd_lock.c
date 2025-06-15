#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#define lock_opts(type) (struct flock){.l_type = type, .l_whence = SEEK_SET, .l_start = 0, .l_len = 2, .l_pid = 0}

int main(int argc, char **argv) {
    const auto fd1 = open("ofd_lock.test", O_RDWR | O_CREAT, 0644);
    if (fd1 < 0) return 1;
    if (write(fd1, "asdf", 4) != 4) return 1;
    const auto fd2 = open("ofd_lock.test", O_RDWR);
    if (fd2 < 0) return 1;


    auto lock = lock_opts(F_WRLCK);
    if (fcntl(fd1, F_OFD_SETLK, &lock)) return 2;

    lock = lock_opts(F_WRLCK);
    if (
        fcntl(fd2, F_OFD_SETLK, &lock) == 0 ||
        (errno != EACCES && errno != EAGAIN)
    ) return 3;

    lock = lock_opts(F_UNLCK);
    if (fcntl(fd1, F_OFD_SETLK, &lock)) return 4;

    lock = lock_opts(F_RDLCK);
    if (fcntl(fd1, F_OFD_SETLK, &lock)) return 5;

    lock = lock_opts(F_RDLCK);
    if (fcntl(fd2, F_OFD_SETLK, &lock)) return 6;

    lock = lock_opts(F_WRLCK);
    if (
        fcntl(fd1, F_OFD_SETLK, &lock) == 0 ||
        (errno != EACCES && errno != EAGAIN)
    ) return 7;

    lock = lock_opts(F_UNLCK);
    if (fcntl(fd1, F_OFD_SETLK, &lock)) return 8;

    lock = lock_opts(F_UNLCK);
    if (fcntl(fd2, F_OFD_SETLK, &lock)) return 9;

    return 0;
}
