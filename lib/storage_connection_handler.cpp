#include "storage_connection_handler.hpp"

using namespace StorageAPI;

std::ofstream write_storage_log_file("/mnt/tmpfs/storage_mngr_write.log", std::ios::app);
std::ofstream read_storage_log_file("/mnt/tmpfs/storage_mngr_read.log", std::ios::app);

void StorageConnectionHandler::handle_request(const StoragePacket& request, StoragePacket& response)
{
    if (request.id == 0)
    {
        response.rescode = ResultCode::to_byte(ResultCode::Type::INVPKT);
        return;
    }

    response.id = request.id; 
    response.opcode = request.opcode;
    response.path_len = request.path_len;
    response.path = request.path;
    SPDLOG_DEBUG(std::format("Processing: {}", OperationCode::to_string(OperationCode::from_byte(request.opcode))));
    try {
        switch (OperationCode::from_byte(request.opcode))
        {
            case OperationCode::Type::NOP:
                response.rescode = ResultCode::to_byte(ResultCode::Type::SUCCESS);
                break;
            case OperationCode::Type::INIT:
                init_connection(request.id, response);
                break;
            case OperationCode::Type::READ:
                {
                    Utils::PerformanceTimer timer("read", read_storage_log_file);
                    read(request, response);
                }
                // std::cout << response.to_string() << std::endl;
                break;
            case OperationCode::Type::WRITE:
                {
                    Utils::PerformanceTimer timer("write", write_storage_log_file);
                    write(request, response);
                }
                break;
            case OperationCode::Type::RM_FILE:
                remove(request, response);
                break;
            
            default:
                response.rescode = ResultCode::to_byte(ResultCode::Type::INVOP);
                break;
        } // switch
    }
    catch (std::exception& e)
    {
        SPDLOG_ERROR(e.what());
        response.rescode = ResultCode::to_byte(ResultCode::Type::ERRMSG);
        response.message_len = 4;
        response.message = Utils::get_byte_array_from_int(errno);
    }
} // hanlde_request

void StorageConnectionHandler::init_connection(uint16_t id, StoragePacket& response)
{
    response.rescode = ResultCode::Type::SUCCESS;
}


