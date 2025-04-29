#include "storage_connection_handler.hpp"

using namespace StorageAPI;

void StorageConnectionHandler::handle_request(const StoragePacket& request, StoragePacket& response)
{
    if (request.id == 0)
    {
        response.rescode = ResultCode::to_byte(ResultCode::Type::INVPKT);
        return;
    }

    response.id = request.id; 
    response.opcode = request.opcode;
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
                read(request, response);
                break;
            case OperationCode::Type::WRITE:
                write(request, response);
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
    response.id = id;
    response.rescode = ResultCode::Type::SUCCESS;
    response.data_len = 0;
    response.message_len = 0;
    response.message.clear();
    response.data.clear();
}

void StorageConnectionHandler::read(const StoragePacket& request, StoragePacket& response)
{
    // TODO
    return;
}

void StorageConnectionHandler::write(const StoragePacket& request, StoragePacket& response)
{
    response.id = request.id;
    response.rescode = ResultCode::Type::SUCCESS;
    response.data_len = 0;
    response.message_len = 0;
    response.message.clear();
    response.data.clear();

    SPDLOG_DEBUG("Write data: {}", request.data.size());
    SPDLOG_DEBUG("Write offset: {}", request.offset);
}

StorageConnectionHandler::StorageConnectionHandler(asio::io_context& context)
    : GenericConnectionHandler<StoragePacket>::GenericConnectionHandler(context) {}
