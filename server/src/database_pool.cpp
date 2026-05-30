#include "qqchat/database_pool.hpp"
#include <openssl/sha.h>
#include <cstring>
#include <algorithm>
#include <random>
#include <sstream>

namespace qqchat {

static std::string sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)input.c_str(), input.length(), hash);
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return oss.str();
}

DatabasePool::DatabasePool(const std::string& host, int port, const std::string& db, 
                           const std::string& user, const std::string& pass, int pool_size)
    : host_(host), port_(port), db_name_(db), user_(user), password_(pass), pool_size_(pool_size) {
    for (int i = 0; i < pool_size_; ++i) {
        MYSQL* conn = create_connection();
        if (conn) {
            connections_.push(conn);
        }
    }
}

DatabasePool::~DatabasePool() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!connections_.empty()) {
        MYSQL* conn = connections_.front();
        connections_.pop();
        mysql_close(conn);
    }
}

MYSQL* DatabasePool::create_connection() {
    MYSQL* conn = mysql_init(nullptr);
    if (!conn) {
        return nullptr;
    }
    
    mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");
    
    if (!mysql_real_connect(conn, host_.c_str(), user_.c_str(), password_.c_str(), 
                            db_name_.c_str(), port_, nullptr, 0)) {
        mysql_close(conn);
        return nullptr;
    }
    
    return conn;
}

std::unique_ptr<MYSQL, std::function<void(MYSQL*)>> DatabasePool::get_connection() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !connections_.empty(); });
    
    MYSQL* conn = connections_.front();
    connections_.pop();
    
    if (mysql_ping(conn) != 0) {
        mysql_close(conn);
        conn = create_connection();
    }
    
    return std::unique_ptr<MYSQL, std::function<void(MYSQL*)>>(conn, [this](MYSQL* c) {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.push(c);
        cv_.notify_one();
    });
}

