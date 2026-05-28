#include "qqchat/cache_manager.hpp"

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

std::string CacheManager::build_key(const std::string& prefix, int user_id) {
    return prefix + ":" + std::to_string(user_id);
}

bool CacheManager::set_online(int user_id, bool online) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;
    
    std::string key = build_key("user", user_id) + ":online";
    redisReply* reply = (redisReply*)redisCommand(ctx_, "SET %s %d", key.c_str(), online ? 1 : 0);
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool CacheManager::is_online(int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;
    
    std::string key = build_key("user", user_id) + ":online";
    redisReply* reply = (redisReply*)redisCommand(ctx_, "GET %s", key.c_str());
    if (!reply) return false;
    
    bool online = false;
    if (reply->type == REDIS_REPLY_STRING && std::string(reply->str) == "1") {
        online = true;
    }
    freeReplyObject(reply);
    return online;
}

bool CacheManager::get_connection_id(int user_id, std::string& conn_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;
    
    std::string key = build_key("user", user_id) + ":conn";
    redisReply* reply = (redisReply*)redisCommand(ctx_, "GET %s", key.c_str());
    if (!reply) return false;
    
    bool success = false;
    if (reply->type == REDIS_REPLY_STRING) {
        conn_id = reply->str;
        success = true;
    }
    freeReplyObject(reply);
    return success;
}

bool CacheManager::set_connection_id(int user_id, const std::string& conn_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;
    
    std::string key = build_key("user", user_id) + ":conn";
    redisReply* reply = (redisReply*)redisCommand(ctx_, "SET %s %s", key.c_str(), conn_id.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool CacheManager::remove_connection_id(int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;
    
    std::string key = build_key("user", user_id) + ":conn";
    redisReply* reply = (redisReply*)redisCommand(ctx_, "DEL %s", key.c_str());
    if (!reply) return false;
    freeReplyObject(reply);
    return true;
}

bool CacheManager::add_friend(int user_id, int friend_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;
    
    std::string key = build_key("user", user_id) + ":friends";
    redisReply* reply = (redisReply*)redisCommand(ctx_, "SADD %s %d", key.c_str(), friend_id);
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool CacheManager::remove_friend(int user_id, int friend_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;
    
    std::string key = build_key("user", user_id) + ":friends";
    redisReply* reply = (redisReply*)redisCommand(ctx_, "SREM %s %d", key.c_str(), friend_id);
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool CacheManager::get_friends(int user_id, std::vector<int>& friends) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;
    
    std::string key = build_key("user", user_id) + ":friends";
    redisReply* reply = (redisReply*)redisCommand(ctx_, "SMEMBERS %s", key.c_str());
    if (!reply || reply->type != REDIS_REPLY_ARRAY) {
        if (reply) freeReplyObject(reply);
        return false;
    }
    
    for (size_t i = 0; i < reply->elements; ++i) {
        if (reply->element[i]->type == REDIS_REPLY_STRING) {
            friends.push_back(std::stoi(reply->element[i]->str));
        }
    }
    
    freeReplyObject(reply);
    return true;
}

bool CacheManager::clear_friends(int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;
    
    std::string key = build_key("user", user_id) + ":friends";
    redisReply* reply = (redisReply*)redisCommand(ctx_, "DEL %s", key.c_str());
    if (!reply) return false;
    freeReplyObject(reply);
    return true;
}

bool CacheManager::increment_unread(int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;
    
    std::string key = build_key("user", user_id) + ":unread";
    redisReply* reply = (redisReply*)redisCommand(ctx_, "INCR %s", key.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool CacheManager::decrement_unread(int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;
    
    std::string key = build_key("user", user_id) + ":unread";
    redisReply* reply = (redisReply*)redisCommand(ctx_, "DECR %s", key.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool CacheManager::get_unread_count(int user_id, int& count) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;
    
    std::string key = build_key("user", user_id) + ":unread";
    redisReply* reply = (redisReply*)redisCommand(ctx_, "GET %s", key.c_str());
    if (!reply) return false;
    
    bool success = false;
    if (reply->type == REDIS_REPLY_STRING) {
        count = std::stoi(reply->str);
        success = true;
    } else if (reply->type == REDIS_REPLY_NIL) {
        count = 0;
        success = true;
    }
    freeReplyObject(reply);
    return success;
}

bool CacheManager::clear_unread(int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;
    
    std::string key = build_key("user", user_id) + ":unread";
    redisReply* reply = (redisReply*)redisCommand(ctx_, "SET %s 0", key.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool CacheManager::set_ttl(const std::string& key, int seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;
    
    redisReply* reply = (redisReply*)redisCommand(ctx_, "EXPIRE %s %d", key.c_str(), seconds);
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool CacheManager::delete_key(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;
    
    redisReply* reply = (redisReply*)redisCommand(ctx_, "DEL %s", key.c_str());
    if (!reply) return false;
    freeReplyObject(reply);
    return true;
}

bool CacheManager::exists_key(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;
    
    redisReply* reply = (redisReply*)redisCommand(ctx_, "EXISTS %s", key.c_str());
    if (!reply) return false;
    
    bool exists = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);
    return exists;
}

} // namespace qqchat
