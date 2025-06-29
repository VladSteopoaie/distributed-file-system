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
                    // Utils::PerformanceTimer timer("read", read_storage_log_file);
                    read(request, response);
                }
                // std::cout << response.to_string() << std::endl;
                break;
            case OperationCode::Type::WRITE:
                {
                    // Utils::PerformanceTimer timer("write", write_storage_log_file);
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
    std::vector<std::vector<uint8_t>> raw_buffers = std::vector<std::vector<uint8_t>>(stripes_num);
    std::vector<std::vector<uint8_t>> raw_responses = std::vector<std::vector<uint8_t>>(stripes_num);
    std::future<size_t> future_responses[stripes_num];
    int node, offset;
    int offset_sizes[stripes_num] = {0};
    node_request.opcode = OperationCode::Type::READ;
    node_request.path_len = request.path_len;
    node_request.path = request.path;
    // std::cout << "Stripes num: " << stripes_num << std::endl;
    for (size_t i = 0; i < stripes_num; i++) {
        node = (i + request.offset / stripe_size) % connections.size();
        offset = request.offset + i * stripe_size;
        
        node_request.id = Utils::generate_id();
        node_request.offset = offset;
        node_request.to_buffer(raw_buffers[i]);
        // connections[node].send_data_async(raw_buffers[i]);
        future_responses[i] = asio::co_spawn(
            context,
            connections[node].send_receive_data_async(context, raw_buffers[i], raw_responses[i]),
            asio::use_future
        );
    }
    
    StoragePacket node_response;
    for (int i = 0; i < stripes_num; i ++)
    {
        future_responses[i].wait();
        node = (i + request.offset / stripe_size) % connections.size(); // !!! assuming master node has rank 0 !!!
        offset = request.offset + i * stripe_size;
        // std::cout << "Node: " << node << std::endl;
        // std::cout << "Offset: " << offset << std::endl;
        
        // std::cout << "Raw response size: " << raw_responses[i].size() << std::endl;
        node_response.from_buffer(raw_responses[i].data(), raw_responses[i].size());
        // std::cout << "Node response: " << node_response.to_string() << std::endl;
        if (node_response.rescode != ResultCode::Type::SUCCESS)
        {
            response.rescode = ResultCode::Type::ERRMSG;
            response.message = node_response.message;
            response.message_len = response.message.size();
            return;
        }
        if (node_response.rescode == ResultCode::Type::SUCCESS)
        {
            // std::cout << node_response.to_string() << std::endl;
            offset_sizes[i] = node_response.data_len;
            // std::cout << offset << " " << request.offset << std::endl;
            response.data.resize(response.data.size() + node_response.data_len);
            std::copy(node_response.data.begin(), node_response.data.end(), response.data.begin() + offset - request.offset);
            // std::cout << "Stripe offset: " << offset << " recovered!" << std::endl;
        }
    }
    std::cout << "All stripes recovered!" << std::endl;
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

void StorageConnectionHandler::write(const StoragePacket& request, StoragePacket& response)
{    
    size_t stripes_num = request.data.size() / stripe_size + (request.data.size() % stripe_size != 0 ? 1 : 0);
    size_t last_stripe_size = request.data.size() % stripe_size;
    if (last_stripe_size == 0 && stripes_num > 0) {
        last_stripe_size = stripe_size;
    }
    
    StoragePacket node_request, node_response;
    std::vector<std::vector<uint8_t>> raw_buffers = std::vector<std::vector<uint8_t>>(stripes_num);
    std::vector<std::vector<uint8_t>> raw_responses = std::vector<std::vector<uint8_t>>(stripes_num);
    std::future<size_t> future_responses[stripes_num];
    node_request.opcode = OperationCode::Type::WRITE;
    node_request.path_len = request.path_len;
    node_request.path = request.path;

    size_t offset, final_size;
    int node;

    for (size_t i = 0; i < stripes_num; i++) {
        final_size = (i == stripes_num - 1) ? last_stripe_size : stripe_size;
        node = ((i + request.offset / stripe_size) % connections.size()); // !!! assuming master node has rank 0 !!!
        offset = request.offset + i * stripe_size;

        node_request.id = Utils::generate_id();
        node_request.data_len = final_size;
        node_request.offset = offset;
        node_request.data.assign(request.data.begin() + i * stripe_size, request.data.begin() + i * stripe_size + final_size);
        node_request.to_buffer(raw_buffers[i]);
        // connections[node].send_data_async(raw_buffers[i]);
        future_responses[i] = asio::co_spawn(
            context,
            connections[node].send_receive_data_async(context, raw_buffers[i], raw_responses[i]),
            asio::use_future
        );
    }

  
    for (int i = 0; i < stripes_num; i ++)
    {
        future_responses[i].wait();
        node = (i + request.offset / stripe_size) % connections.size(); // !!! assuming master node has rank 0 !!!
       
        node_response.from_buffer(raw_responses[i].data(), raw_responses[i].size());
        if (node_response.rescode != ResultCode::Type::SUCCESS)
        {
            response.rescode = ResultCode::Type::ERRMSG;
            response.message = node_response.message;
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
    std::vector<std::vector<uint8_t>> responses = std::vector<std::vector<uint8_t>>(connections.size());
    std::future<size_t> futures[connections.size()];
    for (int i = 0; i < connections.size(); i ++)
    {
        // connections[i].send_data_async(raw_buffer);
        futures[i] = asio::co_spawn(
            context,
            connections[i].send_receive_data_async(context, raw_buffer, responses[i]),
            asio::use_future
        );
    }

    StoragePacket node_response;
    for (int i = 0; i < connections.size(); i ++)
    {
        futures[i].wait();
        node_response.from_buffer(responses[i].data(), responses[i].size());
        if (node_response.rescode != ResultCode::Type::SUCCESS)
        {
            response.rescode = ResultCode::Type::ERRMSG;
            response.message = Utils::get_byte_array_from_string("Error removing file");
            response.message_len = response.message.size();
            return;
        }
    }

    response.rescode = ResultCode::Type::SUCCESS;
}


StorageConnectionHandler::StorageConnectionHandler(
    asio::io_context& context, 
    size_t stripe_size, 
    std::vector<Utils::ConnectionInfo<StoragePacket>>& connections
)
    : GenericConnectionHandler<StoragePacket>::GenericConnectionHandler(context)
    , stripe_size(stripe_size)
    , connections(connections)
     {}

