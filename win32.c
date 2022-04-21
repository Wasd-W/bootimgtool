#include <unistd.h>

#include "win32.h"

ssize_t pread(int fd, void *buf, size_t size, off_t offset)
{
    int current_pos = lseek(fd, 0, SEEK_CUR);
    lseek(fd, offset, SEEK_SET);
    ssize_t bytes_read = read(fd, buf, size);
    lseek(fd, current_pos, SEEK_SET);
    return bytes_read;
}