#include "qqchat/chat_server.hpp"
#include "qqchat/protocol_codec.hpp"

namespace qqchat {

ChatServer::ChatServer(const std::string& address, short port,
                       std::shared_ptr<DatabasePool> db_pool,
                       std::shared_ptr<CacheManager> cache_manager)
    : io_context_(1),
      acceptor_(io_context_, tcp::endpoint(boost::asio::ip::address::from_string(address), port)),
      connection_manager_(*this),
      db_pool_(db_pool),
      cache_manager_(cache_manager),
      message_handler_(*this, db_pool, cache_manager) {
    do_accept();
}

ChatServer::~ChatServer() {
    stop();
}

void ChatServer::run() {
    io_context_.run();
}

void ChatServer::stop() {
    io_context_.stop();
    connection_manager_.stop_all();
}

std::shared_ptr<DatabasePool> ChatServer::get_database_pool() const {
    return db_pool_;
}

std::shared_ptr<CacheManager> ChatServer::get_cache_manager() const {
    return cache_manager_;
}

ConnectionManager& ChatServer::get_connection_manager() {
    return connection_manager_;
}

MessageHandler& ChatServer::get_message_handler() {
    return message_handler_;
}

void ChatServer::do_accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                auto conn = std::make_shared<Connection>(std::move(socket), connection_manager_, *this);
                connection_manager_.start(conn);
            }
            
            do_accept();
        });
}

} // namespace qqchat
