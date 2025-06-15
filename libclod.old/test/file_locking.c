

#ifdef CLOD_USE_POSIX

#include <errno.h>
#include <math.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/file.h>
#include <asm-generic/errno-base.h>

bool failed = false;

void *task(void *ptr) {
    const auto fd = open("asdf", O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        printf("Failed to open\n");
        return nullptr;
    }
    printf("Opened fd %d\n", fd);



    struct flock lock2;
    lock2.l_type = F_WRLCK;
    lock2.l_whence = SEEK_SET;
    lock2.l_start = 0;
    lock2.l_len = 2;
    lock2.l_pid = 0;

    if (fcntl(fd, F_OFD_SETLK, &lock2)) {
        if (errno == EACCES || errno == EAGAIN) {
            printf("Failed to lock fd %d: already locked", fd);
            if (!ptr) failed = true;
            return nullptr;
        }
        printf("Failed to lock fd %d: %s\n", fd, strerror(errno));
        return nullptr;
    }
    printf("Locked fd %d\n", fd);

    if (errno == EACCES || errno == EAGAIN) {
        printf("Already Locked\n");
    } else {
        printf("File locked a second time.\n");
    }

    if (write(fd, "asdf", 5) != 5) {
        printf("Failed to write to fd %d: %s", fd, strerror(errno));
    }
    if (ptr) *(int*)ptr = fd;
    return nullptr;
}

int main(int argc, char **argv) {
    task(nullptr);

    int fd;
    task(&fd);
    close(fd);

    pthread_t thread_id;
    pthread_create(&thread_id, nullptr, task, nullptr);
    pthread_join(thread_id, nullptr);

    if (failed) return 0;
    return -1;
};

#elifdef CLOD_USE_WINDOWS
// https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-lockfileex
#error not implemented
#else
#error not implemented
#endif
