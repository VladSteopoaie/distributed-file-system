#include "cache_protocol.hpp"

/*####################################*/
/*---------[ ResultCodeFunc ]---------*/
/*####################################*/

uint8_t ResultCodeFunc::to_byte(ResultCode rescode)
{
    switch (rescode)
    {
        case ResultCode::SUCCESS:
            return 0;
        default:
            return -1;
    }
}

ResultCode ResultCodeFunc::from_byte(uint8_t byte)
{
    switch (byte)
    {
        case 0:
            return ResultCode::SUCCESS;
        default:
            return ResultCode::UNKNOWN;
    }
}

std::string ResultCodeFunc::to_string(ResultCode rescode)
{
    switch (rescode)
    {
        case ResultCode::SUCCESS:
            return "SUCCESS";
        default:
            return "UNKNOWN";
    }
}

/*#######################################*/
/*---------[ OperationCodeFunc ]---------*/
/*#######################################*/


uint8_t OperationCodeFunc::to_byte(ResultCode rescode)
{
    switch (rescode)
    {
        case OperationCode::NOP:
            return 0;
        case OperationCode::GET:
            return 1;
        case OperationCode::SET:
            return 2;
        default:
            return -1;
    }
}

ResultCode OperationCodeFunc::from_byte(uint8_t byte)
{
    switch (byte)
    {
        case 0:
            return OperationCode::NOP;
        case 1:
            return OperationCode::GET;
        case 2:
            return OperationCode::SET;
        default:
            return ResultCode::UNKNOWN;
    }
}

std::string OperationCodeFunc::to_string(ResultCode rescode)
{
    switch (rescode)
    {
        case OperationCode::NOP:
            return "NOP";
        case OperationCode::GET:
            return "GET";
        case OperationCode::SET:
            return "SET";
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

BytePacketBuffer::BytePacketBuffer(char* new_buf, size_t len)
{
    c_pos = 0;            
    buffer = std::vector<uint8_t>(len);

    for (size_t i = 0; i < len; i ++)
    {
        buffer[i] = new_buf[i];
    }
}

// current position within buffer
size_t BytePacketBuffer::get_possition()
{
    return c_pos;
}

// move the position over a specific number of bytes
// void BytePacketBuffer::step(size_t steps)
// {
//     c_pos += steps;
// }

// change current position
// void BytePacketBuffer::seek(size_t pos)
// {
//     c_pos = pos;
// }

// get the current byte from buffer's position without changing the position
uint8_t BytePacketBuffer::get_byte(size_t pos)
{
    if (pos < buffer.size())
        return buffer[pos];
    
    throw std::runtime_error(std::format("get_byte: Index outside of bounds: tried {}, but size is {}", pos, buffer.size()))
}

// get a range of bytes from buffer without changing the position
uint8_t* BytePacketBuffer::get_range(size_t start, size_t len)
{
    if (start + len >= buffer.size())
        throw std::runtime_error(
            std::format("get_range: Index outside of bounds: tried {} and {}, but size is {}", start, len, buffer.size())
        );

    uint8_t res[len + 1];
    std::copy(buffer.begin() + start, buffer.begin() + start + len, res);
    res[len] = 0;

    return res;
}

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
        buffer.push_back(val);
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

CachePacket::CachePacket()
{
    opcode = OperationCode::NOP;
    rescode = ResultCode::SUCCESS;
    flags = 0;
    time = 0;
    key_len = 0;
    value_len = 0;

    key = std::vector<uint8_t>();
    value = std::vector<uint8_t>();
}