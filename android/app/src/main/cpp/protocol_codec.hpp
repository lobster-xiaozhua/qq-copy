#ifndef PROTOCOL_CODEC_HPP
#define PROTOCOL_CODEC_HPP

#include <vector>
#include <string>
#include <cstdint>

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

struct LoginResponse {
    ErrorCode error_code;
    int user_id;
    std::string username;
};

struct MessageNotify {
    int from_id;
    std::string content;
    int64_t timestamp;
};

struct FriendInfo {
    int friend_id;
    std::string friend_name;
};

class ProtocolCodec {
public:
    static constexpr uint8_t VERSION = 1;
    static constexpr size_t HEADER_SIZE = 7;
    
    static std::vector<uint8_t> encode(CommandCode command, const std::vector<uint8_t>& data);
    static bool decode(const std::vector<uint8_t>& buffer, CommandCode& command, std::vector<uint8_t>& data);
    
    static std::vector<uint8_t> encode_login_request(const std::string& username, const std::string& password);
    static bool decode_login_response(const std::vector<uint8_t>& data, LoginResponse& resp);
    
    static std::vector<uint8_t> encode_message_request(int to_id, const std::string& content);
    static bool decode_message_notify(const std::vector<uint8_t>& data, MessageNotify& notify);
    
    static std::vector<uint8_t> encode_friend_list_request();
    static bool decode_friend_list_response(const std::vector<uint8_t>& data, std::vector<FriendInfo>& friends);
    
    static std::vector<uint8_t> encode_add_friend_request(int friend_id);
    
private:
    static void write_uint16(std::vector<uint8_t>& buffer, uint16_t value);
    static void write_uint32(std::vector<uint8_t>& buffer, uint32_t value);
    static void write_int32(std::vector<uint8_t>& buffer, int32_t value);
    static void write_string(std::vector<uint8_t>& buffer, const std::string& str);
    
    static uint16_t read_uint16(const uint8_t* data);
    static uint32_t read_uint32(const uint8_t* data);
    static int32_t read_int32(const uint8_t* data);
    static int64_t read_int64(const uint8_t* data);
    static std::string read_string(const uint8_t* data, size_t len);
};

} // namespace qqchat

#endif // PROTOCOL_CODEC_HPP
