// #define ASIO_STANALONE // non-boost version
// #define ASIO_NO_DEPRECATED // no need for deprecated stuff
// #define ASIO_HAS_STD_COROUTINE // c++20 coroutines needed
// #include <asio.hpp>
#include <mpi.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include "../lib/net_protocol.hpp"

// asio::io_context context;
std::string storage_path = "/project/storage";
int master_rank = 0, rank, thread_count = 4;
int stripe_size;
std::ofstream log_file;

void init()
{
    // MPI_Status status;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    std::cout << rank << ": Connected to master with rank " << master_rank << std::endl;
    MPI_Recv(&stripe_size, 1, MPI_INT, master_rank, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    std::cout << "Received stripe_size: " << stripe_size << std::endl;
    log_file.open("/mnt/tmpfs/storage" + std::to_string(rank) + ".log", std::ios::app);
}

int write(const StoragePacket& request)
{
    std::string stripe_path = storage_path + Utils::get_string_from_byte_array(request.path) + "#" + std::to_string(request.offset);
    // std::cout << rank << ": path " << stripe_path << std::endl;
    Utils::PerformanceTimer timer("Slave handle_task", log_file);
    int fd = open(stripe_path.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        close(fd);
        // std::cout << rank << ": " << std::strerror(errno) << std::endl;
        return errno;
    }

    ssize_t nbytes = write(fd, request.data.data(), request.data_len);
    close(fd);
    return nbytes == request.data_len ? 0 : errno;
}

int read(const StoragePacket& request, std::vector<uint8_t>& result)
{
    std::string stripe_path = storage_path + Utils::get_string_from_byte_array(request.path) + "#" + std::to_string(request.offset);
    // std::cout << "Path: " << stripe_path << std::endl;
    int fd = open(stripe_path.c_str(), O_RDONLY, 0600);
    if (fd < 0)
    {
        // result = Utils::get_byte_array_from_int(errno);
        // std::cout << rank << ": " << std::strerror(errno) << std::endl;
        close(fd);
        return -1;
    }
    
    result.resize(stripe_size);
    ssize_t nbytes = read(fd, result.data(), stripe_size);
    
    if (nbytes == -1)
    {
        // result = Utils::get_byte_array_from_int(errno);
        // std::cout << rank << ": " << std::strerror(errno) << std::endl;
        close(fd);
        return -1;
    }
    
    if (nbytes < stripe_size)
        result.resize(nbytes);
    close(fd);
    return 0;
}

int remove(const StoragePacket& request)
{
    std::string stripe_path = storage_path + Utils::get_string_from_byte_array(request.path);
    std::string command = "rm -rf " + stripe_path + "#*";
    int err = system(command.c_str()); // this is vulnerable to command injection, but I'm too lazy to fix it

    return err;
}

// asio::awaitable<void> handle_task(uint8_t* data, int data_size, int tag) {
void handle_task(uint8_t* data, int data_size, int tag) {
    int result;
    StoragePacket response, request, node_response;
    int err;
    std::vector<uint8_t> node_data;
    std::vector<uint8_t> raw_buffer;
    // std::cout << rank << ": Received task (size = " << data_size << ") on tag " << tag << "\n";

    request.from_buffer(data, data_size);
    free(data);
    // std::cout << request.to_string() << std::endl;
    response.id = request.id;
    response.opcode = request.opcode;
    response.offset = request.offset;
    response.path_len = request.path_len;
    response.path = request.path;
    
    switch (request.opcode)
    {
        case OperationCode::Type::NOP:
            result = 0;
            break;
        case OperationCode::Type::RM_FILE:
            result = remove(request);
            break;
        case OperationCode::Type::WRITE:
            result = write(request);
            break;
        case OperationCode::Type::READ:
            node_response.id = request.id;
            node_response.opcode = request.opcode;
            node_response.path_len = request.path_len;
            node_response.path = request.path;
            node_response.offset = request.offset;
            err = read(request, node_data);
            if (err == -1)
            {
                node_response.rescode = ResultCode::Type::ERRMSG;
                node_response.message_len = 4;
                node_response.message = Utils::get_byte_array_from_int(errno);
                node_response.data_len = 0;
                node_response.data.clear();
            }
            else
            {
                node_response.rescode = ResultCode::Type::SUCCESS;
                node_response.data_len = node_data.size();
                node_response.data = node_data;
            }
            node_response.to_buffer(node_data); // reusing node_data vector
            MPI_Send(node_data.data(), node_data.size(), MPI_UNSIGNED_CHAR, master_rank, tag, MPI_COMM_WORLD);
            // co_return;
            return;
            break;
    }
    MPI_Send(&result, 1, MPI_INT, master_rank, tag, MPI_COMM_WORLD);
    
    // std::cout << rank << ": Sent " << result << " for offset " << request.offset << "\n";
    // co_return;
    return;
}

void listener_thread_func() {
    MPI_Status status;
    int data_size;

    // while (!context.stopped()) {
    while (true) {
        MPI_Probe(master_rank, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        MPI_Get_count(&status, MPI_UNSIGNED_CHAR, &data_size);

        int tag = status.MPI_TAG;
        uint8_t *data = (uint8_t*) malloc(data_size);
        MPI_Recv(data, data_size, MPI_UNSIGNED_CHAR, master_rank, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // asio::post(context, [data, data_size, tag]() {
        //     asio::co_spawn(context, handle_task(data, data_size, tag), asio::detached);
        // });
        handle_task(data, data_size, tag);
    }
}

int main(int argc, char** argv) {
    // MPI_Init(&argc, &argv);
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        std::cerr << "MPI does not support multiple threads!" << std::endl;
        exit(1);
    }

    init();

    // asio::signal_set signals(context, SIGINT, SIGTERM);
    // signals.async_wait([](const std::error_code&, int) {
    //     context.stop();
    // });

    // std::thread listener_thread(listener_thread_func);

    // std::vector<std::thread> thread_pool;
    // for (int i = 0; i < thread_count - 2; i ++) // one thread is the listener and one is main thread
    //     thread_pool.emplace_back([&]() { context.run(); });

    // context.run();

    // for (auto& t : thread_pool)
    // {
    //     if (t.joinable())
    //         t.join();
    // }
    listener_thread_func();
    // listener_thread.join();
    MPI_Finalize();
    return 0;
}
