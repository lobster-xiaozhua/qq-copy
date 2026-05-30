#include "protocol_codec.hpp"

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

std::vector<uint8_t> ProtocolCodec::encode(CommandCode command, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> buffer;
    buffer.push_back(VERSION);
    
    uint16_t cmd = static_cast<uint16_t>(command);
    write_uint16(buffer, cmd);
    
    uint32_t length = static_cast<uint32_t>(data.size());
    write_uint32(buffer, length);
    
    buffer.insert(buffer.end(), data.begin(), data.end());
    
    return buffer;
}

bool ProtocolCodec::decode(const std::vector<uint8_t>& buffer, CommandCode& command, std::vector<uint8_t>& data) {
    if (buffer.size() < HEADER_SIZE) {
        return false;
    }
    
    uint16_t cmd = read_uint16(&buffer[1]);
    command = static_cast<CommandCode>(cmd);
    
    uint32_t length = read_uint32(&buffer[3]);
    
    if (buffer.size() != HEADER_SIZE + length) {
        return false;
    }
    
    data.resize(length);
    std::copy(buffer.begin() + HEADER_SIZE, buffer.end(), data.begin());
    
    return true;
}

std::vector<uint8_t> ProtocolCodec::encode_login_request(const std::string& username, const std::string& password) {
    std::vector<uint8_t> data;
    write_string(data, username);
    write_string(data, password);
    return encode(CommandCode::LOGIN_REQ, data);
}

bool ProtocolCodec::decode_login_response(const std::vector<uint8_t>& data, LoginResponse& resp) {
    if (data.size() < 6) return false;
    
    resp.error_code = static_cast<ErrorCode>(read_uint16(&data[0]));
    resp.user_id = read_int32(&data[2]);
    
    size_t offset = 6;
    if (offset < data.size()) {
        uint16_t name_len = read_uint16(&data[offset]);
        offset += 2;
        if (offset + name_len <= data.size()) {
            resp.username = read_string(&data[offset], name_len);
        }
    }
    
    return true;
}

std::vector<uint8_t> ProtocolCodec::encode_message_request(int to_id, const std::string& content) {
    std::vector<uint8_t> data;
    write_int32(data, to_id);
    write_uint32(data, static_cast<uint32_t>(content.length()));
    data.insert(data.end(), content.begin(), content.end());
    return encode(CommandCode::MESSAGE_REQ, data);
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

std::vector<uint8_t> ProtocolCodec::encode_friend_list_request() {
    std::vector<uint8_t> data;
    return encode(CommandCode::FRIEND_LIST_REQ, data);
}

bool ProtocolCodec::decode_friend_list_response(const std::vector<uint8_t>& data, std::vector<FriendInfo>& friends) {
    if (data.size() < 6) return false;
    
    ErrorCode error_code = static_cast<ErrorCode>(read_uint16(&data[0]));
    if (error_code != ErrorCode::SUCCESS) {
        return false;
    }
    
    uint32_t count = read_uint32(&data[2]);
    size_t offset = 6;
    
    for (uint32_t i = 0; i < count; ++i) {
        if (offset + 4 > data.size()) break;
        
        FriendInfo info;
        info.friend_id = read_int32(&data[offset]);
        offset += 4;
        
        if (offset + 2 > data.size()) break;
        uint16_t name_len = read_uint16(&data[offset]);
        offset += 2;
        
        if (offset + name_len > data.size()) break;
        info.friend_name = read_string(&data[offset], name_len);
        offset += name_len;
        
        friends.push_back(info);
    }
    
    return true;
}

std::vector<uint8_t> ProtocolCodec::encode_add_friend_request(int friend_id) {
    std::vector<uint8_t> data;
    write_int32(data, friend_id);
    return encode(CommandCode::ADD_FRIEND_REQ, data);
}

std::vector<uint8_t> ProtocolCodec::encode_register_request(const std::string& username, const std::string& password,
                                                          const std::string& security_question, const std::string& security_answer) {
    std::vector<uint8_t> data;
    write_string(data, username);
    write_string(data, password);
    write_string(data, security_question);
    write_string(data, security_answer);
    return encode(CommandCode::REGISTER_REQ, data);
}

bool ProtocolCodec::decode_register_response(const std::vector<uint8_t>& data, RegisterResponse& resp) {
    if (data.size() < 6) return false;
    resp.error_code = static_cast<ErrorCode>(read_uint16(&data[0]));
    resp.user_id = read_int32(&data[2]);
    return true;
}

std::vector<uint8_t> ProtocolCodec::encode_security_question_request(const std::string& username) {
    std::vector<uint8_t> data;
    write_string(data, username);
    return encode(CommandCode::SECURITY_QUESTION_REQ, data);
}

bool ProtocolCodec::decode_security_question_response(const std::vector<uint8_t>& data, SecurityQuestionResponse& resp) {
    if (data.size() < 2) return false;
    resp.error_code = static_cast<ErrorCode>(read_uint16(&data[0]));
    size_t offset = 2;
    if (offset < data.size()) {
        uint16_t question_len = read_uint16(&data[offset]);
        offset += 2;
        if (offset + question_len <= data.size()) {
            resp.question = read_string(&data[offset], question_len);
        }
    }
    return true;
}

std::vector<uint8_t> ProtocolCodec::encode_reset_password_request(const std::string& username, const std::string& new_password,
                                                                 const std::string& security_answer) {
    std::vector<uint8_t> data;
    write_string(data, username);
    write_string(data, new_password);
    write_string(data, security_answer);
    return encode(CommandCode::RESET_PASSWORD_REQ, data);
}

bool ProtocolCodec::decode_reset_password_response(const std::vector<uint8_t>& data, ResetPasswordResponse& resp) {
    if (data.size() < 2) return false;
    resp.error_code = static_cast<ErrorCode>(read_uint16(&data[0]));
    return true;
}

std::vector<uint8_t> ProtocolCodec::encode_version_check_request(int client_version) {
    std::vector<uint8_t> data;
    write_int32(data, client_version);
    return encode(CommandCode::VERSION_CHECK_REQ, data);
}

bool ProtocolCodec::decode_version_check_response(const std::vector<uint8_t>& data, VersionCheckResponse& resp) {
    if (data.size() < 6) return false;
    
    resp.error_code = static_cast<ErrorCode>(read_uint16(&data[0]));
    resp.server_version = read_int32(&data[2]);
    
    size_t offset = 6;
    if (offset + 2 <= data.size()) {
        uint16_t url_len = read_uint16(&data[offset]);
        offset += 2;
        if (offset + url_len <= data.size()) {
            resp.update_url = read_string(&data[offset], url_len);
            offset += url_len;
        }
    }
    
    if (offset + 2 <= data.size()) {
        uint16_t desc_len = read_uint16(&data[offset]);
        offset += 2;
        if (offset + desc_len <= data.size()) {
            resp.update_desc = read_string(&data[offset], desc_len);
        }
    }
    
    return true;
}

} // namespace qqchat