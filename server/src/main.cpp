#include "qqchat/chat_server.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <csignal>
#include <chrono>
#include <ctime>
#include <sstream>
#include <memory>

namespace {
    std::string now_str() {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
        return buf;
    }

    void log_message(const std::string& msg, bool is_error = false) {
        std::ostream& os = is_error ? std::cerr : std::cout;
        os << "[" << now_str() << "] " << msg << std::endl;
    }

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }

    int safe_stoi(const std::string& str, int default_val) {
        if (str.empty()) return default_val;
        try {
            size_t pos = 0;
            int val = std::stoi(str, &pos);
            if (pos != str.length()) return default_val;
            return val;
        } catch (...) {
            return default_val;
        }
    }

    std::string get_config_value(const std::string& config_file, const std::string& section, const std::string& key) {
        std::ifstream file(config_file);
        if (!file.is_open()) {
            log_message("Cannot open config file: " + config_file, true);
            return "";
        }
        
        std::string line;
        std::string current_section;
        
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;
            
            if (line[0] == '[' && line.back() == ']') {
                current_section = line.substr(1, line.size() - 2);
                continue;
            }
            
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string k = trim(line.substr(0, pos));
                std::string v = trim(line.substr(pos + 1));
                
                if (current_section == section && k == key) {
                    return v;
                }
            }
        }
        
        return "";
    }

    std::shared_ptr<qqchat::ChatServer> g_server;

    void signal_handler(int signal) {
        log_message("Signal " + std::to_string(signal) + " received, stopping server...");
        if (g_server) {
            g_server->stop();
        }
        std::exit(0);
    }
}

int main(int argc, char* argv[]) {
    std::string config_file = "config/server.conf";
    if (argc > 1) {
        config_file = argv[1];
    }
    
    std::string host = get_config_value(config_file, "server", "host");
    std::string port_str = get_config_value(config_file, "server", "port");
    short port = static_cast<short>(safe_stoi(port_str, 8888));
    
    std::string mysql_host = get_config_value(config_file, "mysql", "host");
    std::string mysql_port_str = get_config_value(config_file, "mysql", "port");
    int mysql_port = safe_stoi(mysql_port_str, 3306);
    std::string mysql_db = get_config_value(config_file, "mysql", "database");
    std::string mysql_user = get_config_value(config_file, "mysql", "username");
    std::string mysql_pass = get_config_value(config_file, "mysql", "password");
    
    std::string redis_host = get_config_value(config_file, "redis", "host");
    std::string redis_port_str = get_config_value(config_file, "redis", "port");
    int redis_port = safe_stoi(redis_port_str, 6379);
    
    log_message("=====================================");
    log_message("  QQChat Server v1.0");
    log_message("  Port: " + std::to_string(port));
    log_message("  MySQL: " + mysql_host + ":" + std::to_string(mysql_port) + "/" + mysql_db);
    log_message("  Redis: " + redis_host + ":" + std::to_string(redis_port));
    log_message("=====================================");
    
    try {
        auto db_pool = std::make_shared<qqchat::DatabasePool>(
            mysql_host, mysql_port, mysql_db, mysql_user, mysql_pass);
        
        auto cache_manager = std::make_shared<qqchat::CacheManager>(redis_host, redis_port);
        
        g_server = std::make_shared<qqchat::ChatServer>(host.empty() ? "0.0.0.0" : host, port, db_pool, cache_manager);
        
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        
        log_message("Server started successfully, listening on " + (host.empty() ? "0.0.0.0" : host) + ":" + std::to_string(port));
        log_message("Press Ctrl+C to stop.");
        
        g_server->run();
    } catch (const std::exception& e) {
        log_message("Server fatal error: " + std::string(e.what()), true);
        return 1;
    }
    
    return 0;
}
