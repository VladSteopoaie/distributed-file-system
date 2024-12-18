#include "cache_protocol.hpp"

/*################################*/
/*---------[ ResultCode ]---------*/
/*################################*/

uint8_t ResultCode::to_byte(ResultCode::Type rescode)
{
    switch (rescode)
    {
        case ResultCode::Type::SUCCESS:
            return 0;
        case ResultCode::Type::INVPKT:
            return 1;
        case ResultCode::Type::INVOP:
            return 2;
        case ResultCode::Type::NOLOCAL:
            return 3;
        default:
            return -1;
    }
}

ResultCode::Type ResultCode::from_byte(uint8_t byte)
{
    switch (byte)
    {
        case 0:
            return ResultCode::Type::SUCCESS;
        case 1:
            return ResultCode::Type::INVPKT;
        case 2:
            return ResultCode::Type::INVOP;
        case 3:
            return ResultCode::Type::NOLOCAL;
        default:
            return ResultCode::Type::UNKNOWN;
    }
}

std::string ResultCode::to_string(ResultCode::Type rescode)
{
    switch (rescode)
    {
        case ResultCode::Type::SUCCESS:
            return "SUCCESS";
        case ResultCode::Type::INVPKT:
            return "INVPKT";
        case ResultCode::Type::INVOP:
            return "INVOP";
        case ResultCode::Type::NOLOCAL:
            return "NOLOCAL";
        default:
            return "UNKNOWN";
    }
}

/*#######################################*/
/*---------[ OperationCode ]---------*/
/*#######################################*/


uint8_t OperationCode::to_byte(OperationCode::Type opcode)
{
    switch (opcode)
    {
        case OperationCode::Type::NOP:
            return 0;
        case OperationCode::Type::GET:
            return 1;
        case OperationCode::Type::SET:
            return 2;
        case OperationCode::Type::INIT:
            return 3;
        default:
            return -1;
    }
}

OperationCode::Type OperationCode::from_byte(uint8_t byte)
{
    switch (byte)
    {
        case 0:
            return OperationCode::Type::NOP;
        case 1:
            return OperationCode::Type::GET;
        case 2:
            return OperationCode::Type::SET;
        case 3:
            return OperationCode::Type::INIT;
        default:
            return OperationCode::Type::UNKNOWN;
    }
}

std::string OperationCode::to_string(OperationCode::Type opcode)
{
    switch (opcode)
    {
        case OperationCode::Type::NOP:
            return "NOP";
        case OperationCode::Type::GET:
            return "GET";
        case OperationCode::Type::SET:
            return "SET";
        case OperationCode::Type::INIT:
            return "INIT";
        default:
            return "UNKNOWN";
    }
}

/*######################################*/
/*---------[ BytePacketBuffer ]---------*/
/*######################################*/

// From: https://github.com/VladSteopoaie/DNS-tunneling/blob/main/dns_server/modules/dns_module.cpp

BytePacketBuffer::BytePacketBuffer()
{
    buffer = std::vector<uint8_t>();
    c_pos = 0;            
}

BytePacketBuffer::BytePacketBuffer(const uint8_t* new_buf, size_t len)
{
    c_pos = 0;            
    buffer = std::vector<uint8_t>(len);

    for (size_t i = 0; i < len; i ++)
    {
        buffer[i] = new_buf[i];
    }
}

void BytePacketBuffer::resize(size_t len)
{
    buffer.resize(len);
    c_pos = 0;
}

size_t BytePacketBuffer::get_size() const
{
    return buffer.size();
}

std::vector<uint8_t> BytePacketBuffer::get_buffer() const
{
    return buffer;
}

// current position within buffer
size_t BytePacketBuffer::get_possition() const
{
    return c_pos;
}

// move the position over a specific number of bytes
void BytePacketBuffer::step(size_t steps)
{
    c_pos += steps;
}

// change current position
void BytePacketBuffer::seek(size_t pos)
{
    c_pos = pos;
}

// get the current byte from buffer's position without changing the position
uint8_t BytePacketBuffer::get_byte(size_t pos) const
{
    if (pos < buffer.size())
        return buffer[pos];
    
    throw std::runtime_error(std::format("get_byte: Index outside of bounds: tried {}, but size is {}", pos, buffer.size()));
}

// get a range of bytes from buffer without changing the position
// uint8_t* BytePacketBuffer::get_range(size_t start, size_t len)
// {
//     if (start + len >= buffer.size())
//         throw std::runtime_error(
//             std::format("get_range: Index outside of bounds: tried {} and {}, but size is {}", start, len, buffer.size())
//         );

//     uint8_t res[len + 1];
//     std::copy(buffer.begin() + start, buffer.begin() + start + len, res);
//     res[len] = 0;

//     return res;
// }

// read one byte from the buffer's position and change the position
uint8_t BytePacketBuffer::read_u8()
{
    if (c_pos >= buffer.size())
        throw std::runtime_error(std::format("read_u8: Index outside of bounds: tried {}, but size is {}", c_pos, buffer.size()));
    
    return buffer[c_pos++];
}

// reads 2 bytes from buffer
uint16_t BytePacketBuffer::read_u16()
{
    uint16_t rez; 
    try {
        rez = (uint16_t) read_u8() << 8;
        rez += (uint16_t) read_u8();
    } catch (std::runtime_error e) {
        throw std::runtime_error(std::format("read_u16: {}", e.what()));
    }

    return rez;
}

// reads 4 bytes from buffer
uint32_t BytePacketBuffer::read_u32()
{
    uint32_t rez;
    try{
        rez = (uint32_t) read_u16() << 16;
        rez += (uint32_t) read_u16();
    } catch (std::runtime_error e)
    {
        throw std::runtime_error(std::format("read_u32: {}", e.what()));
    }
    
    return rez;
}

