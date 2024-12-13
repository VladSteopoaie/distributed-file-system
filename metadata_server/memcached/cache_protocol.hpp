#ifndef CACHE_PROTOCOL_HPP
#define CACHE_PROTOCOL_HPP

#include <cstdint>
#include <vector>

enum ResultCode {
    UNKNOWN,
    SUCCESS = 0
}

namespace ResultCodeFunc {
    uint8_t to_byte(ResultCode rescode);
    ResultCode from_byte(uint8_t byte);
    std::string to_string(ResultCode rescode);
}

enum OperationCode {
    UNKNOWN,
    NOP = 0,
    GET = 1,
    SET = 2
}

namespace OperationCodeFunc {
    uint8_t to_byte(OperationCode rescode);
    OperationCode from_byte(uint8_t byte);
    std::string to_string(OperationCode rescode);
}


// From: https://github.com/VladSteopoaie/DNS-tunneling/blob/main/dns_server/modules/dns_module.h

// A struct to easily read and write bytes into a buffer
class BytePacketBuffer{
private:
    std::vector<uint8_t> buffer; // raw buffer to store all the bytes
    size_t c_pos; // current position within the buffer

public:
    BytePacketBuffer();
    BytePacketBuffer(uint8_t* new_buf, size_t len);

    size_t get_possition();

    // move the position over a specific number of bytes
    // void step(size_t steps);

    // change current position
    // void seek(size_t pos);

    // get the current byte from buffer's position without changing the position
    uint8_t get_byte(size_t pos);

    // get a range of bytes from buf without changing the position
    uint8_t* get_range(size_t start, size_t len);

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
    OperationCode opcode;
    ResultCode rescode;
    uint8_t flags;
    uint8_t pad; // padding (some space for future add-ons)
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
    static CachePacket from_buffer(uint8_t* buffer, size_t length);
    void write(BytePacketBuffer buffer);
    std::string to_stirng() const;
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
#endif