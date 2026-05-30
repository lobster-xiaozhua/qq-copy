#ifndef DATABASE_POOL_HPP
#define DATABASE_POOL_HPP

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>
#include <stdexcept>
#include <iomanip>

namespace qqchat {

struct User {
    int user_id;
    std::string username;
    std::string password_hash;
    std::string avatar_url;
    std::string security_question;
    std::string security_answer;
};

struct Friend {
    int user_id;
    int friend_id;
    std::string friend_name;
    std::string avatar_url;
};

struct Message {
    int msg_id;
    int from_id;
    int to_id;
    std::string content;
    std::string timestamp;
};

class DatabasePool {
public:
    DatabasePool(const std::string& host, int port, const std::string& db, 
                 const std::string& user, const std::string& pass, int pool_size = 10);
    ~DatabasePool();

    std::unique_ptr<MYSQL, std::function<void(MYSQL*)>> get_connection();
    
    bool verify_user(const std::string& username, const std::string& password, User& user);
    bool get_user_by_id(int user_id, User& user);
    bool get_user_by_name(const std::string& username, User& user);
    bool register_user(const std::string& username, const std::string& password,
                      const std::string& security_question, const std::string& security_answer);
    bool username_exists(const std::string& username);
    bool get_security_question(const std::string& username, std::string& question, std::string& answer);
    bool reset_password(const std::string& username, const std::string& new_password);
    bool update_password(int user_id, const std::string& new_password);
    
    bool add_user(const std::string& username, const std::string& password_hash);
    bool add_friend(int user_id, int friend_id);
    bool delete_friend(int user_id, int friend_id);
    bool get_friends(int user_id, std::vector<Friend>& friends);
    bool send_message(int from_id, int to_id, const std::string& content, int& out_msg_id);
    bool get_messages(int user_id, int friend_id, std::vector<Message>& messages, int limit = 50);

private:
    std::string host_;
    int port_;
    std::string db_name_;
    std::string user_;
    std::string password_;
    int pool_size_;
    
    std::queue<MYSQL*> connections_;
    std::mutex mutex_;
    std::condition_variable cv_;
    
    MYSQL* create_connection();
};

} // namespace qqchat

#endif // DATABASE_POOL_HPP
