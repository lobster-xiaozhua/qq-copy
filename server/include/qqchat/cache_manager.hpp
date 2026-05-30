#ifndef CACHE_MANAGER_HPP
#define CACHE_MANAGER_HPP

#include <hiredis/hiredis.h>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>

namespace qqchat {

class CacheManager {
public:
    explicit CacheManager(const std::string& host, int port, int db = 0);
    ~CacheManager();

    bool set_online(int user_id, bool online);
    bool is_online(int user_id);
    bool get_connection_id(int user_id, std::string& conn_id);
    bool set_connection_id(int user_id, const std::string& conn_id);
    bool remove_connection_id(int user_id);

    bool add_friend(int user_id, int friend_id);
    bool remove_friend(int user_id, int friend_id);
    bool get_friends(int user_id, std::vector<int>& friends);
    bool clear_friends(int user_id);

    bool increment_unread(int user_id);
    bool decrement_unread(int user_id);
    bool get_unread_count(int user_id, int& count);
    bool clear_unread(int user_id);

    bool set_ttl(const std::string& key, int seconds);
    bool delete_key(const std::string& key);
    bool exists_key(const std::string& key);

    template<typename Func>
    bool pipeline(Func&& commands);

private:
    redisContext* ctx_;
    std::mutex mutex_;

    std::string build_key(const std::string& prefix, int user_id);
    redisReply* safe_command(const char* fmt, ...);
    void free_reply(redisReply* r);
};

template<typename Func>
bool CacheManager::pipeline(Func&& commands) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;

    redisReply* reply;
    auto free_all = [&](redisReply* r) {
        if (r) {
            for (size_t i = 0; i < r->elements; ++i) {
                if (r->element[i]) freeReplyObject(r->element[i]);
            }
            freeReplyObject(r);
        }
    };

    if (redisAppendCommand(ctx_, "MULTI") != REDIS_OK) return false;

    commands(*this);

    int status = redisAppendCommand(ctx_, "EXEC");
    if (status != REDIS_OK) return false;

    void* replies[2] = {nullptr, nullptr};
    for (int i = 0; i < 2; ++i) {
        if (redisGetReply(ctx_, &replies[i]) != REDIS_OK) {
            free_all(static_cast<redisReply*>(replies[0]));
            return false;
        }
    }

    free_all(static_cast<redisReply*>(replies[0]));
    free_all(static_cast<redisReply*>(replies[1]));
    return true;
}

} // namespace qqchat

#endif // CACHE_MANAGER_HPP
