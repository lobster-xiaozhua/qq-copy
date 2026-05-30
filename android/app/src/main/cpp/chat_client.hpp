#ifndef CHAT_CLIENT_HPP
#define CHAT_CLIENT_HPP

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <thread>
#include <atomic>

namespace qqchat {

class ChatClient {
public:
    using LoginCallback = std::function<void(int error_code, int user_id, const std::string& username)>;
    using MessageCallback = std::function<void(int from_id, const std::string& content, long long timestamp)>;
    using FriendListCallback = std::function<void(int error_code, const std::vector<std::pair<int, std::string>>& friends)>;
    using StatusCallback = std::function<void(int status)>;
    using RegisterCallback = std::function<void(int error_code, int user_id)>;
    using SecurityQuestionCallback = std::function<void(int error_code, const std::string& question)>;
    using ResetPasswordCallback = std::function<void(int error_code)>;
    using VersionCheckCallback = std::function<void(int error_code, int server_version, const std::string& update_url, const std::string& update_desc)>;
    
    ChatClient();
    ~ChatClient();
    
    void connect(const std::string& host, int port);
    void disconnect();
    bool is_connected() const;
    
    void login(const std::string& username, const std::string& password, LoginCallback callback);
    void send_message(int to_id, const std::string& content);
    void get_friend_list(FriendListCallback callback);
    void add_friend(int friend_id);
    void register_user(const std::string& username, const std::string& password,
                      const std::string& security_question, const std::string& security_answer,
                      RegisterCallback callback);
    void get_security_question(const std::string& username, SecurityQuestionCallback callback);
    void reset_password(const std::string& username, const std::string& new_password,
                       const std::string& security_answer, ResetPasswordCallback callback);
    void check_version(int client_version, VersionCheckCallback callback);
    
    void set_message_callback(MessageCallback callback);
    void set_status_callback(StatusCallback callback);
    
private:
    void run();
    void read_loop();
    void handle_packet(const std::vector<uint8_t>& packet);
    
    std::string host_;
    int port_;
    int socket_fd_;
    
    std::thread worker_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> connected_;
    
    std::mutex callback_mutex_;
    LoginCallback login_callback_;
    MessageCallback message_callback_;
    FriendListCallback friend_list_callback_;
    StatusCallback status_callback_;
    RegisterCallback register_callback_;
    SecurityQuestionCallback security_question_callback_;
    ResetPasswordCallback reset_password_callback_;
    VersionCheckCallback version_check_callback_;
    
    std::vector<uint8_t> read_buffer_;
};

} // namespace qqchat

#endif // CHAT_CLIENT_HPP