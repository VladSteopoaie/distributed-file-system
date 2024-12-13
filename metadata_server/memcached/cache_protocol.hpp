#ifndef CACHE_PROTOCOL_HPP
#define CACHE_PROTOCOL_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <ctime>
#include <stdexcept>
#include <format>


namespace ResultCode {
    enum Type {
        UNKNOWN,
        SUCCESS = 0
    };

    uint8_t to_byte(Type rescode);
    Type from_byte(uint8_t byte);
    std::string to_string(Type rescode);
}

namespace OperationCode {
    enum Type {
        UNKNOWN,
        NOP = 0,
        GET = 1,
        SET = 2
    };

    uint8_t to_byte(Type rescode);
    Type from_byte(uint8_t byte);
    std::string to_string(Type rescode);
}


// From: https://github.com/VladSteopoaie/DNS-tunneling/blob/main/dns_server/modules/dns_module.h

// A struct to easily read and write bytes into a buffer
class BytePacketBuffer{
private:
    std::vector<uint8_t> buffer; // raw buffer to store all the bytes
    size_t c_pos; // current position within the buffer

public:
    BytePacketBuffer();
    BytePacketBuffer(const uint8_t* new_buf, size_t len);

    // resizes the buffer and moves c_pos at the beggining  
    void resize(size_t len);
    size_t get_size() const;
    const uint8_t* get_buffer() const;

    size_t get_possition() const;

    // move the position over a specific number of bytes
    void step(size_t steps);

    // change current position
    void seek(size_t pos);

    // get the current byte from buffer's position without changing the position
    uint8_t get_byte(size_t pos) const;

    // get a range of bytes from buf without changing the position
    // uint8_t* get_range(size_t start, size_t len);

    // read one byte from the buffer's position and change the position
    uint8_t read_u8();
    uint16_t read_u16(); // 2 bytes
    uint32_t read_u32();

    void write_u8(uint8_t val);
    void write_u16(uint16_t val);
    void write_u32(uint32_t val);

    // set a byte to have the value of val without changing the possition
    void set_u8(size_t pos, uint8_t val);
    void set_u16(size_t pos, uint16_t val);
};

class CachePacket {
private:
    // header
    uint16_t id;
    OperationCode::Type opcode; // 1 byte
    ResultCode::Type rescode; // 1 byte

    uint8_t flags;
    uint8_t pad8; // padding (some space for future add-ons)
    uint16_t pad16;
    
    uint32_t time;
    uint32_t key_len;
    uint32_t value_len;
    // CacheHeader header;    

    // body
    std::vector<uint8_t> key;
    std::vector<uint8_t> value;
public:
    CachePacket();

    // static CachePacket read(BytePacketBuffer &buffer);
    void from_buffer(const uint8_t* buffer, size_t len);
    size_t to_buffer(BytePacketBuffer& packetBuffer);
    std::string to_string() const;
};




// class CacheHeader {
// private:
//     OperationCode opcode;
//     ResultCode rescode;
//     uint8_t flags;
//     uint8_t pad; // padding (some space for future add-ons)
//     uint32_t time;
//     uint32_t key_len;
//     uint32_t value_len;
// public:
//     CacheHeader();

//     // CacheHeader read(BytePacketBuffer &buffer);
//     void write(BytePacketBuffer buffer);
//     std::string to_stirng() const;
// }


// Some additional helper functions

// From: https://github.com/VladSteopoaie/DNS-tunneling/blob/main/dns_server/modules/dns_module.h

std::vector<uint8_t> get_byte_array_from_string(std::string string); 
std::string get_string_from_byte_array(std::vector<uint8_t> byte_array);

#endif