#include "qqchat/message_handler.hpp"
#include "qqchat/chat_server.hpp"
#include "qqchat/protocol_codec.hpp"
#include <openssl/sha.h>
#include <iostream>
#include <sstream>
#include <chrono>
#include <ctime>

namespace qqchat {

namespace {
std::string now_str() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return buf;
}
}

#define LOG(msg) do { std::ostringstream _os; _os << "[" << now_str() << "] " << msg; std::cout << _os.str() << std::endl; } while(0)

MessageHandler::MessageHandler(ChatServer& server, std::shared_ptr<DatabasePool> db_pool, 
                               std::shared_ptr<CacheManager> cache_manager)
    : server_(server), db_pool_(db_pool), cache_manager_(cache_manager) {}

void MessageHandler::handle_message(std::shared_ptr<Connection> conn, const Packet& packet) {
    LOG("handle_message cmd=" << static_cast<int>(packet.command) << " from_uid=" << conn->get_user_id());
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
        case CommandCode::REGISTER_REQ: {
            RegisterRequest req;
            if (ProtocolCodec::decode_register_request(packet.data, req)) {
                handle_register(conn, req);
            }
            break;
        }
        case CommandCode::SECURITY_QUESTION_REQ: {
            SecurityQuestionRequest req;
            if (ProtocolCodec::decode_security_question_request(packet.data, req)) {
                handle_security_question(conn, req);
            }
            break;
        }
        case CommandCode::RESET_PASSWORD_REQ: {
            ResetPasswordRequest req;
            if (ProtocolCodec::decode_reset_password_request(packet.data, req)) {
                handle_reset_password(conn, req);
            }
            break;
        }
        case CommandCode::VERSION_CHECK_REQ: {
            VersionCheckRequest req;
            if (ProtocolCodec::decode_version_check_request(packet.data, req)) {
                handle_version_check(conn, req);
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
        LOG("login FAIL for user=" << req.username << " reason=USER_NOT_FOUND");
        resp.error_code = ErrorCode::USER_NOT_FOUND;
        resp.user_id = 0;
        send_response(conn, CommandCode::LOGIN_RESP, ProtocolCodec::encode_login_response(resp));
        return;
    }
    
    if (server_.get_connection_manager().is_user_online(user.user_id)) {
        LOG("login FAIL for user=" << req.username << " reason=ALREADY_ONLINE");
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
    
    LOG("login OK uid=" << user.user_id << " user=" << user.username);
    send_response(conn, CommandCode::LOGIN_RESP, ProtocolCodec::encode_login_response(resp));
}

void MessageHandler::handle_chat_message(std::shared_ptr<Connection> conn, const MessageRequest& req) {
    MessageResponse resp;
    
    int from_id = conn->get_user_id();
    if (from_id == 0) {
        LOG("chat FAIL from=0 not_authenticated");
        resp.error_code = ErrorCode::PROTOCOL_ERROR;
        resp.msg_id = 0;
        send_response(conn, CommandCode::MESSAGE_RESP, ProtocolCodec::encode_message_response(resp));
        return;
    }
    
    int msg_id = 0;
    if (!db_pool_->send_message(from_id, req.to_id, req.content, msg_id)) {
        LOG("chat FAIL from=" << from_id << " to=" << req.to_id << " reason=DB_ERROR");
        resp.error_code = ErrorCode::DATABASE_ERROR;
        resp.msg_id = 0;
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
    resp.msg_id = msg_id;
    LOG("chat OK from=" << from_id << " to=" << req.to_id << " online=" << online << " msg_id=" << msg_id);
    send_response(conn, CommandCode::MESSAGE_RESP, ProtocolCodec::encode_message_response(resp));
}

void MessageHandler::handle_friend_list(std::shared_ptr<Connection> conn) {
    FriendListResponse resp;
    
    int user_id = conn->get_user_id();
    if (user_id == 0) {
        LOG("friend_list FAIL not_authenticated");
        resp.error_code = ErrorCode::PROTOCOL_ERROR;
        send_response(conn, CommandCode::FRIEND_LIST_RESP, ProtocolCodec::encode_friend_list_response(resp));
        return;
    }
    
    std::vector<Friend> friends;
    if (!db_pool_->get_friends(user_id, friends)) {
        LOG("friend_list FAIL uid=" << user_id << " reason=DB_ERROR");
        resp.error_code = ErrorCode::DATABASE_ERROR;
        send_response(conn, CommandCode::FRIEND_LIST_RESP, ProtocolCodec::encode_friend_list_response(resp));
        return;
    }
    
    resp.error_code = ErrorCode::SUCCESS;
    for (const auto& f : friends) {
        resp.friends.emplace_back(f.friend_id, f.friend_name);
    }
    
    LOG("friend_list OK uid=" << user_id << " count=" << resp.friends.size());
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
        LOG("add_friend FAIL uid=" << user_id << " reason=CANNOT_ADD_SELF");
        resp.error_code = ErrorCode::CANNOT_ADD_SELF;
        send_response(conn, CommandCode::ADD_FRIEND_RESP, ProtocolCodec::encode_add_friend_response(resp));
        return;
    }
    
    User friend_user;
    if (!db_pool_->get_user_by_id(req.friend_id, friend_user)) {
        LOG("add_friend FAIL uid=" << user_id << " friend=" << req.friend_id << " reason=USER_NOT_FOUND");
        resp.error_code = ErrorCode::USER_NOT_FOUND;
        send_response(conn, CommandCode::ADD_FRIEND_RESP, ProtocolCodec::encode_add_friend_response(resp));
        return;
    }
    
    if (!db_pool_->add_friend(req.user_id, req.friend_id)) {
        LOG("add_friend FAIL uid=" << user_id << " friend=" << req.friend_id << " reason=FRIEND_EXISTS");
        resp.error_code = ErrorCode::FRIEND_REQUEST_EXISTS;
        send_response(conn, CommandCode::ADD_FRIEND_RESP, ProtocolCodec::encode_add_friend_response(resp));
        return;
    }
    
    db_pool_->add_friend(req.friend_id, req.user_id);
    
    resp.error_code = ErrorCode::SUCCESS;
    LOG("add_friend OK uid=" << user_id << " friend=" << req.friend_id);
    send_response(conn, CommandCode::ADD_FRIEND_RESP, ProtocolCodec::encode_add_friend_response(resp));
}

void MessageHandler::handle_register(std::shared_ptr<Connection> conn, const RegisterRequest& req) {
    RegisterResponse resp;
    
    if (req.username.empty() || req.password.empty() || req.security_question.empty() || req.security_answer.empty()) {
        LOG("register FAIL username=" << req.username << " reason=INVALID_REQUEST");
        resp.error_code = ErrorCode::INVALID_REQUEST;
        resp.user_id = 0;
        send_response(conn, CommandCode::REGISTER_RESP, ProtocolCodec::encode_register_response(resp));
        return;
    }
    
    if (db_pool_->username_exists(req.username)) {
        LOG("register FAIL username=" << req.username << " reason=USERNAME_EXISTS");
        resp.error_code = ErrorCode::USERNAME_EXISTS;
        resp.user_id = 0;
        send_response(conn, CommandCode::REGISTER_RESP, ProtocolCodec::encode_register_response(resp));
        return;
    }
    
    std::string password_hash = sha256(req.password);
    std::string answer_hash = sha256(req.security_answer);
    int user_id = 0;
    
    if (!db_pool_->register_user(req.username, password_hash, req.security_question, answer_hash, user_id)) {
        LOG("register FAIL username=" << req.username << " reason=DB_ERROR");
        resp.error_code = ErrorCode::DATABASE_ERROR;
        resp.user_id = 0;
        send_response(conn, CommandCode::REGISTER_RESP, ProtocolCodec::encode_register_response(resp));
        return;
    }
    
    LOG("register OK uid=" << user_id << " username=" << req.username);
    resp.error_code = ErrorCode::SUCCESS;
    resp.user_id = user_id;
    send_response(conn, CommandCode::REGISTER_RESP, ProtocolCodec::encode_register_response(resp));
}

void MessageHandler::handle_security_question(std::shared_ptr<Connection> conn, const SecurityQuestionRequest& req) {
    SecurityQuestionResponse resp;
    
    if (req.username.empty()) {
        LOG("security_question FAIL username=empty reason=INVALID_REQUEST");
        resp.error_code = ErrorCode::INVALID_REQUEST;
        resp.question = "";
        send_response(conn, CommandCode::SECURITY_QUESTION_RESP, ProtocolCodec::encode_security_question_response(resp));
        return;
    }
    
    std::string question, answer_hash;
    if (!db_pool_->get_security_question(req.username, question, answer_hash)) {
        LOG("security_question FAIL username=" << req.username << " reason=USER_NOT_FOUND");
        resp.error_code = ErrorCode::USER_NOT_FOUND;
        resp.question = "";
        send_response(conn, CommandCode::SECURITY_QUESTION_RESP, ProtocolCodec::encode_security_question_response(resp));
        return;
    }
    
    LOG("security_question OK username=" << req.username);
    resp.error_code = ErrorCode::SUCCESS;
    resp.question = question;
    send_response(conn, CommandCode::SECURITY_QUESTION_RESP, ProtocolCodec::encode_security_question_response(resp));
}

void MessageHandler::handle_reset_password(std::shared_ptr<Connection> conn, const ResetPasswordRequest& req) {
    ResetPasswordResponse resp;
    
    if (req.username.empty() || req.new_password.empty() || req.security_answer.empty()) {
        LOG("reset_password FAIL username=" << req.username << " reason=INVALID_REQUEST");
        resp.error_code = ErrorCode::INVALID_REQUEST;
        send_response(conn, CommandCode::RESET_PASSWORD_RESP, ProtocolCodec::encode_reset_password_response(resp));
        return;
    }
    
    std::string question, answer_hash;
    if (!db_pool_->get_security_question(req.username, question, answer_hash)) {
        LOG("reset_password FAIL username=" << req.username << " reason=USER_NOT_FOUND");
        resp.error_code = ErrorCode::USER_NOT_FOUND;
        send_response(conn, CommandCode::RESET_PASSWORD_RESP, ProtocolCodec::encode_reset_password_response(resp));
        return;
    }
    
    std::string input_answer_hash = sha256(req.security_answer);
    if (input_answer_hash != answer_hash) {
        LOG("reset_password FAIL username=" << req.username << " reason=INVALID_ANSWER");
        resp.error_code = ErrorCode::INVALID_ANSWER;
        send_response(conn, CommandCode::RESET_PASSWORD_RESP, ProtocolCodec::encode_reset_password_response(resp));
        return;
    }
    
    std::string new_password_hash = sha256(req.new_password);
    if (!db_pool_->reset_password(req.username, new_password_hash)) {
        LOG("reset_password FAIL username=" << req.username << " reason=DB_ERROR");
        resp.error_code = ErrorCode::DATABASE_ERROR;
        send_response(conn, CommandCode::RESET_PASSWORD_RESP, ProtocolCodec::encode_reset_password_response(resp));
        return;
    }
    
    LOG("reset_password OK username=" << req.username);
    resp.error_code = ErrorCode::SUCCESS;
    send_response(conn, CommandCode::RESET_PASSWORD_RESP, ProtocolCodec::encode_reset_password_response(resp));
}

std::string MessageHandler::sha256(const std::string& str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)str.c_str(), str.length(), hash);
    std::string result;
    result.reserve(SHA256_DIGEST_LENGTH * 2);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", hash[i]);
        result += buf;
    }
    return result;
}

static constexpr int SERVER_APP_VERSION = 2;

void MessageHandler::handle_version_check(std::shared_ptr<Connection> conn, const VersionCheckRequest& req) {
    VersionCheckResponse resp;
    resp.error_code = ErrorCode::SUCCESS;
    resp.server_version = SERVER_APP_VERSION;
    resp.update_url = "https://github.com/lobster-xiaozhua/qq-copy/releases";
    resp.update_desc = "新版本可用，请下载最新安装包更新";
    
    LOG("version_check client=" << req.client_version << " server=" << SERVER_APP_VERSION << " need_update=" << (req.client_version < SERVER_APP_VERSION));
    send_response(conn, CommandCode::VERSION_CHECK_RESP, ProtocolCodec::encode_version_check_response(resp));
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
