#include "cache_protocol.hpp"

/*####################################*/
/*---------[ ResultCode ]---------*/
/*####################################*/

uint8_t ResultCode::to_byte(ResultCode::Type rescode)
{
    switch (rescode)
    {
        case ResultCode::Type::SUCCESS:
            return 0;
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
        default:
            return "UNKNOWN";
    }
}

/*#######################################*/
/*---------[ OperationCode ]---------*/
/*#######################################*/


uint8_t OperationCode::to_byte(OperationCode::Type rescode)
{
    switch (rescode)
    {
        case OperationCode::Type::NOP:
            return 0;
        case OperationCode::Type::GET:
            return 1;
        case OperationCode::Type::SET:
            return 2;
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
        default:
            return OperationCode::Type::UNKNOWN;
    }
}

std::string OperationCode::to_string(OperationCode::Type rescode)
{
    switch (rescode)
    {
        case OperationCode::Type::NOP:
            return "NOP";
        case OperationCode::Type::GET:
            return "GET";
        case OperationCode::Type::SET:
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

const uint8_t* BytePacketBuffer::get_buffer() const
{
    return buffer.data();
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

CachePacket::CachePacket()
{
    id = 0;
    opcode = OperationCode::Type::NOP;
    rescode = ResultCode::Type::SUCCESS;

    flags = 0;

    time = 0;
    key_len = 0;
    value_len = 0;

    key = std::vector<uint8_t>();
    value = std::vector<uint8_t>();
}

void CachePacket::from_buffer(const uint8_t* buffer, size_t len)
{
    BytePacketBuffer packetBuffer = BytePacketBuffer(buffer, len);

    try {
        id = packetBuffer.read_u16();
        opcode = OperationCode::from_byte(packetBuffer.read_u8());
        rescode = ResultCode::from_byte(packetBuffer.read_u8());

        flags = packetBuffer.read_u8();
        // packetBuffer.read_u8(); // padding 8
        // packetBuffer.read_u16(); // padding 16
        packetBuffer.step(3); // stepping 3 bytes (padding)

        time = packetBuffer.read_u32();
        key_len = packetBuffer.read_u32();
        value_len = packetBuffer.read_u32();

        key.resize(key_len);
        for (int i = 0; i < key_len; i ++)
            key[i] = packetBuffer.read_u8();

        value.resize(value_len);
        for (int i = 0; i < value_len; i ++)
            value[i] = packetBuffer.read_u8();
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(std::format("from_buffer: {}", e.what()));
    }
}

size_t CachePacket::to_buffer(BytePacketBuffer& packetBuffer)
{
    size_t bytes_returned = 20 + key_len + value_len; // header length + data length 
    packetBuffer.resize(bytes_returned);

    packetBuffer.write_u16(id);
    packetBuffer.write_u8(OperationCode::to_byte(opcode));
    packetBuffer.write_u8(ResultCode::to_byte(rescode));
    
    packetBuffer.write_u8(flags);
    // packetBuffer.write_u8(0); // padding 8
    // packetBuffer.write_u16(0); // padding 16
    packetBuffer.step(3); // skipping 3 bytes (padding) 

    packetBuffer.write_u32(time);
    packetBuffer.write_u32(key_len);
    packetBuffer.write_u32(value_len);

    for (int i = 0; i < key_len; i ++)
        packetBuffer.write_u8(key[i]);
    
    for (int i = 0; i < value_len; i ++)
        packetBuffer.write_u8(key[i]);

    return packetBuffer.get_size();
}

std::string CachePacket::to_string() const
{
    std::string result = "CachePacket:\n";
    result += "--\\ id: " + std::to_string(id) + "\n";
    result += "--\\ opcode: " + OperationCode::to_string(opcode) + "\n";
    result += "--\\ rescode: " + ResultCode::to_string(rescode) + "\n";
    result += "--\\ flags: " + std::to_string(flags) + "\n";
    result += "--\\ time: " + std::to_string(time) + "\n";
    result += "--\\ key_len: " + std::to_string(key_len) + "\n";
    result += "--\\ value_len: " + std::to_string(value_len) + "\n\n";
    result += "--\\ Key:\n";
    result += get_string_from_byte_array(key);
    result += "\n\n";
    result += "--\\ Value:\n";
    result += get_string_from_byte_array(value);
    result += "\n";

    return result;
}


/*######################################*/
/*---------[ Helper Functions ]---------*/
/*######################################*/

// From: https://github.com/VladSteopoaie/DNS-tunneling/blob/main/dns_server/modules/dns_module.cpp

// Conversion funtions
std::vector<uint8_t> get_byte_array_from_string(std::string string)
{
    std::vector<uint8_t> byte_array = std::vector<uint8_t>(0);
    for (int i = 0; i < string.size(); i ++)
        byte_array.push_back(string[i]);
    return byte_array;
}

std::string get_string_from_byte_array(std::vector<uint8_t> byte_array)
{
    std::string result(byte_array.size(), '\0');

    for (int i = 0; i < byte_array.size(); i ++){
        result[i] = byte_array[i];
    }
    return result;
}