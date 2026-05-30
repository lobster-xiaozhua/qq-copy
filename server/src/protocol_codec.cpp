#include "qqchat/protocol_codec.hpp"
#include <cstring>

namespace qqchat {

void ProtocolCodec::write_uint16(std::vector<uint8_t>& buffer, uint16_t value) {
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

void ProtocolCodec::write_uint32(std::vector<uint8_t>& buffer, uint32_t value) {
    buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

void ProtocolCodec::write_int32(std::vector<uint8_t>& buffer, int32_t value) {
    write_uint32(buffer, static_cast<uint32_t>(value));
}

void ProtocolCodec::write_int64(std::vector<uint8_t>& buffer, int64_t value) {
    buffer.push_back(static_cast<uint8_t>((value >> 56) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 48) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 40) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 32) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

void ProtocolCodec::write_string(std::vector<uint8_t>& buffer, const std::string& str) {
    write_uint16(buffer, static_cast<uint16_t>(str.length()));
    for (char c : str) {
        buffer.push_back(static_cast<uint8_t>(c));
    }
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
    buffer.reserve(HEADER_SIZE + packet.data.size());
    
    buffer.push_back(packet.version);
    write_uint16(buffer, static_cast<uint16_t>(packet.command));
    write_uint32(buffer, static_cast<uint32_t>(packet.data.size()));
    
    for (auto b : packet.data) {
        buffer.push_back(b);
    }
    
    return buffer;
}

bool ProtocolCodec::decode(const std::vector<uint8_t>& data, Packet& packet) {
    if (data.size() < HEADER_SIZE) return false;
    
    packet.version = data[0];
    packet.command = static_cast<CommandCode>(read_uint16(data.data() + 1));
    uint32_t body_len = read_uint32(data.data() + 3);
    
    if (data.size() < HEADER_SIZE + body_len) return false;
    
    packet.data.resize(body_len);
    memcpy(packet.data.data(), data.data() + HEADER_SIZE, body_len);
    
    return true;
}

bool ProtocolCodec::decode_login_request(const std::vector<uint8_t>& data, LoginRequest& req) {
    if (data.empty()) return false;
    
    size_t offset = 0;
    uint16_t username_len = read_uint16(data.data() + offset);
    offset += 2;
    if (offset + username_len > data.size()) return false;
    req.username = read_string(data.data() + offset, username_len);
    offset += username_len;
    
    uint16_t password_len = read_uint16(data.data() + offset);
    offset += 2;
    if (offset + password_len > data.size()) return false;
    req.password = read_string(data.data() + offset, password_len);
    
    return true;
}

std::vector<uint8_t> ProtocolCodec::encode_login_response(const LoginResponse& resp) {
    std::vector<uint8_t> data;
    write_uint16(data, static_cast<uint16_t>(resp.error_code));
    write_int32(data, resp.user_id);
    write_string(data, resp.username);
    return data;
}

bool ProtocolCodec::decode_message_request(const std::vector<uint8_t>& data, MessageRequest& req) {
    if (data.size() < 4) return false;
    req.to_id = read_int32(data.data());
    
    size_t offset = 4;
    uint16_t content_len = read_uint16(data.data() + offset);
    offset += 2;
    if (offset + content_len > data.size()) return false;
    req.content = read_string(data.data() + offset, content_len);
    
    return true;
}

std::vector<uint8_t> ProtocolCodec::encode_message_response(const MessageResponse& resp) {
    std::vector<uint8_t> data;
    write_uint16(data, static_cast<uint16_t>(resp.error_code));
    write_int32(data, resp.msg_id);
    return data;
}

std::vector<uint8_t> ProtocolCodec::encode_message_notify(const MessageNotify& notify) {
    std::vector<uint8_t> data;
    write_int32(data, notify.from_id);
    write_string(data, notify.content);
    write_int64(data, notify.timestamp);
    return data;
}

bool ProtocolCodec::decode_message_notify(const std::vector<uint8_t>& data, MessageNotify& notify) {
    if (data.size() < 4) return false;
    
    size_t offset = 0;
    notify.from_id = read_int32(data.data() + offset);
    offset += 4;
    
    uint16_t content_len = read_uint16(data.data() + offset);
    offset += 2;
    if (offset + content_len > data.size()) return false;
    notify.content = read_string(data.data() + offset, content_len);
    offset += content_len;
    
    if (offset + 8 > data.size()) return false;
    notify.timestamp = read_int64(data.data() + offset);
    
    return true;
}

std::vector<uint8_t> ProtocolCodec::encode_friend_list_response(const FriendListResponse& resp) {
    std::vector<uint8_t> data;
    write_uint16(data, static_cast<uint16_t>(resp.error_code));
    write_uint16(data, static_cast<uint16_t>(resp.friends.size()));
    
    for (const auto& f : resp.friends) {
        write_int32(data, f.first);
        write_string(data, f.second);
    }
    
    return data;
}

std::vector<uint8_t> ProtocolCodec::encode_add_friend_response(const AddFriendResponse& resp) {
    std::vector<uint8_t> data;
    write_uint16(data, static_cast<uint16_t>(resp.error_code));
    return data;
}

bool ProtocolCodec::decode_register_request(const std::vector<uint8_t>& data, RegisterRequest& req) {
    if (data.empty()) return false;
    
    size_t offset = 0;
    uint16_t username_len = read_uint16(data.data() + offset);
    offset += 2;
    if (offset + username_len > data.size()) return false;
    req.username = read_string(data.data() + offset, username_len);
    offset += username_len;
    
    uint16_t password_len = read_uint16(data.data() + offset);
    offset += 2;
    if (offset + password_len > data.size()) return false;
    req.password = read_string(data.data() + offset, password_len);
    offset += password_len;
    
    uint16_t question_len = read_uint16(data.data() + offset);
    offset += 2;
    if (offset + question_len > data.size()) return false;
    req.security_question = read_string(data.data() + offset, question_len);
    offset += question_len;
    
    uint16_t answer_len = read_uint16(data.data() + offset);
    offset += 2;
    if (offset + answer_len > data.size()) return false;
    req.security_answer = read_string(data.data() + offset, answer_len);
    
    return true;
}

std::vector<uint8_t> ProtocolCodec::encode_register_response(const RegisterResponse& resp) {
    std::vector<uint8_t> data;
    write_uint16(data, static_cast<uint16_t>(resp.error_code));
    write_int32(data, resp.user_id);
    return data;
}

bool ProtocolCodec::decode_security_question_request(const std::vector<uint8_t>& data, SecurityQuestionRequest& req) {
    if (data.empty()) return false;
    
    uint16_t username_len = read_uint16(data.data());
    if (2 + username_len > data.size()) return false;
    req.username = read_string(data.data() + 2, username_len);
    
    return true;
}

std::vector<uint8_t> ProtocolCodec::encode_security_question_response(const SecurityQuestionResponse& resp) {
    std::vector<uint8_t> data;
    write_uint16(data, static_cast<uint16_t>(resp.error_code));
    write_string(data, resp.question);
    return data;
}

bool ProtocolCodec::decode_reset_password_request(const std::vector<uint8_t>& data, ResetPasswordRequest& req) {
    if (data.empty()) return false;
    
    size_t offset = 0;
    uint16_t username_len = read_uint16(data.data() + offset);
    offset += 2;
    if (offset + username_len > data.size()) return false;
    req.username = read_string(data.data() + offset, username_len);
    offset += username_len;
    
    uint16_t password_len = read_uint16(data.data() + offset);
    offset += 2;
    if (offset + password_len > data.size()) return false;
    req.new_password = read_string(data.data() + offset, password_len);
    offset += password_len;
    
    uint16_t answer_len = read_uint16(data.data() + offset);
    offset += 2;
    if (offset + answer_len > data.size()) return false;
    req.security_answer = read_string(data.data() + offset, answer_len);
    
    return true;
}

std::vector<uint8_t> ProtocolCodec::encode_reset_password_response(const ResetPasswordResponse& resp) {
    std::vector<uint8_t> data;
    write_uint16(data, static_cast<uint16_t>(resp.error_code));
    return data;
}

} // namespace qqchat
