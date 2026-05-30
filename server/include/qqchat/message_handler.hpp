#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP

#include "protocol_codec.hpp"
#include "database_pool.hpp"
#include "cache_manager.hpp"
#include "connection_manager.hpp"
#include <memory>

namespace qqchat {

class ChatServer;

class MessageHandler {
public:
    MessageHandler(ChatServer& server, std::shared_ptr<DatabasePool> db_pool,
                  std::shared_ptr<CacheManager> cache_manager);
    
    void handle_message(std::shared_ptr<Connection> conn, const Packet& packet);
    void handle_login(std::shared_ptr<Connection> conn, const LoginRequest& req);
    void handle_chat_message(std::shared_ptr<Connection> conn, const MessageRequest& req);
    void handle_friend_list(std::shared_ptr<Connection> conn);
    void handle_add_friend(std::shared_ptr<Connection> conn, const AddFriendRequest& req);
    void handle_register(std::shared_ptr<Connection> conn, const RegisterRequest& req);
    void handle_security_question(std::shared_ptr<Connection> conn, const SecurityQuestionRequest& req);
    void handle_reset_password(std::shared_ptr<Connection> conn, const ResetPasswordRequest& req);
    void handle_version_check(std::shared_ptr<Connection> conn, const VersionCheckRequest& req);
    
private:
    void send_response(std::shared_ptr<Connection> conn, CommandCode command, 
                      const std::vector<uint8_t>& data);
    std::string sha256(const std::string& str);
    
    ChatServer& server_;
    std::shared_ptr<DatabasePool> db_pool_;
    std::shared_ptr<CacheManager> cache_manager_;
};

} // namespace qqchat

#endif // MESSAGE_HANDLER_HPP