void StorageConnectionHandler::read(const StoragePacket& request, StoragePacket& response)
{
    response.rescode = ResultCode::Type::SUCCESS;
    int data_len = Utils::get_int_from_byte_array(request.data);
    size_t stripes_num = data_len / stripe_size + (data_len % stripe_size != 0 ? 1 : 0);
    StoragePacket node_request;
    std::vector<uint8_t> raw_buffer;
    // size_t raw_buffer_size;
    int node, offset;
    std::vector<int> responses = std::vector<int>(stripes_num);
    std::vector<MPI_Request> requests = std::vector<MPI_Request>(stripes_num);
    
    node_request.opcode = OperationCode::Type::READ;
    node_request.path_len = request.path_len;
    node_request.path = request.path;
    for (size_t i = 0; i < stripes_num; i++) {
        node = ((i + request.offset / stripe_size) % (comm_size - 1)) + 1; // !!! assuming master node has rank 0 !!!
        offset = request.offset + i * stripe_size;

        node_request.id = Utils::generate_id();
        node_request.offset = offset;
        node_request.to_buffer(raw_buffer);
        // raw_buffer_size = raw_buffer.size();
        // std::cout << "data_size: " << raw_buffer_size  << "\n"
        //     << "node: " << node << "\n"
        //     << "offset: " << offset << "\n";
        MPI_Send(raw_buffer.data(), raw_buffer.size(), MPI_UNSIGNED_CHAR, node, offset, MPI_COMM_WORLD);
    }

    int completed = 0, flag;
    int offset_sizes[stripes_num] = {0};
    // bool valid_response = true;
    std::vector<uint8_t> received_bytes;
    received_bytes.reserve(StoragePacket::header_size + stripe_size);
    StoragePacket node_response;
    response.data.resize(data_len);
    response.data_len = data_len;
    MPI_Status status;
    while (completed < stripes_num) {
        for (int i = 0; i < stripes_num; ++i) {
            node = ((i + request.offset / stripe_size) % (comm_size - 1)) + 1; // !!! assuming master node has rank 0 !!!
            offset = request.offset + i * stripe_size;
            flag = 0;

            MPI_Iprobe(node, offset, MPI_COMM_WORLD, &flag, &status);
            
            if (flag) {
                int size;
                MPI_Get_count(&status, MPI_UNSIGNED_CHAR, &size);
                if (received_bytes.size() != size)
                    received_bytes.resize(size);

                MPI_Recv(received_bytes.data(), size, MPI_UNSIGNED_CHAR, node, offset, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                completed++;
                // std::cout << completed << " < " << stripes_num << std::endl;
                // if (valid_response)
                // {
                node_response.from_buffer(received_bytes.data(), received_bytes.size());

                // if (node_response.rescode == ResultCode::Type::ERRMSG)
                // {
                //     // response.rescode = ResultCode::Type::ERRMSG;
                //     // response.message_len = node_response.message_len;
                //     // response.message = node_response.message;
                //     // valid_response = false;
                //     // std::cout << "Stripe offset: " << offset << " Error!" << std::endl;
                //     ;
                // }
                if (node_response.rescode == ResultCode::Type::SUCCESS)
                {
                    // std::cout << node_response.to_string() << std::endl;
                    offset_sizes[i] = node_response.data_len;
                    std::copy(node_response.data.begin(), node_response.data.end(), response.data.begin() + offset - request.offset);
                    // std::cout << "Stripe offset: " << offset << " recovered!" << std::endl;
                }
                // } // if (valid_response)
            } // if (flag)
        } // for
    } // while

    int final_size = 0;
    bool encountered_null = false;
    // std::cout << stripes_num << std::endl;
    for (int i = 0; i < stripes_num; i++)
    {
        final_size += offset_sizes[i];
        // std::cout << i << " " << offset_sizes[i] << std::endl;
        // std::cout << encountered_null << std::endl;
        if (encountered_null == false)
        {
            if (offset_sizes[i] == 0)
                encountered_null = true;
            // std::cout << "first branch" << std::endl;
        }
        else {
            // std::cout << "second branch" << std::endl;
            if (offset_sizes[i] != 0)
            {
                std::string message = "Fragmented result, something bad happened!"; 
                response.rescode = ResultCode::Type::ERRMSG;
                response.message_len = message.length();
                response.message = Utils::get_byte_array_from_string(message);
                response.data_len = 0;
                response.data.clear();
                return;
            }
        }
        // std::cout << "i = " << i << std::endl;
    }
    // std::cout << std::endl;
    response.data_len = final_size;
    response.data.resize(final_size);
}

// void StorageConnectionHandler::write(const StoragePacket& request, StoragePacket& response)
// {    
//     size_t stripes_num = request.data.size() / stripe_size + (request.data.size() % stripe_size != 0 ? 1 : 0);
//     size_t last_stripe_size = request.data.size() % stripe_size;
//     if (last_stripe_size == 0 && stripes_num > 0) {
//         last_stripe_size = stripe_size;
//     }
    
//     // int node;
//     StoragePacket node_request;
//     std::vector<uint8_t> raw_buffers[comm_size - 1];
//     MPI_Request send_requests[comm_size - 1], recv_requests[stripe_size];
//     int responses[stripes_num];
//     for (size_t node = 1; node < comm_size; node++)
//     {
//         raw_buffers[node - 1].resize(StoragePacket::header_size + stripe_size + 1024); // 1024 is the path max size
//         MPI_Send_init(raw_buffers[node - 1].data(), raw_buffers[node - 1].size(), MPI_UNSIGNED_CHAR, node, node, MPI_COMM_WORLD, &send_requests[node - 1]);
//     }
//     node_request.path_len = request.path_len;
//     node_request.path = request.path;
//     for (size_t stripe_idx = 0; stripe_idx < stripes_num; stripe_idx += comm_size - 1) {
//         for (size_t node = 1; node < comm_size; node++) {
//             node_request.id = Utils::generate_id();
//             size_t current_stripe = stripe_idx + (node - 1);
//             if (current_stripe >= stripes_num)
//             {
//                 node_request.opcode = OperationCode::Type::NOP;
//                 node_request.data_len = 0;
//                 node_request.to_buffer_no_resize(raw_buffers[node - 1]);
//             }
//             else
//             {
//                 node_request.opcode = OperationCode::Type::WRITE;
//                 // size_t final_size = 
//                 // size_t offset = 
//                 node_request.data_len = (current_stripe == stripes_num - 1) ? last_stripe_size : stripe_size;;
//                 node_request.offset = request.offset + current_stripe * stripe_size;;
//                 node_request.data.assign(request.data.begin() + current_stripe * stripe_size, request.data.begin() + current_stripe * stripe_size + node_request.data_len);
//                 // std::cout << "data_size: " << final_size  << "\n"
//                 //     << "node: " << node << "\n"
//                 //     << "offset: " <<  << "\n";
//                 // std::cout << "Current stripe: " << current_stripe << std::endl;
//                 // std::cout << node_request.to_string() << std::endl;
//                 node_request.to_buffer_no_resize(raw_buffers[node - 1]);
//                 MPI_Irecv(&responses[current_stripe], 1, MPI_INT, node, node, MPI_COMM_WORLD, &recv_requests[current_stripe]);
//             }
//             // raw_buffers[node - 1].resize(StoragePacket::header_size + stripe_size + 1024); // 1024 is the path max size
//             // std::cout << "node: " << node << ", stripe: " << current_stripe << std::endl;
//         }
//         MPI_Startall(comm_size - 1, send_requests);
//         MPI_Waitall(comm_size - 1, send_requests, MPI_STATUS_IGNORE);
//         // MPI_Isend(raw_buffers[i].data(), raw_buffers[i].size(), MPI_UNSIGNED_CHAR, node, offset, MPI_COMM_WORLD, &send_requests[i]);
//         // MPI_Irecv(&responses[i], 1, MPI_INT, node, offset, MPI_COMM_WORLD, &requests[i]);
//     }

//     // MPI_Waitall(send_requests.size(), send_requests.data(), MPI_STATUSES_IGNORE);
//     MPI_Waitall(stripes_num, recv_requests, MPI_STATUSES_IGNORE);

//     for (int i = 0; i < stripes_num; i ++)
//     {
//         int node = ((i + request.offset / stripe_size) % (comm_size - 1)) + 1; // !!! assuming master node has rank 0 !!!
//         size_t offset = request.offset + i * stripe_size;
//         // std::cout << "Node: " << node << ", Offset: " << offset << ", Sent ack: " << responses[i] << std::endl;
//         if (responses[i] != 0)
//         {
//             response.rescode = ResultCode::Type::ERRMSG;
//             response.message = Utils::get_byte_array_from_int(responses[i]);
//             response.message_len = response.message.size();
//             return;
//         }
//     }

//     response.rescode = ResultCode::Type::SUCCESS;
// }

// void StorageConnectionHandler::write(const StoragePacket& request, StoragePacket& response)
// {    
//     size_t stripes_num = request.data.size() / stripe_size + (request.data.size() % stripe_size != 0 ? 1 : 0);
//     size_t last_stripe_size = request.data.size() % stripe_size;
//     if (last_stripe_size == 0 && stripes_num > 0) {
//         last_stripe_size = stripe_size;
//     }
    
//     StoragePacket node_request;
//     std::vector<uint8_t> raw_buffers[stripes_num];
//     int responses[stripes_num];
//     // std::vector<MPI_Request> requests = std::vector<MPI_Request>(stripes_num);
//     // std::vector<MPI_Request> send_requests = std::vector<MPI_Request>(stripes_num);
//     MPI_Request recv_requests[stripes_num], send_requests[stripes_num];
//     node_request.opcode = OperationCode::Type::WRITE;
//     node_request.path_len = request.path_len;
//     node_request.path = request.path;
//     for (size_t node = 1; node < comm_size; node++)
//     {
//         int first_stripe = node - 1;
//         for (size_t i = first_stripe; i < stripes_num; i += comm_size - 1) {
//             size_t final_size = (i == stripes_num - 1) ? last_stripe_size : stripe_size;
//             // int node = ((i + request.offset / stripe_size) % (comm_size - 1)) + 1; // !!! assuming master node has rank 0 !!!
//             size_t offset = request.offset + i * stripe_size;
            
//             node_request.id = Utils::generate_id();
//             node_request.data_len = final_size;
//             node_request.offset = offset;
//             node_request.data.assign(request.data.begin() + i * stripe_size, request.data.begin() + i * stripe_size + final_size);
//             node_request.to_buffer(raw_buffers[i]);
//             // std::cout << "data_size: " << raw_buffer_size  << "\n"
//             //     << "node: " << node << "\n"
//             //     << "offset: " << offset << "\n";
//             MPI_Isend(raw_buffers[i].data(), raw_buffers[i].size(), MPI_UNSIGNED_CHAR, node, offset, MPI_COMM_WORLD, &send_requests[i]);
//             MPI_Irecv(&responses[i], 1, MPI_INT, node, offset, MPI_COMM_WORLD, &recv_requests[i]);
//         }
//     }

//     MPI_Waitall(stripes_num, send_requests, MPI_STATUSES_IGNORE);
//     MPI_Waitall(stripes_num, recv_requests, MPI_STATUSES_IGNORE);

//     for (int i = 0; i < stripes_num; i ++)
//     {
//         int node = ((i + request.offset / stripe_size) % (comm_size - 1)) + 1; // !!! assuming master node has rank 0 !!!
//         size_t offset = request.offset + i * stripe_size;
//         // std::cout << "Node: " << node << ", Offset: " << offset << ", Sent ack: " << responses[i] << std::endl;
//         if (responses[i] != 0)
//         {
//             response.rescode = ResultCode::Type::ERRMSG;
//             response.message = Utils::get_byte_array_from_int(responses[i]);
//             response.message_len = response.message.size();
//             return;
//         }
//     }

//     response.rescode = ResultCode::Type::SUCCESS;
// }

void StorageConnectionHandler::write(const StoragePacket& request, StoragePacket& response)
{    
    size_t stripes_num = request.data.size() / stripe_size + (request.data.size() % stripe_size != 0 ? 1 : 0);
    size_t last_stripe_size = request.data.size() % stripe_size;
    if (last_stripe_size == 0 && stripes_num > 0) {
        last_stripe_size = stripe_size;
    }
    
    StoragePacket node_request;
    std::vector<std::vector<uint8_t>> raw_buffers = std::vector<std::vector<uint8_t>>(stripes_num);
    int responses[stripes_num];
    MPI_Request requests[stripes_num], send_requests[stripes_num];
    node_request.opcode = OperationCode::Type::WRITE;
    node_request.path_len = request.path_len;
    node_request.path = request.path;

    size_t offset, final_size;
    int node;

    for (size_t i = 0; i < stripes_num; i++) {
        final_size = (i == stripes_num - 1) ? last_stripe_size : stripe_size;
        node = ((i + request.offset / stripe_size) % (comm_size - 1)) + 1; // !!! assuming master node has rank 0 !!!
        offset = request.offset + i * stripe_size;

        node_request.id = Utils::generate_id();
        node_request.data_len = final_size;
        node_request.offset = offset;
        node_request.data.assign(request.data.begin() + i * stripe_size, request.data.begin() + i * stripe_size + final_size);
        node_request.to_buffer(raw_buffers[i]);
        // std::cout << "data_size: " << raw_buffer_size  << "\n"
        //     << "node: " << node << "\n"
        //     << "offset: " << offset << "\n";
        MPI_Isend(raw_buffers[i].data(), raw_buffers[i].size(), MPI_UNSIGNED_CHAR, node, offset, MPI_COMM_WORLD, &send_requests[i]);
        MPI_Irecv(&responses[i], 1, MPI_INT, node, offset, MPI_COMM_WORLD, &requests[i]);    
    }

    // MPI_Waitall(stripes_num, send_requests, MPI_STATUSES_IGNORE);
    MPI_Waitall(stripes_num, requests, MPI_STATUSES_IGNORE);

    for (int i =0; i < stripes_num; i ++)
    {
        node = ((i + request.offset / stripe_size) % (comm_size - 1)) + 1; // !!! assuming master node has rank 0 !!!
        offset = request.offset + i * stripe_size;
        // std::cout << "Node: " << node << ", Offset: " << offset << ", Sent ack: " << responses[i] << std::endl;
        if (responses[i] != 0)
        {
            response.rescode = ResultCode::Type::ERRMSG;
            response.message = Utils::get_byte_array_from_int(responses[i]);
            response.message_len = response.message.size();
            return;
        }
    }

    response.rescode = ResultCode::Type::SUCCESS;
}

void StorageConnectionHandler::remove(const StoragePacket& request, StoragePacket& response)
{
    std::vector<uint8_t> raw_buffer;
    request.to_buffer(raw_buffer);
    std::vector<int> responses = std::vector<int>(comm_size - 1);
    std::vector<MPI_Request> requests = std::vector<MPI_Request>(comm_size - 1);
    for (int i = 1; i < comm_size; i++)
    {
        MPI_Send(raw_buffer.data(), raw_buffer.size(), MPI_UNSIGNED_CHAR, i, i, MPI_COMM_WORLD);
        MPI_Irecv(&responses[i - 1], 1, MPI_INT, i, i, MPI_COMM_WORLD, &requests[i - 1]);
    }

    MPI_Waitall(requests.size(), requests.data(), MPI_STATUSES_IGNORE);

    for (int i = 1; i < comm_size; i ++)
    {
        if (responses[i - 1] != 0)
        {
            response.rescode = ResultCode::Type::ERRMSG;
            response.message = Utils::get_byte_array_from_string("Error removing file");
            response.message_len = response.message.size();
            return;
        }
    }

    response.rescode = ResultCode::Type::SUCCESS;
}


StorageConnectionHandler::StorageConnectionHandler(asio::io_context& context, int rank, int comm_size, size_t stripe_size)
    : GenericConnectionHandler<StoragePacket>::GenericConnectionHandler(context)
    , rank(rank), comm_size(comm_size), stripe_size(stripe_size) {}

