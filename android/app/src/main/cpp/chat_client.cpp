#include "chat_client.hpp"
#include "protocol_codec.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

namespace qqchat {

ChatClient::ChatClient() 
    : socket_fd_(-1), running_(false), connected_(false) {}

ChatClient::~ChatClient() {
    disconnect();
}

void ChatClient::connect(const std::string& host, int port) {
    disconnect();
    
    host_ = host;
    port_ = port;
    running_ = true;
    
    worker_thread_ = std::thread(&ChatClient::run, this);
}

void ChatClient::disconnect() {
    running_ = false;
    
    if (socket_fd_ != -1) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    
    connected_ = false;
}

bool ChatClient::is_connected() const {
    return connected_;
}

void ChatClient::run() {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    
    if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
        if (status_callback_) {
            status_callback_(0);
        }
        return;
    }
    
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        if (status_callback_) {
            status_callback_(0);
        }
        return;
    }
    
    if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        if (status_callback_) {
            status_callback_(0);
        }
        return;
    }
    
    connected_ = true;
    if (status_callback_) {
        status_callback_(1);
    }
    
    read_loop();
}

void ChatClient::read_loop() {
    char buffer[4096];
    while (running_) {
        ssize_t n = recv(socket_fd_, buffer, sizeof(buffer), 0);
        if (n <= 0) {
            break;
        }
        
        read_buffer_.insert(read_buffer_.end(), buffer, buffer + n);
        
        while (read_buffer_.size() >= ProtocolCodec::HEADER_SIZE) {
            uint32_t length = (read_buffer_[3] << 24) | 
                              (read_buffer_[4] << 16) | 
                              (read_buffer_[5] << 8) | 
                              read_buffer_[6];
            
            if (read_buffer_.size() >= ProtocolCodec::HEADER_SIZE + length) {
                std::vector<uint8_t> packet(read_buffer_.begin(), 
                                            read_buffer_.begin() + ProtocolCodec::HEADER_SIZE + length);
                read_buffer_.erase(read_buffer_.begin(), 
                                  read_buffer_.begin() + ProtocolCodec::HEADER_SIZE + length);
                handle_packet(packet);
            } else {
                break;
            }
        }
    }
    
    connected_ = false;
    if (status_callback_) {
        status_callback_(0);
    }
}

void ChatClient::handle_packet(const std::vector<uint8_t>& packet) {
    CommandCode command;
    std::vector<uint8_t> data;
    
    if (!ProtocolCodec::decode(packet, command, data)) {
        return;
    }
    
    switch (command) {
        case CommandCode::LOGIN_RESP: {
            LoginResponse resp;
            if (ProtocolCodec::decode_login_response(data, resp)) {
                std::lock_guard<std::mutex> lock(callback_mutex_);
                if (login_callback_) {
                    login_callback_(static_cast<int>(resp.error_code), resp.user_id, resp.username);
                }
            }
            break;
        }
        case CommandCode::MESSAGE_NOTIFY: {
            MessageNotify notify;
            if (ProtocolCodec::decode_message_notify(data, notify)) {
                std::lock_guard<std::mutex> lock(callback_mutex_);
                if (message_callback_) {
                    message_callback_(notify.from_id, notify.content, notify.timestamp);
                }
            }
            break;
        }
        case CommandCode::FRIEND_LIST_RESP: {
            std::vector<FriendInfo> friends;
            if (ProtocolCodec::decode_friend_list_response(data, friends)) {
                std::vector<std::pair<int, std::string>> result;
                for (const auto& f : friends) {
                    result.emplace_back(f.friend_id, f.friend_name);
                }
                std::lock_guard<std::mutex> lock(callback_mutex_);
                if (friend_list_callback_) {
                    friend_list_callback_(0, result);
                }
            }
            break;
        }
        case CommandCode::REGISTER_RESP: {
            RegisterResponse resp;
            if (ProtocolCodec::decode_register_response(data, resp)) {
                std::lock_guard<std::mutex> lock(callback_mutex_);
                if (register_callback_) {
                    register_callback_(static_cast<int>(resp.error_code), resp.user_id);
                }
            }
            break;
        }
        case CommandCode::SECURITY_QUESTION_RESP: {
            SecurityQuestionResponse resp;
            if (ProtocolCodec::decode_security_question_response(data, resp)) {
                std::lock_guard<std::mutex> lock(callback_mutex_);
                if (security_question_callback_) {
                    security_question_callback_(static_cast<int>(resp.error_code), resp.question);
                }
            }
            break;
        }
        case CommandCode::RESET_PASSWORD_RESP: {
            ResetPasswordResponse resp;
            if (ProtocolCodec::decode_reset_password_response(data, resp)) {
                std::lock_guard<std::mutex> lock(callback_mutex_);
                if (reset_password_callback_) {
                    reset_password_callback_(static_cast<int>(resp.error_code));
                }
            }
            break;
        }
        default:
            break;
    }
}

void ChatClient::login(const std::string& username, const std::string& password, LoginCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    login_callback_ = callback;
    
    if (socket_fd_ != -1) {
        std::vector<uint8_t> packet = ProtocolCodec::encode_login_request(username, password);
        send(socket_fd_, packet.data(), packet.size(), 0);
    }
}

void ChatClient::send_message(int to_id, const std::string& content) {
    if (socket_fd_ != -1) {
        std::vector<uint8_t> packet = ProtocolCodec::encode_message_request(to_id, content);
        send(socket_fd_, packet.data(), packet.size(), 0);
    }
}

void ChatClient::get_friend_list(FriendListCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    friend_list_callback_ = callback;
    
    if (socket_fd_ != -1) {
        std::vector<uint8_t> packet = ProtocolCodec::encode_friend_list_request();
        send(socket_fd_, packet.data(), packet.size(), 0);
    }
}

void ChatClient::add_friend(int friend_id) {
    if (socket_fd_ != -1) {
        std::vector<uint8_t> packet = ProtocolCodec::encode_add_friend_request(friend_id);
        send(socket_fd_, packet.data(), packet.size(), 0);
    }
}

void ChatClient::register_user(const std::string& username, const std::string& password,
                               const std::string& security_question, const std::string& security_answer,
                               RegisterCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    register_callback_ = callback;
    
    if (socket_fd_ != -1) {
        std::vector<uint8_t> packet = ProtocolCodec::encode_register_request(username, password, security_question, security_answer);
        send(socket_fd_, packet.data(), packet.size(), 0);
    }
}

void ChatClient::get_security_question(const std::string& username, SecurityQuestionCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    security_question_callback_ = callback;
    
    if (socket_fd_ != -1) {
        std::vector<uint8_t> packet = ProtocolCodec::encode_security_question_request(username);
        send(socket_fd_, packet.data(), packet.size(), 0);
    }
}

void ChatClient::reset_password(const std::string& username, const std::string& new_password,
                               const std::string& security_answer, ResetPasswordCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    reset_password_callback_ = callback;
    
    if (socket_fd_ != -1) {
        std::vector<uint8_t> packet = ProtocolCodec::encode_reset_password_request(username, new_password, security_answer);
        send(socket_fd_, packet.data(), packet.size(), 0);
    }
}

void ChatClient::set_message_callback(MessageCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    message_callback_ = callback;
}

void ChatClient::set_status_callback(StatusCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    status_callback_ = callback;
}

} // namespace qqchat