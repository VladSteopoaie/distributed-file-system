#include "cache_protocol.hpp"

/*################################*/
/*---------[ ResultCode ]---------*/
/*################################*/

uint8_t ResultCode::to_byte(Type rescode)
{
    switch (rescode)
    {
        case Type::SUCCESS:
            return 0;
        case Type::INVPKT:
            return 1;
        case Type::INVOP:
            return 2;
        case Type::NOLOCAL:
            return 3;
        case Type::ERRMSG:
            return 4;
        default:
            return -1;
    }
}

ResultCode::Type ResultCode::from_byte(uint8_t byte)
{
    switch (byte)
    {
        case 0:
            return Type::SUCCESS;
        case 1:
            return Type::INVPKT;
        case 2:
            return Type::INVOP;
        case 3:
            return Type::NOLOCAL;
        case 4:
            return Type::ERRMSG;
        default:
            return Type::UNKNOWN;
    }
}

std::string ResultCode::to_string(Type rescode)
{
    switch (rescode)
    {
        case Type::SUCCESS:
            return "SUCCESS";
        case Type::INVPKT:
            return "INVPKT";
        case Type::INVOP:
            return "INVOP";
        case Type::NOLOCAL:
            return "NOLOCAL";
        case Type::ERRMSG:
            return "ERRMSG";
        default:
            return "UNKNOWN";
    }
}

/*###################################*/
/*---------[ OperationCode ]---------*/
/*###################################*/


uint8_t OperationCode::to_byte(OperationCode::Type opcode)
{
    switch (opcode)
    {
        case Type::NOP:
            return 0;
        case Type::INIT:
            return 1;
        case Type::GET_FILE:
            return 2;
        case Type::GET_DIR:
            return 3;
        case Type::SET_FILE:
            return 4;
        case Type::SET_DIR:
            return 5;
        case Type::RM_FILE:
            return 6;
        case Type::RM_DIR:
            return 7;
        case Type::UPDATE:
            return 8;
        default:
            return -1;
    }
}

OperationCode::Type OperationCode::from_byte(uint8_t byte)
{
    switch (byte)
    {
        case 0:
            return Type::NOP;
        case 1:
            return Type::INIT;
        case 2:
            return Type::GET_FILE;
        case 3:
            return Type::GET_DIR;
        case 4:
            return Type::SET_FILE;
        case 5:
            return Type::SET_DIR;
        case 6:
            return Type::RM_FILE;
        case 7:
            return Type::RM_DIR;
        case 8:
            return Type::UPDATE;
        default:
            return Type::UNKNOWN;
    }
}

std::string OperationCode::to_string(OperationCode::Type opcode)
{
    switch (opcode)
    {
        case Type::NOP:
            return "NOP";
        case Type::INIT:
            return "INIT";
        case Type::GET_FILE:
            return "GET_FILE";
        case Type::GET_DIR:
            return "GET_DIR";
        case Type::SET_FILE:
            return "SET_FILE";
        case Type::SET_DIR:
            return "SET_DIR";
        case Type::RM_FILE:
            return "RM_FILE";
        case Type::RM_DIR:
            return "RM_DIR";
        case Type::UPDATE:
            return "UPDATE";
        default:
            return "UNKNOWN";
    }
}
/*################################*/
/*---------[ UpdateCode ]---------*/
/*################################*/

uint8_t UpdateCode::to_byte(Type opcode)
{
    switch (opcode)
    {
        case Type::CHMOD:
            return 1;
        case Type::CHOWN:
            return 2;
        case Type::RENAME:
            return 3;
        
        default:
            return -1;
    }
}

UpdateCode::Type UpdateCode::from_byte(uint8_t byte)
{
    switch (byte)
    {
        case 1:
            return Type::CHMOD;
        case 2:
            return Type::CHOWN;
        case 3:
            return Type::RENAME;
        
        default:
            return Type::UNKNOWN;
    }
}

