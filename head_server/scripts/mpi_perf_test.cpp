#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <chrono>

// Constants
#define STRIPE_SIZE 4096  // 4 KB

// This will simulate the storage write operation
void simulate_storage_write(int rank, int comm_size, const char *data, size_t data_size) {
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time = std::chrono::high_resolution_clock::now();
    size_t stripes_num = data_size / STRIPE_SIZE + (data_size % STRIPE_SIZE != 0 ? 1 : 0);
    size_t last_stripe_size = data_size % STRIPE_SIZE;
    if (last_stripe_size == 0 && stripes_num > 0) {
        last_stripe_size = STRIPE_SIZE;
    }

    MPI_Request send_requests[stripes_num], recv_requests[stripes_num];
    int responses[stripes_num];
    unsigned char *raw_buffers[stripes_num];
    
    for (size_t i = 0; i < stripes_num; i++) {
        raw_buffers[i] = (unsigned char*)malloc(STRIPE_SIZE);
    }

    // Initialize a simulated StoragePacket for each stripe
    for (size_t i = 0; i < stripes_num; i++) {
        size_t final_size = (i == stripes_num - 1) ? last_stripe_size : STRIPE_SIZE;
        size_t offset = i * STRIPE_SIZE;
        memcpy(raw_buffers[i], data + offset, final_size);

        // Simulate sending data to the other nodes
        int node = (i % (comm_size - 1)) + 1;  // Simulate sending to different nodes
        MPI_Isend(raw_buffers[i], final_size, MPI_UNSIGNED_CHAR, node, i, MPI_COMM_WORLD, &send_requests[i]);
        MPI_Irecv(&responses[i], 1, MPI_INT, node, i, MPI_COMM_WORLD, &recv_requests[i]);
    }

    // Wait for all communications to finish
    MPI_Waitall(stripes_num, send_requests, MPI_STATUSES_IGNORE);
    MPI_Waitall(stripes_num, recv_requests, MPI_STATUSES_IGNORE);

    // Check responses and print result
    for (int i = 0; i < stripes_num; i++) {
        if (responses[i] != 0) {
            printf("Error on stripe %d: %d\n", i, responses[i]);
        }
    }

    // Free memory
    for (size_t i = 0; i < stripes_num; i++) {
        free(raw_buffers[i]);
    }

    std::chrono::time_point<std::chrono::high_resolution_clock> end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    printf("write operation in %ld microseconds\n", duration);
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Make sure the number of processes is appropriate
    if (size < 2) {
        printf("This simulation requires at least 2 processes.\n");
        MPI_Finalize();
        return 1;
    }

    int data_size = 1024 * 1024 * 128; // 1 MB of data
    const char *data = (char *)malloc(data_size);
    char bla;
    while(true) {
        printf("Wait\n");
        if (read(0, &bla, 1) == -1) {
            perror("read");
            MPI_Finalize();
            return 1;
        }
    
        // Simulate the storage write operation for each process
        simulate_storage_write(rank, size, data, data_size);
     }

    MPI_Finalize();
    return 0;
}
