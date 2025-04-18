// combined.cpp
#include <mpi.h>
#include <iostream>
#include <string>
#include <array>
#include <memory>
#include <cstdio>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size, master_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Recv(&master_rank, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    std::cout << "I am a slave process with rank: " << rank << " my master is: " << master_rank << std::endl;

    MPI_Finalize();
    return 0;
}