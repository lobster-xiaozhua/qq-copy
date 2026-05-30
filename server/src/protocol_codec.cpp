#include "qqchat/protocol_codec.hpp"
#include <algorithm>

namespace qqchat {

void ProtocolCodec::write_uint16(std::vector<uint8_t>& buffer, uint16_t value) {
    buffer.push_back((value >> 8) & 0xFF);
    buffer.push_back(value & 0xFF);
}

void ProtocolCodec::write_uint32(std::vector<uint8_t>& buffer, uint32_t value) {
    buffer.push_back((value >> 24) & 0xFF);
    buffer.push_back((value >> 16) & 0xFF);
    buffer.push_back((value >> 8) & 0xFF);
    buffer.push_back(value & 0xFF);
}

void ProtocolCodec::write_int32(std::vector<uint8_t>& buffer, int32_t value) {
    write_uint32(buffer, static_cast<uint32_t>(value));
}

void ProtocolCodec::write_int64(std::vector<uint8_t>& buffer, int64_t value) {
    for (int i = 7; i >= 0; --i) {
        buffer.push_back((value >> (8 * i)) & 0xFF);
    }
}

void ProtocolCodec::write_string(std::vector<uint8_t>& buffer, const std::string& str) {
    uint16_t len = static_cast<uint16_t>(str.length());
    write_uint16(buffer, len);
    buffer.insert(buffer.end(), str.begin(), str.end());
}

uint16_t ProtocolCodec::read_uint16(const uint8_t* data) {
    return (static_cast<uint16_t>(data[0]) << 8) | data[1];
}

uint32_t ProtocolCodec::read_uint32(const uint8_t* data) {
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) |
           data[3];
}

int32_t ProtocolCodec::read_int32(const uint8_t* data) {
    return static_cast<int32_t>(read_uint32(data));
}

int64_t ProtocolCodec::read_int64(const uint8_t* data) {
    int64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value = (value << 8) | data[i];
    }
    return value;
}

std::string ProtocolCodec::read_string(const uint8_t* data, size_t len) {
    return std::string(reinterpret_cast<const char*>(data), len);
}

std::vector<uint8_t> ProtocolCodec::encode(const Packet& packet) {
    std::vector<uint8_t> buffer;
    buffer.push_back(packet.version);
    
    uint16_t command = static_cast<uint16_t>(packet.command);
    write_uint16(buffer, command);
    
    uint32_t length = static_cast<uint32_t>(packet.data.size());
    write_uint32(buffer, length);
    
    buffer.insert(buffer.end(), packet.data.begin(), packet.data.end());
    
    return buffer;
}

bool ProtocolCodec::decode(const std::vector<uint8_t>& data, Packet& packet) {
    if (data.size() < HEADER_SIZE) {
        return false;
    }
    
    packet.version = data[0];
    packet.command = static_cast<CommandCode>(read_uint16(&data[1]));
    uint32_t length = read_uint32(&data[3]);
    
    if (data.size() != HEADER_SIZE + length) {
        return false;
    }
    
    packet.data.resize(length);
    std::copy(data.begin() + HEADER_SIZE, data.end(), packet.data.begin());
    
    return true;
}

bool ProtocolCodec::decode_login_request(const std::vector<uint8_t>& data, LoginRequest& req) {
    if (data.size() < 4) return false;
    
    size_t offset = 0;
    uint16_t username_len = read_uint16(&data[offset]);
    offset += 2;
    
    if (offset + username_len > data.size()) return false;
    req.username = read_string(&data[offset], username_len);
    offset += username_len;
    
    if (offset + 2 > data.size()) return false;
    uint16_t password_len = read_uint16(&data[offset]);
    offset += 2;
    
    if (offset + password_len > data.size()) return false;
    req.password = read_string(&data[offset], password_len);
    
    return true;
}

std::vector<uint8_t> ProtocolCodec::encode_login_response(const LoginResponse& resp) {
    std::vector<uint8_t> buffer;
    write_uint16(buffer, static_cast<uint16_t>(resp.error_code));
    write_int32(buffer, resp.user_id);
    write_string(buffer, resp.username);
    return buffer;
}

bool ProtocolCodec::decode_message_request(const std::vector<uint8_t>& data, MessageRequest& req) {
    if (data.size() < 8) return false;
    
    req.to_id = read_int32(&data[0]);
    
    uint32_t content_len = read_uint32(&data[4]);
    if (4 + 4 + content_len > data.size()) return false;
    
    req.content = read_string(&data[8], content_len);
    
    return true;
}

std::vector<uint8_t> ProtocolCodec::encode_message_response(const MessageResponse& resp) {
    std::vector<uint8_t> buffer;
    write_uint16(buffer, static_cast<uint16_t>(resp.error_code));
    write_int32(buffer, resp.msg_id);
    return buffer;
}

