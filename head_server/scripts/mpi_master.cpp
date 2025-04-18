// combined.cpp
#include <mpi.h>
#include <iostream>
#include <string>
#include <array>
#include <memory>
#include <cstdio>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    std::cout << "I am MASTER with rank: " << rank << std::endl;

    for (int i = 0; i < size; i ++)
    {
        if (i == rank) continue;

        MPI_Send(&rank, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }
    
    MPI_Finalize();
    return 0;
}