// writing 1 byte into the buffer
void BytePacketBuffer::write_u8(uint8_t val)
{
    if (c_pos < buffer.size())
        buffer[c_pos] = val;
    else 
    {
        c_pos = buffer.size();
        buffer.push_back(val);
    }
    c_pos += 1;
}

// writing 2 bytes into the buffer
void BytePacketBuffer::write_u16(uint16_t val)
{
    write_u8((uint8_t) (val >> 8));
    write_u8((uint8_t) (val & 0xFF));
}

// writing 4 bytes into the buffer
void BytePacketBuffer::write_u32(uint32_t val)
{
    write_u8((uint8_t) (val >> 24 & 0xFF));
    write_u8((uint8_t) (val >> 16 & 0xFF));
    write_u8((uint8_t) (val >> 8 & 0xFF));
    write_u8((uint8_t) (val >> 0 & 0xFF));
}

// set a byte to have the value of val without changing the possition
void BytePacketBuffer::set_u8(size_t pos, uint8_t val)
{
    if (pos >= buffer.size())
        throw std::runtime_error(std::format("set_u8: Index outside of bounds: tried {}, but size is {}", pos, buffer.size()));
    
    buffer[pos] = val;
}

// set 2 bytes to have the value of val without changing the possition
void BytePacketBuffer::set_u16(size_t pos, uint16_t val)
{
    try {
        set_u8(pos, (uint8_t) (val >> 8));
        set_u8(pos + 1, (uint8_t) (val & 0xFF));
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(std::format("set_u16: {}", e.what()));
    }
}

/*#################################*/
/*---------[ CachePacket ]---------*/
/*#################################*/
const size_t CachePacket::max_packet_size = 8192;

CachePacket::CachePacket()
{
    id = 0;
    opcode = 0; // NOP
    rescode = 0; // SUCCESS

    flags = 0;
    message_len = 0;

    time = 0;
    key_len = 0;
    value_len = 0;

    message = std::vector<uint8_t>();
    key = std::vector<uint8_t>();
    value = std::vector<uint8_t>();
}

CachePacket::CachePacket(const uint8_t* buffer, size_t len)
{
    from_buffer(buffer, len);
}

void CachePacket::from_buffer(const uint8_t* buffer, size_t len)
{
    BytePacketBuffer packet_buffer = BytePacketBuffer(buffer, len);

    try {
        id = packet_buffer.read_u16();
        opcode = packet_buffer.read_u8();
        rescode = packet_buffer.read_u8();

        flags = packet_buffer.read_u8();
        message_len = packet_buffer.read_u16();
        // packet_buffer.read_u8(); // padding 8
        packet_buffer.step(1); // stepping 3 bytes (padding)

        time = packet_buffer.read_u32();
        key_len = packet_buffer.read_u32();
        value_len = packet_buffer.read_u32();

        message.resize(message_len);
        for (int i = 0; i < message_len; i ++)
            message[i] = packet_buffer.read_u8();

        key.resize(key_len);
        for (int i = 0; i < key_len; i ++)
            key[i] = packet_buffer.read_u8();

        value.resize(value_len);
        for (int i = 0; i < value_len; i ++)
            value[i] = packet_buffer.read_u8();
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(std::format("from_buffer: {}", e.what()));
    }
}

size_t CachePacket::to_buffer(std::vector<uint8_t>& final_buffer) const
{
    BytePacketBuffer packet_buffer = BytePacketBuffer();
    
    size_t bytes_returned = 20 + key_len + value_len; // header length + data length 
    packet_buffer.resize(bytes_returned);
    final_buffer.resize(bytes_returned);

    packet_buffer.write_u16(id);
    packet_buffer.write_u8(opcode);
    packet_buffer.write_u8(rescode);
    
    packet_buffer.write_u8(flags);
    packet_buffer.write_u16(message_len);
    // packet_buffer.write_u8(0); // padding 8
    packet_buffer.step(1); // skipping 3 bytes (padding) 

    packet_buffer.write_u32(time);
    packet_buffer.write_u32(key_len);
    packet_buffer.write_u32(value_len);

    for (int i = 0; i < message_len; i ++)
        packet_buffer.write_u8(message[i]);
    
    for (int i = 0; i < key_len; i ++)
        packet_buffer.write_u8(key[i]);
    
    for (int i = 0; i < value_len; i ++)
        packet_buffer.write_u8(key[i]);

    final_buffer = packet_buffer.get_buffer();
    return packet_buffer.get_size();
}

std::string CachePacket::to_string() const
{
    std::string result = "CachePacket:\n";
    result += "--\\ id: " + std::to_string(id) + "\n";
    result += "--\\ opcode: " + OperationCode::to_string(OperationCode::from_byte(opcode)) + "\n";
    result += "--\\ rescode: " + ResultCode::to_string(ResultCode::from_byte(rescode)) + "\n";
    result += "--\\ flags: " + std::to_string(flags) + "\n";
    result += "--\\ message_len: " + std::to_string(message_len) + "\n";
    result += "--\\ time: " + std::to_string(time) + "\n";
    result += "--\\ key_len: " + std::to_string(key_len) + "\n";
    result += "--\\ value_len: " + std::to_string(value_len) + "\n\n";
    result += "--\\ Message:\n";
    result += Utils::get_string_from_byte_array(message);
    result += "\n\n";
    result += "--\\ Key:\n";
    result += Utils::get_string_from_byte_array(key);
    result += "\n\n";
    result += "--\\ Value:\n";
    result += Utils::get_string_from_byte_array(value);
    result += "\n";

    return result;
}
