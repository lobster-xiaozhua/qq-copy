#include "qqchat/cache_manager.hpp"
#include <cstdio>
#include <cstdlib>

namespace qqchat {

CacheManager::CacheManager(const std::string& host, int port, int db) {
    ctx_ = redisConnect(host.c_str(), port);
    if (ctx_ == nullptr || ctx_->err) {
        if (ctx_) {
            redisFree(ctx_);
            ctx_ = nullptr;
        }
    } else {
        redisCommand(ctx_, "SELECT %d", db);
    }
}

CacheManager::~CacheManager() {
    if (ctx_) {
        redisFree(ctx_);
    }
}

redisReply* CacheManager::safe_command(const char* fmt, ...) {
    if (!ctx_) return nullptr;
    va_list args;
    va_start(args, fmt);
    redisReply* reply = (redisReply*)redisCommand(ctx_, fmt, args);
    va_end(args);
    return reply;
}

void CacheManager::free_reply(redisReply* r) {
    if (r) freeReplyObject(r);
}

std::string CacheManager::build_key(const std::string& prefix, int user_id) {
    return prefix + ":" + std::to_string(user_id);
}

bool CacheManager::set_online(int user_id, bool online) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;

    std::string key = build_key("user", user_id) + ":online";
    auto reply = safe_command("SET %s %d", key.c_str(), online ? 1 : 0);
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        free_reply(reply);
        return false;
    }
    free_reply(reply);
    return true;
}

bool CacheManager::is_online(int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;

    std::string key = build_key("user", user_id) + ":online";
    auto reply = safe_command("GET %s", key.c_str());
    if (!reply) return false;

    bool online = (reply->type == REDIS_REPLY_STRING && reply->len == 1 && reply->str[0] == '1');
    free_reply(reply);
    return online;
}

bool CacheManager::get_connection_id(int user_id, std::string& conn_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;

    std::string key = build_key("user", user_id) + ":conn";
    auto reply = safe_command("GET %s", key.c_str());
    if (!reply) return false;

    bool success = (reply->type == REDIS_REPLY_STRING);
    if (success) conn_id.assign(reply->str, reply->len);
    free_reply(reply);
    return success;
}

bool CacheManager::set_connection_id(int user_id, const std::string& conn_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;

    std::string key = build_key("user", user_id) + ":conn";
    auto reply = safe_command("SET %s %s", key.c_str(), conn_id.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        free_reply(reply);
        return false;
    }
    free_reply(reply);
    return true;
}

bool CacheManager::remove_connection_id(int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;

    std::string key = build_key("user", user_id) + ":conn";
    auto reply = safe_command("DEL %s", key.c_str());
    free_reply(reply);
    return true;
}

bool CacheManager::add_friend(int user_id, int friend_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;

    std::string key = build_key("user", user_id) + ":friends";
    auto reply = safe_command("SADD %s %d", key.c_str(), friend_id);
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        free_reply(reply);
        return false;
    }
    free_reply(reply);
    return true;
}

bool CacheManager::remove_friend(int user_id, int friend_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;

    std::string key = build_key("user", user_id) + ":friends";
    auto reply = safe_command("SREM %s %d", key.c_str(), friend_id);
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        free_reply(reply);
        return false;
    }
    free_reply(reply);
    return true;
}

bool CacheManager::get_friends(int user_id, std::vector<int>& friends) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;

    std::string key = build_key("user", user_id) + ":friends";
    auto reply = safe_command("SMEMBERS %s", key.c_str());
    if (!reply || reply->type != REDIS_REPLY_ARRAY) {
        free_reply(reply);
        return false;
    }

    friends.reserve(reply->elements);
    for (size_t i = 0; i < reply->elements; ++i) {
        if (reply->element[i]->type == REDIS_REPLY_STRING) {
            friends.push_back(std::stoi(std::string(reply->element[i]->str, reply->element[i]->len)));
        }
    }

    free_reply(reply);
    return true;
}

bool CacheManager::clear_friends(int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;

    std::string key = build_key("user", user_id) + ":friends";
    auto reply = safe_command("DEL %s", key.c_str());
    free_reply(reply);
    return true;
}

bool CacheManager::increment_unread(int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;

    std::string key = build_key("user", user_id) + ":unread";
    auto reply = safe_command("INCR %s", key.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        free_reply(reply);
        return false;
    }
    free_reply(reply);
    return true;
}

bool CacheManager::decrement_unread(int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;

    std::string key = build_key("user", user_id) + ":unread";
    auto reply = safe_command("DECR %s", key.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        free_reply(reply);
        return false;
    }
    free_reply(reply);
    return true;
}

bool CacheManager::get_unread_count(int user_id, int& count) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;

    std::string key = build_key("user", user_id) + ":unread";
    auto reply = safe_command("GET %s", key.c_str());
    if (!reply) return false;

    bool success = false;
    if (reply->type == REDIS_REPLY_STRING) {
        count = std::stoi(std::string(reply->str, reply->len));
        success = true;
    } else if (reply->type == REDIS_REPLY_NIL) {
        count = 0;
        success = true;
    }
    free_reply(reply);
    return success;
}

bool CacheManager::clear_unread(int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;

    std::string key = build_key("user", user_id) + ":unread";
    auto reply = safe_command("SET %s 0", key.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        free_reply(reply);
        return false;
    }
    free_reply(reply);
    return true;
}

bool CacheManager::set_ttl(const std::string& key, int seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;

    auto reply = safe_command("EXPIRE %s %d", key.c_str(), seconds);
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        free_reply(reply);
        return false;
    }
    free_reply(reply);
    return true;
}

bool CacheManager::delete_key(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;

    auto reply = safe_command("DEL %s", key.c_str());
    free_reply(reply);
    return true;
}

bool CacheManager::exists_key(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;

    auto reply = safe_command("EXISTS %s", key.c_str());
    if (!reply) return false;

    bool exists = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    free_reply(reply);
    return exists;
}

} // namespace qqchat
