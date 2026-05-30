#include "qqchat/connection_manager.hpp"
#include "qqchat/chat_server.hpp"
#include "qqchat/protocol_codec.hpp"
#include <cstdint>
#include <random>
#include <sstream>

namespace qqchat {

ConnectionManager::ConnectionManager(ChatServer& server) : server_(server) {}

ConnectionManager::~ConnectionManager() {
    stop_all();
}

void ConnectionManager::start(std::shared_ptr<Connection> conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    connections_[conn->get_id()] = conn;
}

void ConnectionManager::stop(std::shared_ptr<Connection> conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (conn->get_user_id() != 0) {
        user_to_conn_.erase(conn->get_user_id());
    }
    
    connections_.erase(conn->get_id());
}

void ConnectionManager::stop_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    connections_.clear();
    user_to_conn_.clear();
}

bool ConnectionManager::send_to_user(int user_id, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = user_to_conn_.find(user_id);
    if (it == user_to_conn_.end()) {
        return false;
    }
    
    auto conn_it = connections_.find(it->second);
    if (conn_it == connections_.end()) {
        return false;
    }
    
    conn_it->second->send(data);
    return true;
}

bool ConnectionManager::is_user_online(int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return user_to_conn_.find(user_id) != user_to_conn_.end();
}

void ConnectionManager::set_user_connection(int user_id, const std::string& conn_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    user_to_conn_[user_id] = conn_id;
}

void ConnectionManager::remove_user_connection(int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    user_to_conn_.erase(user_id);
}

std::string ConnectionManager::get_user_connection_id(int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = user_to_conn_.find(user_id);
    if (it != user_to_conn_.end()) {
        return it->second;
    }
    return "";
}

size_t ConnectionManager::get_connection_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connections_.size();
}

Connection::Connection(tcp::socket socket, ConnectionManager& manager, ChatServer& server)
    : socket_(std::move(socket)), manager_(manager), server_(server), user_id_(0) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(0, UINT64_MAX);
    std::stringstream ss;
    ss << std::hex << dis(gen);
    conn_id_ = ss.str();
    
    header_buffer_.resize(ProtocolCodec::HEADER_SIZE);
}

Connection::~Connection() {}

void Connection::start() {
    do_read_header();
}

void Connection::stop() {
    socket_.close();
}

void Connection::send(const std::vector<uint8_t>& data) {
    auto self(shared_from_this());
    boost::asio::post(socket_.get_executor(), [this, self, data]() {
        bool write_in_progress = !write_buffer_.empty();
        write_buffer_.push_back(data);
        if (!write_in_progress) {
            do_write(write_buffer_.front());
        }
    });
}

std::string Connection::get_id() const {
    return conn_id_;
}

int Connection::get_user_id() const {
    return user_id_;
}

void Connection::set_user_id(int user_id) {
    user_id_ = user_id;
}

void Connection::do_read_header() {
    auto self(shared_from_this());
    boost::asio::async_read(socket_, boost::asio::buffer(header_buffer_),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                uint32_t body_length = (header_buffer_[3] << 24) | 
                                      (header_buffer_[4] << 16) | 
                                      (header_buffer_[5] << 8) | 
                                      header_buffer_[6];
                
                if (body_length > 0 && body_length < 1024 * 1024) {
                    body_buffer_.resize(body_length);
                    do_read_body(body_length);
                } else {
                    manager_.stop(self);
                }
            } else {
                manager_.stop(self);
            }
        });
}

void Connection::do_read_body(size_t /*length*/) {
    auto self(shared_from_this());
    boost::asio::async_read(socket_, boost::asio::buffer(body_buffer_),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                Packet packet;
                packet.version = header_buffer_[0];
                uint16_t command = (header_buffer_[1] << 8) | header_buffer_[2];
                packet.command = static_cast<CommandCode>(command);
                packet.data = body_buffer_;
                
                server_.get_message_handler().handle_message(self, packet);
                
                do_read_header();
            } else {
                manager_.stop(self);
            }
        });
}

void Connection::do_write(const std::vector<uint8_t>& data) {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(data),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                write_buffer_.pop_front();
                if (!write_buffer_.empty()) {
                    do_write(write_buffer_.front());
                }
            } else {
                manager_.stop(self);
            }
        });
}

} // namespace qqchat
