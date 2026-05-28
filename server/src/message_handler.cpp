#include "qqchat/message_handler.hpp"
#include "qqchat/chat_server.hpp"
#include "qqchat/protocol_codec.hpp"
#include <iostream>
#include <chrono>

namespace qqchat {

MessageHandler::MessageHandler(ChatServer& server, std::shared_ptr<DatabasePool> db_pool, 
                               std::shared_ptr<CacheManager> cache_manager)
    : server_(server), db_pool_(db_pool), cache_manager_(cache_manager) {}

void MessageHandler::handle_message(std::shared_ptr<Connection> conn, const Packet& packet) {
    switch (packet.command) {
        case CommandCode::LOGIN_REQ: {
            LoginRequest req;
            if (ProtocolCodec::decode_login_request(packet.data, req)) {
                handle_login(conn, req);
            }
            break;
        }
        case CommandCode::MESSAGE_REQ: {
            MessageRequest req;
            if (ProtocolCodec::decode_message_request(packet.data, req)) {
                handle_chat_message(conn, req);
            }
            break;
        }
        case CommandCode::FRIEND_LIST_REQ: {
            handle_friend_list(conn);
            break;
        }
        case CommandCode::ADD_FRIEND_REQ: {
            AddFriendRequest req;
            req.user_id = conn->get_user_id();
            if (packet.data.size() >= 4) {
                req.friend_id = ProtocolCodec::read_int32(packet.data.data());
                handle_add_friend(conn, req);
            }
            break;
        }
        default:
            std::cerr << "Unknown command: " << static_cast<int>(packet.command) << std::endl;
            break;
    }
}

void MessageHandler::handle_login(std::shared_ptr<Connection> conn, const LoginRequest& req) {
    LoginResponse resp;
    
    User user;
    if (!db_pool_->verify_user(req.username, req.password, user)) {
        resp.error_code = ErrorCode::USER_NOT_FOUND;
        resp.user_id = 0;
        send_response(conn, CommandCode::LOGIN_RESP, ProtocolCodec::encode_login_response(resp));
        return;
    }
    
    if (server_.get_connection_manager().is_user_online(user.user_id)) {
        resp.error_code = ErrorCode::USER_ALREADY_ONLINE;
        resp.user_id = 0;
        send_response(conn, CommandCode::LOGIN_RESP, ProtocolCodec::encode_login_response(resp));
        return;
    }
    
    conn->set_user_id(user.user_id);
    server_.get_connection_manager().set_user_connection(user.user_id, conn->get_id());
    cache_manager_->set_online(user.user_id, true);
    cache_manager_->set_connection_id(user.user_id, conn->get_id());
    
    resp.error_code = ErrorCode::SUCCESS;
    resp.user_id = user.user_id;
    resp.username = user.username;
    
    send_response(conn, CommandCode::LOGIN_RESP, ProtocolCodec::encode_login_response(resp));
}

void MessageHandler::handle_chat_message(std::shared_ptr<Connection> conn, const MessageRequest& req) {
    MessageResponse resp;
    
    int from_id = conn->get_user_id();
    if (from_id == 0) {
        resp.error_code = ErrorCode::PROTOCOL_ERROR;
        send_response(conn, CommandCode::MESSAGE_RESP, ProtocolCodec::encode_message_response(resp));
        return;
    }
    
    if (!db_pool_->send_message(from_id, req.to_id, req.content)) {
        resp.error_code = ErrorCode::DATABASE_ERROR;
        send_response(conn, CommandCode::MESSAGE_RESP, ProtocolCodec::encode_message_response(resp));
        return;
    }
    
    bool online = server_.get_connection_manager().is_user_online(req.to_id);
    
    if (online) {
        MessageNotify notify;
        notify.from_id = from_id;
        notify.content = req.content;
        auto now = std::chrono::system_clock::now();
        notify.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        
        std::vector<uint8_t> notify_data = ProtocolCodec::encode_message_notify(notify);
        Packet notify_packet;
        notify_packet.version = ProtocolCodec::VERSION;
        notify_packet.command = CommandCode::MESSAGE_NOTIFY;
        notify_packet.data = notify_data;
        
        std::vector<uint8_t> encoded = ProtocolCodec::encode(notify_packet);
        server_.get_connection_manager().send_to_user(req.to_id, encoded);
    } else {
        cache_manager_->increment_unread(req.to_id);
    }
    
    resp.error_code = ErrorCode::SUCCESS;
    send_response(conn, CommandCode::MESSAGE_RESP, ProtocolCodec::encode_message_response(resp));
}

void MessageHandler::handle_friend_list(std::shared_ptr<Connection> conn) {
    FriendListResponse resp;
    
    int user_id = conn->get_user_id();
    if (user_id == 0) {
        resp.error_code = ErrorCode::PROTOCOL_ERROR;
        send_response(conn, CommandCode::FRIEND_LIST_RESP, ProtocolCodec::encode_friend_list_response(resp));
        return;
    }
    
    std::vector<Friend> friends;
    if (!db_pool_->get_friends(user_id, friends)) {
        resp.error_code = ErrorCode::DATABASE_ERROR;
        send_response(conn, CommandCode::FRIEND_LIST_RESP, ProtocolCodec::encode_friend_list_response(resp));
        return;
    }
    
    resp.error_code = ErrorCode::SUCCESS;
    for (const auto& f : friends) {
        resp.friends.emplace_back(f.friend_id, f.friend_name);
    }
    
    send_response(conn, CommandCode::FRIEND_LIST_RESP, ProtocolCodec::encode_friend_list_response(resp));
}

void MessageHandler::handle_add_friend(std::shared_ptr<Connection> conn, const AddFriendRequest& req) {
    AddFriendResponse resp;
    
    int user_id = conn->get_user_id();
    if (user_id == 0) {
        resp.error_code = ErrorCode::PROTOCOL_ERROR;
        send_response(conn, CommandCode::ADD_FRIEND_RESP, ProtocolCodec::encode_add_friend_response(resp));
        return;
    }
    
    if (req.user_id == req.friend_id) {
        resp.error_code = ErrorCode::CANNOT_ADD_SELF;
        send_response(conn, CommandCode::ADD_FRIEND_RESP, ProtocolCodec::encode_add_friend_response(resp));
        return;
    }
    
    User friend_user;
    if (!db_pool_->get_user_by_id(req.friend_id, friend_user)) {
        resp.error_code = ErrorCode::USER_NOT_FOUND;
        send_response(conn, CommandCode::ADD_FRIEND_RESP, ProtocolCodec::encode_add_friend_response(resp));
        return;
    }
    
    if (!db_pool_->add_friend(req.user_id, req.friend_id)) {
        resp.error_code = ErrorCode::FRIEND_REQUEST_EXISTS;
        send_response(conn, CommandCode::ADD_FRIEND_RESP, ProtocolCodec::encode_add_friend_response(resp));
        return;
    }
    
    db_pool_->add_friend(req.friend_id, req.user_id);
    
    resp.error_code = ErrorCode::SUCCESS;
    send_response(conn, CommandCode::ADD_FRIEND_RESP, ProtocolCodec::encode_add_friend_response(resp));
}

void MessageHandler::send_response(std::shared_ptr<Connection> conn, CommandCode command, 
                                   const std::vector<uint8_t>& data) {
    Packet packet;
    packet.version = ProtocolCodec::VERSION;
    packet.command = command;
    packet.data = data;
    
    std::vector<uint8_t> encoded = ProtocolCodec::encode(packet);
    conn->send(encoded);
}

} // namespace qqchat