std::vector<uint8_t> ProtocolCodec::encode_message_notify(const MessageNotify& notify) {
    std::vector<uint8_t> buffer;
    write_int32(buffer, notify.from_id);
    write_uint32(buffer, static_cast<uint32_t>(notify.content.length()));
    buffer.insert(buffer.end(), notify.content.begin(), notify.content.end());
    write_int64(buffer, notify.timestamp);
    return buffer;
}

bool ProtocolCodec::decode_message_notify(const std::vector<uint8_t>& data, MessageNotify& notify) {
    if (data.size() < 16) return false;
    
    notify.from_id = read_int32(&data[0]);
    
    uint32_t content_len = read_uint32(&data[4]);
    if (8 + content_len + 8 > data.size()) return false;
    
    notify.content = read_string(&data[8], content_len);
    notify.timestamp = read_int64(&data[8 + content_len]);
    
    return true;
}

std::vector<uint8_t> ProtocolCodec::encode_friend_list_response(const FriendListResponse& resp) {
    std::vector<uint8_t> buffer;
    write_uint16(buffer, static_cast<uint16_t>(resp.error_code));
    write_uint32(buffer, static_cast<uint32_t>(resp.friends.size()));
    
    for (const auto& friend_pair : resp.friends) {
        write_int32(buffer, friend_pair.first);
        write_string(buffer, friend_pair.second);
    }
    
    return buffer;
}

std::vector<uint8_t> ProtocolCodec::encode_add_friend_response(const AddFriendResponse& resp) {
    std::vector<uint8_t> buffer;
    write_uint16(buffer, static_cast<uint16_t>(resp.error_code));
    return buffer;
}

bool ProtocolCodec::decode_register_request(const std::vector<uint8_t>& data, RegisterRequest& req) {
    if (data.size() < 6) return false;
    
    size_t offset = 0;
    uint16_t username_len = read_uint16(&data[offset]);
    offset += 2;
    if (offset + username_len > data.size()) return false;
    req.username = read_string(&data[offset], username_len);
    offset += username_len;
    
    if (offset + 2 > data.size()) return false;
    uint16_t password_len = read_uint16(&data[offset]);
    offset += 2;
    if (offset + password_len > data.size()) return false;
    req.password = read_string(&data[offset], password_len);
    offset += password_len;
    
    if (offset + 2 > data.size()) return false;
    uint16_t question_len = read_uint16(&data[offset]);
    offset += 2;
    if (offset + question_len > data.size()) return false;
    req.security_question = read_string(&data[offset], question_len);
    offset += question_len;
    
    if (offset + 2 > data.size()) return false;
    uint16_t answer_len = read_uint16(&data[offset]);
    offset += 2;
    if (offset + answer_len > data.size()) return false;
    req.security_answer = read_string(&data[offset], answer_len);
    
    return true;
}

std::vector<uint8_t> ProtocolCodec::encode_register_response(const RegisterResponse& resp) {
    std::vector<uint8_t> buffer;
    write_uint16(buffer, static_cast<uint16_t>(resp.error_code));
    write_int32(buffer, resp.user_id);
    return buffer;
}

bool ProtocolCodec::decode_security_question_request(const std::vector<uint8_t>& data, SecurityQuestionRequest& req) {
    if (data.size() < 2) return false;
    
    uint16_t username_len = read_uint16(&data[0]);
    if (2 + username_len > data.size()) return false;
    req.username = read_string(&data[2], username_len);
    return true;
}

std::vector<uint8_t> ProtocolCodec::encode_security_question_response(const SecurityQuestionResponse& resp) {
    std::vector<uint8_t> buffer;
    write_uint16(buffer, static_cast<uint16_t>(resp.error_code));
    write_string(buffer, resp.question);
    return buffer;
}

bool ProtocolCodec::decode_reset_password_request(const std::vector<uint8_t>& data, ResetPasswordRequest& req) {
    if (data.size() < 6) return false;
    
    size_t offset = 0;
    uint16_t username_len = read_uint16(&data[offset]);
    offset += 2;
    if (offset + username_len > data.size()) return false;
    req.username = read_string(&data[offset], username_len);
    offset += username_len;
    
    if (offset + 2 > data.size()) return false;
    uint16_t password_len = read_uint16(&data[offset]);
    offset += 2;
    if (offset + password_len > data.size()) return false;
    req.new_password = read_string(&data[offset], password_len);
    offset += password_len;
    
    if (offset + 2 > data.size()) return false;
    uint16_t answer_len = read_uint16(&data[offset]);
    offset += 2;
    if (offset + answer_len > data.size()) return false;
    req.security_answer = read_string(&data[offset], answer_len);
    
    return true;
}

std::vector<uint8_t> ProtocolCodec::encode_reset_password_response(const ResetPasswordResponse& resp) {
    std::vector<uint8_t> buffer;
    write_uint16(buffer, static_cast<uint16_t>(resp.error_code));
    return buffer;
}

} // namespace qqchat
