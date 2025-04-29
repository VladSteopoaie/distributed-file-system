#include "storage_server.hpp"

using namespace StorageAPI;

template class GenericServer<StorageConnectionHandler>;

StorageServer::StorageServer(int thread_count)
    : GenericServer<StorageConnectionHandler>::GenericServer(thread_count)
    {}

void StorageServer::run(uint16_t port) {
    GenericServer<StorageConnectionHandler>::run(port);
}

StorageServer::~StorageServer()
{
    SPDLOG_INFO("Exiting Storage Server.");
}