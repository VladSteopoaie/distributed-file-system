#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#define CHUNK_SIZE (1024 * 512)  // 1 MB

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source> <destination>\n", argv[0]);
        return 1;
    }

    int src = open(argv[1], O_RDONLY);
    if (src < 0) {
        perror("open src");
        return 1;
    }

    struct stat st;
    if (fstat(src, &st) < 0) {
        perror("fstat");
        close(src);
        return 1;
    }

    int dst = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, st.st_mode);
    if (dst < 0) {
        perror("open dst");
        close(src);
        return 1;
    }

    char *buf = malloc(st.st_size);
    if (!buf) {
        perror("malloc");
        close(src);
        close(dst);
        return 1;
    }

    ssize_t bytes;
    while ((bytes = read(src, buf, st.st_size)) > 0) {
        ssize_t written = 0;
        while (written < bytes) {
            ssize_t w = write(dst, buf + written, bytes - written);
            printf("Wrote %zd bytes\n", bytes);
            if (w < 0) {
                perror("write");
                free(buf);
                close(src);
                close(dst);
                return 1;
            }
            written += w;
        }
    }

    if (bytes < 0)
        perror("read");

    free(buf);
    close(src);
    close(dst);
    return 0;
}
