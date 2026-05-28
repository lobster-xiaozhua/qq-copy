#ifndef CHAT_SERVER_HPP
#define CHAT_SERVER_HPP

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include "connection_manager.hpp"
#include "database_pool.hpp"
#include "cache_manager.hpp"
#include "message_handler.hpp"

namespace qqchat {

using boost::asio::ip::tcp;

class ChatServer {
public:
    ChatServer(const std::string& address, short port,
               std::shared_ptr<DatabasePool> db_pool,
               std::shared_ptr<CacheManager> cache_manager);
    ~ChatServer();
    
    void run();
    void stop();
    
    std::shared_ptr<DatabasePool> get_database_pool() const;
    std::shared_ptr<CacheManager> get_cache_manager() const;
    ConnectionManager& get_connection_manager();
    MessageHandler& get_message_handler();
    
private:
    void do_accept();
    
    boost::asio::io_context io_context_;
    tcp::acceptor acceptor_;
    ConnectionManager connection_manager_;
    std::shared_ptr<DatabasePool> db_pool_;
    std::shared_ptr<CacheManager> cache_manager_;
    MessageHandler message_handler_;
};

} // namespace qqchat

#endif // CHAT_SERVER_HPP
