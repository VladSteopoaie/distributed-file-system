#include <mpi.h>
#include <iostream>
#include <cstdint>
#include <unistd.h>

int master_rank = 0, rank, thread_count = 4;

int main(int argc, char** argv) {
    // MPI_Init(&argc, &argv);
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        std::cerr << "MPI does not support multiple threads!" << std::endl;
        exit(1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Status status;
    int data_size;

    while (true) {
        MPI_Probe(master_rank, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        MPI_Get_count(&status, MPI_UNSIGNED_CHAR, &data_size);

        int tag = status.MPI_TAG;
        uint8_t *data = (uint8_t*) malloc(data_size);
        MPI_Recv(data, data_size, MPI_UNSIGNED_CHAR, master_rank, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        usleep(200);
        int r = 0;
        MPI_Send(&r, 1, MPI_INT, master_rank, tag, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}
