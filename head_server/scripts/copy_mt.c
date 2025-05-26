#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>

#define CHUNK_SIZE (1024 * 1024)  // 1 MB
#define QUEUE_CAPACITY 16
#define NUM_WRITERS 4
#define NUM_READERS 4

typedef struct {
    off_t offset;
    ssize_t size;
    char *data;
} Chunk;

typedef struct {
    Chunk *chunks[QUEUE_CAPACITY];
    int front, rear, count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty, not_full;
    int readers_done;
} ChunkQueue;

ChunkQueue queue = {
    .front = 0, .rear = 0, .count = 0, .readers_done = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .not_empty = PTHREAD_COND_INITIALIZER,
    .not_full = PTHREAD_COND_INITIALIZER
};

int src_fd, dst_fd;
off_t file_size;

void enqueue_chunk(Chunk *chunk) {
    pthread_mutex_lock(&queue.mutex);
    while (queue.count == QUEUE_CAPACITY)
        pthread_cond_wait(&queue.not_full, &queue.mutex);

    queue.chunks[queue.rear] = chunk;
    queue.rear = (queue.rear + 1) % QUEUE_CAPACITY;
    queue.count++;
    pthread_cond_signal(&queue.not_empty);
    pthread_mutex_unlock(&queue.mutex);
}

Chunk* dequeue_chunk() {
    pthread_mutex_lock(&queue.mutex);
    while (queue.count == 0 && queue.readers_done < NUM_READERS)
        pthread_cond_wait(&queue.not_empty, &queue.mutex);

    if (queue.count == 0 && queue.readers_done == NUM_READERS) {
        pthread_mutex_unlock(&queue.mutex);
        return NULL;
    }

    Chunk *chunk = queue.chunks[queue.front];
    queue.front = (queue.front + 1) % QUEUE_CAPACITY;
    queue.count--;
    pthread_cond_signal(&queue.not_full);
    pthread_mutex_unlock(&queue.mutex);
    return chunk;
}

void* reader_thread(void *arg) {
    int reader_id = *(int *)arg;
    free(arg);

    off_t start = reader_id * (file_size / NUM_READERS);
    off_t end = (reader_id == NUM_READERS - 1) ? file_size : start + (file_size / NUM_READERS);
    off_t offset = start;

    while (offset < end) {
        ssize_t chunk_size = CHUNK_SIZE;
        if (offset + CHUNK_SIZE > end)
            chunk_size = end - offset;

        char *buf = malloc(chunk_size);
        if (!buf) break;

        ssize_t n = pread(src_fd, buf, chunk_size, offset);
        if (n < 0) {
            perror("pread");
            free(buf);
            break;
        }
        if (n == 0) {
            free(buf);
            break;
        }

        Chunk *chunk = malloc(sizeof(Chunk));
        chunk->offset = offset;
        chunk->size = n;
        chunk->data = buf;

        offset += n;
        enqueue_chunk(chunk);
    }

    pthread_mutex_lock(&queue.mutex);
    queue.readers_done++;
    pthread_cond_broadcast(&queue.not_empty);
    pthread_mutex_unlock(&queue.mutex);

    return NULL;
}

void* writer_thread(void *arg) {
    (void)arg;
    while (1) {
        Chunk *chunk = dequeue_chunk();
        if (!chunk) break;

        ssize_t written = 0;
        while (written < chunk->size) {
            ssize_t w = pwrite(dst_fd, chunk->data + written, chunk->size - written, chunk->offset + written);
            if (w < 0) {
                perror("pwrite");
                break;
            }
            written += w;
        }

        free(chunk->data);
        free(chunk);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source> <destination>\n", argv[0]);
        return 1;
    }

    src_fd = open(argv[1], O_RDONLY);
    if (src_fd < 0) {
        perror("open src");
        return 1;
    }

    struct stat st;
    if (fstat(src_fd, &st) < 0) {
        perror("fstat");
        close(src_fd);
        return 1;
    }
    file_size = st.st_size;

    dst_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, st.st_mode);
    if (dst_fd < 0) {
        perror("open dst");
        close(src_fd);
        return 1;
    }

    pthread_t readers[NUM_READERS], writers[NUM_WRITERS];
    for (int i = 0; i < NUM_READERS; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&readers[i], NULL, reader_thread, id);
    }

    for (int i = 0; i < NUM_WRITERS; i++)
        pthread_create(&writers[i], NULL, writer_thread, NULL);

    for (int i = 0; i < NUM_READERS; i++)
        pthread_join(readers[i], NULL);

    for (int i = 0; i < NUM_WRITERS; i++)
        pthread_join(writers[i], NULL);

    close(src_fd);
    close(dst_fd);
    return 0;
}
