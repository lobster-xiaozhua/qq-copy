#ifndef PROTOCOL_CODEC_HPP
#define PROTOCOL_CODEC_HPP

#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace qqchat {

enum class CommandCode : uint16_t {
    LOGIN_REQ = 0x0001,
    LOGIN_RESP = 0x0002,
    MESSAGE_REQ = 0x0003,
    MESSAGE_RESP = 0x0004,
    MESSAGE_NOTIFY = 0x0005,
    FRIEND_LIST_REQ = 0x0006,
    FRIEND_LIST_RESP = 0x0007,
    ADD_FRIEND_REQ = 0x0008,
    ADD_FRIEND_RESP = 0x0009
};

enum class ErrorCode : uint16_t {
    SUCCESS = 0x0000,
    USER_NOT_FOUND = 0x0001,
    WRONG_PASSWORD = 0x0002,
    USER_ALREADY_ONLINE = 0x0003,
    USER_OFFLINE = 0x0004,
    PROTOCOL_ERROR = 0x0005,
    DATABASE_ERROR = 0x0006,
    FRIEND_REQUEST_EXISTS = 0x0007,
    CANNOT_ADD_SELF = 0x0008
};

struct LoginRequest {
    std::string username;
    std::string password;
};

struct LoginResponse {
    ErrorCode error_code;
    int user_id;
    std::string username;
};

struct MessageRequest {
    int to_id;
    std::string content;
};

struct MessageResponse {
    ErrorCode error_code;
    int msg_id;
};

struct MessageNotify {
    int from_id;
    std::string content;
    int64_t timestamp;
};

struct FriendListRequest {
    int user_id;
};

struct FriendListResponse {
    ErrorCode error_code;
    std::vector<std::pair<int, std::string>> friends;
};

struct AddFriendRequest {
    int user_id;
    int friend_id;
};

struct AddFriendResponse {
    ErrorCode error_code;
};

struct Packet {
    uint8_t version;
    CommandCode command;
    std::vector<uint8_t> data;
};

class ProtocolCodec {
public:
    static const uint8_t VERSION = 1;
    static const size_t HEADER_SIZE = 7;
    
    static std::vector<uint8_t> encode(const Packet& packet);
    static bool decode(const std::vector<uint8_t>& data, Packet& packet);
    
    static bool decode_login_request(const std::vector<uint8_t>& data, LoginRequest& req);
    static std::vector<uint8_t> encode_login_response(const LoginResponse& resp);
    
    static bool decode_message_request(const std::vector<uint8_t>& data, MessageRequest& req);
    static std::vector<uint8_t> encode_message_response(const MessageResponse& resp);
    
    static std::vector<uint8_t> encode_message_notify(const MessageNotify& notify);
    static bool decode_message_notify(const std::vector<uint8_t>& data, MessageNotify& notify);
    
    static std::vector<uint8_t> encode_friend_list_response(const FriendListResponse& resp);
    static std::vector<uint8_t> encode_add_friend_response(const AddFriendResponse& resp);
    
private:
    static void write_uint16(std::vector<uint8_t>& buffer, uint16_t value);
    static void write_uint32(std::vector<uint8_t>& buffer, uint32_t value);
    static void write_int32(std::vector<uint8_t>& buffer, int32_t value);
    static void write_int64(std::vector<uint8_t>& buffer, int64_t value);
    static void write_string(std::vector<uint8_t>& buffer, const std::string& str);
    
    static uint16_t read_uint16(const uint8_t* data);
    static uint32_t read_uint32(const uint8_t* data);
    static int32_t read_int32(const uint8_t* data);
    static int64_t read_int64(const uint8_t* data);
    static std::string read_string(const uint8_t* data, size_t len);
};

} // namespace qqchat

#endif // PROTOCOL_CODEC_HPP
