#ifndef CACHE_MANAGER_HPP
#define CACHE_MANAGER_HPP

#include <hiredis/hiredis.h>
#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace qqchat {

class CacheManager {
public:
    CacheManager(const std::string& host, int port, int db = 0);
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

private:
    redisContext* ctx_;
    std::mutex mutex_;
    
    std::string build_key(const std::string& prefix, int user_id);
};

} // namespace qqchat

#endif // CACHE_MANAGER_HPP
