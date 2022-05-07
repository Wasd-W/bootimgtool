ssize_t pread(int fd, void *buf, size_t size, off_t offset);

#define open(filename, flags, ...) \
({                                 \
    int fd = 0;                    \
    FILE *fs = NULL;               \
    char *mode = "rb";             \
    if(flags & O_WRONLY)           \
    {                              \
        mode = "wb";               \
    }                              \
    fs = fopen(filename, mode);    \
    if(fs == NULL)                 \
        fd = -1;                   \
    else                           \
        fd = fileno(fs);           \
    fd;                            \
})
