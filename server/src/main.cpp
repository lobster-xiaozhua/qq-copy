#include "qqchat/chat_server.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <signal.h>

std::shared_ptr<qqchat::ChatServer> g_server;

void signal_handler(int signal) {
    if (g_server) {
        g_server->stop();
        std::cout << "Server stopped." << std::endl;
    }
    exit(0);
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

std::string get_config_value(const std::string& config_file, const std::string& section, const std::string& key) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        std::cerr << "Cannot open config file: " << config_file << std::endl;
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

int main(int argc, char* argv[]) {
    std::string config_file = "config/server.conf";
    if (argc > 1) {
        config_file = argv[1];
    }
    
    std::string host = get_config_value(config_file, "server", "host");
    std::string port_str = get_config_value(config_file, "server", "port");
    short port = port_str.empty() ? 8888 : std::stoi(port_str);
    
    std::string mysql_host = get_config_value(config_file, "mysql", "host");
    std::string mysql_port_str = get_config_value(config_file, "mysql", "port");
    int mysql_port = mysql_port_str.empty() ? 3306 : std::stoi(mysql_port_str);
    std::string mysql_db = get_config_value(config_file, "mysql", "database");
    std::string mysql_user = get_config_value(config_file, "mysql", "username");
    std::string mysql_pass = get_config_value(config_file, "mysql", "password");
    
    std::string redis_host = get_config_value(config_file, "redis", "host");
    std::string redis_port_str = get_config_value(config_file, "redis", "port");
    int redis_port = redis_port_str.empty() ? 6379 : std::stoi(redis_port_str);
    
    std::cout << "========================================" << std::endl;
    std::cout << "  QQChat Server Starting..." << std::endl;
    std::cout << "  Port: " << port << std::endl;
    std::cout << "  MySQL: " << mysql_host << ":" << mysql_port << "/" << mysql_db << std::endl;
    std::cout << "  Redis: " << redis_host << ":" << redis_port << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        auto db_pool = std::make_shared<qqchat::DatabasePool>(
            mysql_host, mysql_port, mysql_db, mysql_user, mysql_pass);
        
        auto cache_manager = std::make_shared<qqchat::CacheManager>(redis_host, redis_port);
        
        g_server = std::make_shared<qqchat::ChatServer>("0.0.0.0", port, db_pool, cache_manager);
        
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        
        std::cout << "Server started successfully!" << std::endl;
        std::cout << "Press Ctrl+C to stop." << std::endl;
        
        g_server->run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
