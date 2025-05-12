#include "storage_server.hpp"

using namespace StorageAPI;

template class GenericServer<StorageConnectionHandler>;

StorageServer::StorageServer() : StorageServer(1) {}


StorageServer::StorageServer(int thread_count)
    : StorageServer(thread_count, 4096) {} // default stripe size: 4KB

StorageServer::StorageServer(int thread_count, int stripe_size)
    : GenericServer<StorageConnectionHandler>::GenericServer(thread_count)
    , stripe_size(stripe_size)
{
    // MPI_Datatype dtype;
    // int err = MPI_Type_match_size(MPI_TYPECLASS_INTEGER, sizeof(stripe_size), &dtype);

    // if (err == MPI_SUCCESS) {
    //     if (dtype == MPI_INT)
    //         std::cout << "master matches MPI_INT\n";
    //     else if (dtype == MPI_UNSIGNED)
    //         std::cout << "master matches MPI_UNSIGNED\n";
    //     else if (dtype == MPI_LONG)
    //         std::cout << "master matches MPI_LONG\n";
    //     else if (dtype == MPI_UNSIGNED_LONG)
    //         std::cout << "master matches MPI_UNSIGNED_LONG\n";
    //     else if (dtype == MPI_LONG_LONG)
    //         std::cout << "master matches MPI_LONG_LONG\n";
    //     else
    //         std::cout << "master matches some other integer MPI type\n";
    // } else {
    //     std::cerr << "No matching MPI integer type found for master\n";
    // }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    // initializing connection with mpi masters
    // sending the rank of the master process
    for (int i = 0; i < comm_size; i ++)
    {
        if (i == rank)
            continue;
        
        // MPI_Send(&rank, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        int r = stripe_size;
        MPI_Send(&r, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
    }

    SPDLOG_INFO("Server has rank {}", rank);
}

void StorageServer::run(uint16_t port) {
    GenericServer<StorageConnectionHandler>::run(port, rank, comm_size, stripe_size);
}

StorageServer::~StorageServer()
{
    SPDLOG_INFO("Exiting Storage Server.");
}