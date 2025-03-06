#ifndef CACHE_PROTOCOL_HPP
#define CACHE_PROTOCOL_HPP

#include "utils.hpp"

namespace ResultCode {
    enum Type {
        UNKNOWN, // type unkown
        SUCCESS = 0, // no problems here :)
        INVPKT = 1, // invalid packet error
        INVOP = 2, // invalid operation
        NOLOCAL = 3, // no local memcached servers
        ERRMSG = 4, // error with a message
    };

    uint8_t to_byte(Type rescode);
    Type from_byte(uint8_t byte);
    std::string to_string(Type rescode);
}

namespace OperationCode {
    enum Type {
        UNKNOWN,
        NOP = 0,
        INIT = 1, // this command shoud be called by a client who wants to initialize a connection with the server
                  // the server will send back some information
        GET_FILE = 2,
        GET_DIR = 3,
        SET_FILE = 4,
        SET_DIR = 5,
        RM_FILE = 6,
        RM_DIR = 7,
        UPDATE = 8
    };

    uint8_t to_byte(Type opcode);
    Type from_byte(uint8_t byte);
    std::string to_string(Type opcode);
}

namespace UpdateCode {
    enum Type {
        UNKNOWN,
        CHMOD = 1,
        CHOWN = 2,
        RENAME = 3,
    };

    uint8_t to_byte(Type opcode);
    Type from_byte(uint8_t byte);
    std::string to_string(Type opcode);
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
    std::vector<uint8_t> get_buffer() const;

    size_t get_position() const;

    // move the position over a specific number of bytes
    void step(size_t steps);

    // change current position
    void seek(size_t pos);

    // get the current byte from buffer's position without changing the position
    uint8_t get_byte(size_t pos) const;

    // get a range of bytes from buf without changing the position
    uint8_t* get_range(size_t start, size_t len, uint8_t* res);

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

struct CachePacket {
    static const size_t max_packet_size;
    // header
    uint16_t id;
    uint8_t opcode;
    uint8_t rescode;

    uint8_t flags;
    uint16_t message_len; // messages for additional information
    uint8_t pad8; // padding (some space for future add-ons)
    
    uint32_t time;
    uint32_t key_len;
    uint32_t value_len;

    // body
    std::vector<uint8_t> message;
    std::vector<uint8_t> key;
    std::vector<uint8_t> value;

    CachePacket();
    CachePacket(const uint8_t* buffer, size_t len);

    void from_buffer(const uint8_t* buffer, size_t len);
    size_t to_buffer(std::vector<uint8_t>& buffer) const;
    std::string to_string() const;
};

struct UpdateCommand {
    uint8_t opcode;
    uint8_t argc;
    std::vector<std::vector<uint8_t>> argv; // vector of strings basically

    UpdateCommand();
    UpdateCommand(const uint8_t* buffer, size_t len);

    void from_buffer(const uint8_t* buffer, size_t len);
    size_t to_buffer(std::vector<uint8_t>& buffer) const;
    std::string to_string() const;
};

#endif