bool DatabasePool::verify_user(const std::string& username, const std::string& password, User& user) {
    auto conn = get_connection();
    if (!conn) return false;
    
    std::string query = "SELECT user_id, username, password_hash FROM users WHERE username = ?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return false;
    
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_BIND params[1];
    memset(params, 0, sizeof(params));
    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = (char*)username.c_str();
    params[0].buffer_length = username.length();
    
    if (mysql_stmt_bind_param(stmt, params) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_RES* result = mysql_stmt_result_metadata(stmt);
    if (!result) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_BIND result_params[3];
    memset(result_params, 0, sizeof(result_params));
    
    int user_id = 0;
    char username_buf[65] = {0};
    char password_hash_buf[65] = {0};
    
    result_params[0].buffer_type = MYSQL_TYPE_LONG;
    result_params[0].buffer = &user_id;
    
    result_params[1].buffer_type = MYSQL_TYPE_STRING;
    result_params[1].buffer = username_buf;
    result_params[1].buffer_length = 64;
    
    result_params[2].buffer_type = MYSQL_TYPE_STRING;
    result_params[2].buffer = password_hash_buf;
    result_params[2].buffer_length = 64;
    
    mysql_stmt_bind_result(stmt, result_params);
    int fetch_ret = mysql_stmt_fetch(stmt);
    
    bool match = false;
    if (fetch_ret == 0) {
        std::string stored_hash(password_hash_buf);
        std::string input_hash = sha256(password);
        match = (stored_hash == input_hash);
        
        if (match) {
            user.user_id = user_id;
            user.username = username_buf;
            user.password_hash = stored_hash;
        }
    }
    
    mysql_free_result(result);
    mysql_stmt_close(stmt);
    
    return match;
}

bool DatabasePool::get_user_by_id(int user_id, User& user) {
    auto conn = get_connection();
    if (!conn) return false;
    
    std::string query = "SELECT user_id, username, avatar_url FROM users WHERE user_id = ?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return false;
    
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_BIND params[1];
    memset(params, 0, sizeof(params));
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer = &user_id;
    
    if (mysql_stmt_bind_param(stmt, params) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_RES* result = mysql_stmt_result_metadata(stmt);
    if (!result) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_BIND result_params[3];
    memset(result_params, 0, sizeof(result_params));
    
    char username_buf[65] = {0};
    char avatar_url_buf[257] = {0};
    
    result_params[0].buffer_type = MYSQL_TYPE_LONG;
    result_params[0].buffer = &user.user_id;
    
    result_params[1].buffer_type = MYSQL_TYPE_STRING;
    result_params[1].buffer = username_buf;
    result_params[1].buffer_length = 64;
    
    result_params[2].buffer_type = MYSQL_TYPE_STRING;
    result_params[2].buffer = avatar_url_buf;
    result_params[2].buffer_length = 256;
    
    mysql_stmt_bind_result(stmt, result_params);
    int ret = mysql_stmt_fetch(stmt);
    
    if (ret == 0) {
        user.username = username_buf;
        user.avatar_url = avatar_url_buf;
    }
    
    mysql_free_result(result);
    mysql_stmt_close(stmt);
    
    return ret == 0;
}

bool DatabasePool::get_user_by_name(const std::string& username, User& user) {
    auto conn = get_connection();
    if (!conn) return false;
    
    std::string query = "SELECT user_id, username, avatar_url FROM users WHERE username = ?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return false;
    
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_BIND params[1];
    memset(params, 0, sizeof(params));
    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = (char*)username.c_str();
    params[0].buffer_length = username.length();
    
    if (mysql_stmt_bind_param(stmt, params) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_RES* result = mysql_stmt_result_metadata(stmt);
    if (!result) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_BIND result_params[3];
    memset(result_params, 0, sizeof(result_params));
    
    char username_buf[65] = {0};
    char avatar_url_buf[257] = {0};
    
    result_params[0].buffer_type = MYSQL_TYPE_LONG;
    result_params[0].buffer = &user.user_id;
    
    result_params[1].buffer_type = MYSQL_TYPE_STRING;
    result_params[1].buffer = username_buf;
    result_params[1].buffer_length = 64;
    
    result_params[2].buffer_type = MYSQL_TYPE_STRING;
    result_params[2].buffer = avatar_url_buf;
    result_params[2].buffer_length = 256;
    
    mysql_stmt_bind_result(stmt, result_params);
    int ret = mysql_stmt_fetch(stmt);
    
    if (ret == 0) {
        user.username = username_buf;
        user.avatar_url = avatar_url_buf;
    }
    
    mysql_free_result(result);
    mysql_stmt_close(stmt);
    
    return ret == 0;
}

bool DatabasePool::get_friends(int user_id, std::vector<Friend>& friends) {
    auto conn = get_connection();
    if (!conn) return false;
    
    std::string query = "SELECT f.friend_id, u.username, u.avatar_url "
                        "FROM friends f JOIN users u ON f.friend_id = u.user_id "
                        "WHERE f.user_id = ? AND f.status = 1";
    
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return false;
    
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_BIND params[1];
    memset(params, 0, sizeof(params));
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer = &user_id;
    
    if (mysql_stmt_bind_param(stmt, params) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_RES* result = mysql_stmt_result_metadata(stmt);
    if (!result) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_BIND result_params[3];
    memset(result_params, 0, sizeof(result_params));
    
    int friend_id = 0;
    char username_buf[65] = {0};
    char avatar_url_buf[257] = {0};
    
    result_params[0].buffer_type = MYSQL_TYPE_LONG;
    result_params[0].buffer = &friend_id;
    
    result_params[1].buffer_type = MYSQL_TYPE_STRING;
    result_params[1].buffer = username_buf;
    result_params[1].buffer_length = 64;
    
    result_params[2].buffer_type = MYSQL_TYPE_STRING;
    result_params[2].buffer = avatar_url_buf;
    result_params[2].buffer_length = 256;
    
    mysql_stmt_bind_result(stmt, result_params);
    
    while (mysql_stmt_fetch(stmt) == 0) {
        Friend f;
        f.user_id = user_id;
        f.friend_id = friend_id;
        f.friend_name = username_buf;
        f.avatar_url = avatar_url_buf;
        friends.push_back(f);
    }
    
    mysql_free_result(result);
    mysql_stmt_close(stmt);
    
    return true;
}

bool DatabasePool::send_message(int from_id, int to_id, const std::string& content, int& out_msg_id) {
    auto conn = get_connection();
    if (!conn) return false;
    
    std::string query = "INSERT INTO messages (from_id, to_id, content) VALUES (?, ?, ?)";
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return false;
    
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_BIND params[3];
    memset(params, 0, sizeof(params));
    
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer = &from_id;
    
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = &to_id;
    
    params[2].buffer_type = MYSQL_TYPE_STRING;
    params[2].buffer = (char*)content.c_str();
    params[2].buffer_length = content.length();
    
    if (mysql_stmt_bind_param(stmt, params) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    bool success = (mysql_stmt_execute(stmt) == 0);
    if (success) {
        out_msg_id = static_cast<int>(mysql_stmt_insert_id(stmt));
    }
    mysql_stmt_close(stmt);
    
    return success;
}

bool DatabasePool::get_messages(int user_id, int friend_id, std::vector<Message>& messages, int limit) {
    auto conn = get_connection();
    if (!conn) return false;
    
    std::string query = "SELECT msg_id, from_id, to_id, content, timestamp "
                        "FROM messages "
                        "WHERE (from_id = ? AND to_id = ?) OR (from_id = ? AND to_id = ?) "
                        "ORDER BY timestamp DESC LIMIT ?";
    
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return false;
    
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_BIND params[5];
    memset(params, 0, sizeof(params));
    
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer = &user_id;
    
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = &friend_id;
    
    params[2].buffer_type = MYSQL_TYPE_LONG;
    params[2].buffer = &friend_id;
    
    params[3].buffer_type = MYSQL_TYPE_LONG;
    params[3].buffer = &user_id;
    
    params[4].buffer_type = MYSQL_TYPE_LONG;
    params[4].buffer = &limit;
    
    if (mysql_stmt_bind_param(stmt, params) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_RES* result = mysql_stmt_result_metadata(stmt);
    if (!result) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_BIND result_params[5];
    memset(result_params, 0, sizeof(result_params));
    
    int msg_id = 0, from_id = 0, to_id = 0;
    char content_buf[4096] = {0};
    char timestamp_buf[20] = {0};
    
    result_params[0].buffer_type = MYSQL_TYPE_LONG;
    result_params[0].buffer = &msg_id;
    
    result_params[1].buffer_type = MYSQL_TYPE_LONG;
    result_params[1].buffer = &from_id;
    
    result_params[2].buffer_type = MYSQL_TYPE_LONG;
    result_params[2].buffer = &to_id;
    
    result_params[3].buffer_type = MYSQL_TYPE_STRING;
    result_params[3].buffer = content_buf;
    result_params[3].buffer_length = 4095;
    
    result_params[4].buffer_type = MYSQL_TYPE_STRING;
    result_params[4].buffer = timestamp_buf;
    result_params[4].buffer_length = 19;
    
    mysql_stmt_bind_result(stmt, result_params);
    
    while (mysql_stmt_fetch(stmt) == 0) {
        Message m;
        m.msg_id = msg_id;
        m.from_id = from_id;
        m.to_id = to_id;
        m.content = content_buf;
        m.timestamp = timestamp_buf;
        messages.push_back(m);
    }
    
    std::reverse(messages.begin(), messages.end());
    
    mysql_free_result(result);
    mysql_stmt_close(stmt);
    
    return true;
}

bool DatabasePool::add_user(const std::string& username, const std::string& password_hash) {
    auto conn = get_connection();
    if (!conn) return false;
    
    std::string query = "INSERT INTO users (username, password_hash) VALUES (?, ?)";
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return false;
    
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_BIND params[2];
    memset(params, 0, sizeof(params));
    
    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = (char*)username.c_str();
    params[0].buffer_length = username.length();
    
    params[1].buffer_type = MYSQL_TYPE_STRING;
    params[1].buffer = (char*)password_hash.c_str();
    params[1].buffer_length = password_hash.length();
    
    if (mysql_stmt_bind_param(stmt, params) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    bool success = (mysql_stmt_execute(stmt) == 0);
    mysql_stmt_close(stmt);
    
    return success;
}

bool DatabasePool::add_friend(int user_id, int friend_id) {
    auto conn = get_connection();
    if (!conn) return false;
    
    std::string query = "INSERT INTO friends (user_id, friend_id) VALUES (?, ?)";
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return false;
    
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_BIND params[2];
    memset(params, 0, sizeof(params));
    
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer = &user_id;
    
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = &friend_id;
    
    if (mysql_stmt_bind_param(stmt, params) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    bool success = (mysql_stmt_execute(stmt) == 0);
    mysql_stmt_close(stmt);
    
    return success;
}

bool DatabasePool::delete_friend(int user_id, int friend_id) {
    auto conn = get_connection();
    if (!conn) return false;
    
    std::string query = "DELETE FROM friends WHERE user_id = ? AND friend_id = ?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn.get());
    if (!stmt) return false;
    
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_BIND params[2];
    memset(params, 0, sizeof(params));
    
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer = &user_id;
    
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = &friend_id;
    
    if (mysql_stmt_bind_param(stmt, params) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    bool success = (mysql_stmt_execute(stmt) == 0);
    mysql_stmt_close(stmt);
    
    return success;
}

} // namespace qqchat