std::string UpdateCode::to_string(UpdateCode::Type opcode)
{
    switch (opcode)
    {
        case Type::CHMOD:
            return "CHMOD";
        case Type::CHOWN:
            return "CHOWN";
        case Type::RENAME:
            return "RENAME";

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
size_t BytePacketBuffer::get_position() const
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
uint8_t* BytePacketBuffer::get_range(size_t start, size_t len, uint8_t* res)
{
    if (start + len > buffer.size())
        throw std::runtime_error(
            std::format("get_range: Index outside of bounds: tried {} and {}, but size is {}", start, len, buffer.size())
        );

    if (len > sizeof(res))
        throw std::runtime_error(
            std::format("get_range: Buffer too small. Length given: {}, buffer size: {}", len, sizeof(res))
        );

    std::copy(buffer.begin() + start, buffer.begin() + start + len, res);
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
    packet_buffer.step(1); // skipping 1 byte (padding) 

    packet_buffer.write_u32(time);
    packet_buffer.write_u32(key_len);
    packet_buffer.write_u32(value_len);

    for (int i = 0; i < message_len; i ++)
        packet_buffer.write_u8(message[i]);
    
    for (int i = 0; i < key_len; i ++)
        packet_buffer.write_u8(key[i]);
    
    for (int i = 0; i < value_len; i ++)
        packet_buffer.write_u8(value[i]);

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
    result += "--\\ time: " + std::to_string(time) + "\n";
    result += "--\\ message_len: " + std::to_string(message_len) + "\n";
    result += "--\\ key_len: " + std::to_string(key_len) + "\n";
    result += "--\\ value_len: " + std::to_string(value_len) + "\n\n";
    result += "--\\ Message:\n";
    result += Utils::get_string_from_byte_array(message);
    result += "\n";
    result += "--\\ Key:\n";
    result += Utils::get_string_from_byte_array(key);
    result += "\n";
    result += "--\\ Value:\n";
    result += Utils::get_string_from_byte_array(value);
    result += "\n";

    return result;
}

/*###################################*/
/*---------[ UpdateCommand ]---------*/
/*###################################*/

UpdateCommand::UpdateCommand() {
    opcode = 0;
    argc = 0;
    argv = std::vector<std::vector<uint8_t>>();
}

UpdateCommand::UpdateCommand(const uint8_t* buffer, size_t len)
{
    from_buffer(buffer, len);
}

void UpdateCommand::from_buffer(const uint8_t* buffer, size_t len)
{
    BytePacketBuffer command_buffer = BytePacketBuffer(buffer, len);

    opcode = command_buffer.read_u8();
    argc = command_buffer.read_u8();

    // for each argument the length of it is the first byte
    for (uint8_t i = 0; i < argc; i ++)
    {
        uint8_t arg_size = command_buffer.read_u8();
        uint8_t range[arg_size];
        command_buffer.get_range(command_buffer.get_position(), arg_size, range); 
        argv.push_back(
            std::vector<uint8_t>(range, range + arg_size)
        );
        command_buffer.step(arg_size);
    }
}

size_t UpdateCommand::to_buffer(std::vector<uint8_t>& final_buffer) const 
{
    BytePacketBuffer command_buffer = BytePacketBuffer();
    int command_size = 2 + argv.size(); // argc + opcode + each len byte at the start of each argv[i]
    for (auto arg : argv)
        command_size += arg.size();

    command_buffer.resize(command_size);

    command_buffer.write_u8(opcode);
    command_buffer.write_u8(argv.size());

    for (std::vector<uint8_t> arg : argv) 
    {
        command_buffer.write_u8(static_cast<uint8_t> (arg.size()));
        for (size_t i = 0; i < arg.size(); i ++)
            command_buffer.write_u8(arg[i]);
    }

    final_buffer = command_buffer.get_buffer();
    return command_size;
}

std::string UpdateCommand::to_string() const 
{
    std::string result = "UpdateCommand:\n";
    result += "--\\ opcode: " + UpdateCode::to_string(UpdateCode::from_byte(opcode)) + "\n";
    result += "--\\ argc: " + std::to_string(argc) +  "\n";

    for (uint8_t i = 0; i < argc; i ++)
    {
        result += "----\\ argv[" + std::to_string(i) + "] = " + Utils::get_string_from_byte_array(argv[i]) + "\n";
    }
    result += "\n";
    return result;
}