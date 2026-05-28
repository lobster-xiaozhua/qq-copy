#ifndef CONNECTION_MANAGER_HPP
#define CONNECTION_MANAGER_HPP

#include <boost/asio.hpp>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <string>

namespace qqchat {

using boost::asio::ip::tcp;

class Connection;
class ChatServer;

class ConnectionManager {
public:
    explicit ConnectionManager(ChatServer& server);
    ~ConnectionManager();
    
    void start(std::shared_ptr<Connection> conn);
    void stop(std::shared_ptr<Connection> conn);
    void stop_all();
    
    bool send_to_user(int user_id, const std::vector<uint8_t>& data);
    bool is_user_online(int user_id);
    void set_user_connection(int user_id, const std::string& conn_id);
    void remove_user_connection(int user_id);
    std::string get_user_connection_id(int user_id);
    
    size_t get_connection_count() const;
    
private:
    ChatServer& server_;
    std::unordered_map<std::string, std::shared_ptr<Connection>> connections_;
    std::unordered_map<int, std::string> user_to_conn_;
    mutable std::mutex mutex_;
};

class Connection : public std::enable_shared_from_this<Connection> {
public:
    Connection(tcp::socket socket, ConnectionManager& manager, ChatServer& server);
    ~Connection();
    
    void start();
    void stop();
    void send(const std::vector<uint8_t>& data);
    std::string get_id() const;
    int get_user_id() const;
    void set_user_id(int user_id);
    
private:
    void do_read_header();
    void do_read_body(size_t length);
    void do_write(const std::vector<uint8_t>& data);
    
    tcp::socket socket_;
    ConnectionManager& manager_;
    ChatServer& server_;
    
    std::string conn_id_;
    int user_id_;
    
    std::vector<uint8_t> header_buffer_;
    std::vector<uint8_t> body_buffer_;
};

} // namespace qqchat

#endif // CONNECTION_MANAGER_HPP
