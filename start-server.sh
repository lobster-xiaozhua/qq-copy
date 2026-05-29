#!/bin/bash

# QQ-Copy 一键启动脚本
# 服务器端 - 包含依赖安装、数据库初始化、编译启动

set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   QQ-Copy 服务器端一键启动${NC}"
echo -e "${BLUE}========================================${NC}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 1. 检查是否以 root 权限运行
check_root() {
    if [[ $EUID -ne 0 ]]; then
        echo -e "${YELLOW}提示: 需要使用 sudo 安装依赖${NC}"
        NEED_SUDO=1
    else
        NEED_SUDO=0
    fi
}

# 2. 配置阿里镜像源
setup_mirrors() {
    echo -e "${GREEN}[1/8] 配置阿里镜像源...${NC}"
    
    if [ -f /etc/apt/sources.list ]; then
        if [ $NEED_SUDO -eq 1 ]; then
            sudo cp /etc/apt/sources.list /etc/apt/sources.list.backup
            sudo sed -i 's|http://archive.ubuntu.com|https://mirrors.aliyun.com|g' /etc/apt/sources.list
            sudo sed -i 's|http://security.ubuntu.com|https://mirrors.aliyun.com|g' /etc/apt/sources.list
        else
            cp /etc/apt/sources.list /etc/apt/sources.list.backup
            sed -i 's|http://archive.ubuntu.com|https://mirrors.aliyun.com|g' /etc/apt/sources.list
            sed -i 's|http://security.ubuntu.com|https://mirrors.aliyun.com|g' /etc/apt/sources.list
        fi
    fi
    
    if [ -f /etc/debian_version ]; then
        if [ $NEED_SUDO -eq 1 ]; then
            sudo cp /etc/apt/sources.list /etc/apt/sources.list.backup || true
            sudo sed -i 's|http://deb.debian.org|https://mirrors.aliyun.com|g' /etc/apt/sources.list || true
        else
            cp /etc/apt/sources.list /etc/apt/sources.list.backup || true
            sed -i 's|http://deb.debian.org|https://mirrors.aliyun.com|g' /etc/apt/sources.list || true
        fi
    fi
}

# 3. 安装基础依赖
install_dependencies() {
    echo -e "${GREEN}[2/8] 安装系统依赖...${NC}"
    
    if [ $NEED_SUDO -eq 1 ]; then
        sudo apt-get update -y
        sudo apt-get install -y \
            build-essential \
            cmake \
            git \
            curl \
            wget \
            g++ \
            gcc \
            make \
            pkg-config \
            libboost-all-dev \
            libmysqlclient-dev \
            libhiredis-dev \
            mysql-server \
            redis-server
    else
        apt-get update -y
        apt-get install -y \
            build-essential \
            cmake \
            git \
            curl \
            wget \
            g++ \
            gcc \
            make \
            pkg-config \
            libboost-all-dev \
            libmysqlclient-dev \
            libhiredis-dev \
            mysql-server \
            redis-server
    fi
    
    echo -e "${GREEN}基础依赖安装完成${NC}"
}

# 4. 配置数据库
setup_database() {
    echo -e "${GREEN}[3/8] 配置 MySQL 数据库...${NC}"
    
    if [ $NEED_SUDO -eq 1 ]; then
        sudo service mysql start || true
    else
        service mysql start || true
    fi
    
    sleep 2
    
    # 创建数据库和用户
    echo -e "${GREEN}正在初始化数据库...${NC}"
    
    DB_USER="qqchat"
    DB_PASS="password"
    DB_NAME="qqchat"
    
    if [ $NEED_SUDO -eq 1 ]; then
        sudo mysql -e "CREATE DATABASE IF NOT EXISTS $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;" || true
        sudo mysql -e "CREATE USER IF NOT EXISTS '$DB_USER'@'localhost' IDENTIFIED BY '$DB_PASS';" || true
        sudo mysql -e "GRANT ALL PRIVILEGES ON $DB_NAME.* TO '$DB_USER'@'localhost';" || true
        sudo mysql -e "FLUSH PRIVILEGES;" || true
        sudo mysql $DB_NAME < sql/init.sql || true
    else
        mysql -e "CREATE DATABASE IF NOT EXISTS $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;" || true
        mysql -e "CREATE USER IF NOT EXISTS '$DB_USER'@'localhost' IDENTIFIED BY '$DB_PASS';" || true
        mysql -e "GRANT ALL PRIVILEGES ON $DB_NAME.* TO '$DB_USER'@'localhost';" || true
        mysql -e "FLUSH PRIVILEGES;" || true
        mysql $DB_NAME < sql/init.sql || true
    fi
    
    echo -e "${GREEN}数据库初始化完成${NC}"
}

# 5. 启动 Redis
start_redis() {
    echo -e "${GREEN}[4/8] 启动 Redis...${NC}"
    
    if [ $NEED_SUDO -eq 1 ]; then
        sudo service redis-server start || true
    else
        service redis-server start || true
    fi
    
    echo -e "${GREEN}Redis 启动完成${NC}"
}

# 6. 编译服务器
build_server() {
    echo -e "${GREEN}[5/8] 编译服务器...${NC}"
    
    cd "$SCRIPT_DIR/server"
    
    if [ -d "build" ]; then
        rm -rf build
    fi
    
    mkdir -p build
    cd build
    
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j$(nproc)
    
    if [ -f "qqchat_server" ]; then
        echo -e "${GREEN}服务器编译成功${NC}"
    else
        echo -e "${RED}服务器编译失败${NC}"
        exit 1
    fi
    
    cd "$SCRIPT_DIR"
}

# 7. 配置服务器
configure_server() {
    echo -e "${GREEN}[6/8] 配置服务器...${NC}"
    
    cd "$SCRIPT_DIR/server"
    
    if [ -f "config/server.conf" ]; then
        cp config/server.conf config/server.conf.backup 2>/dev/null || true
        
        cat > config/server.conf << 'EOF'
[server]
host = 0.0.0.0
port = 8888
max_connections = 10000

[mysql]
host = localhost
port = 3306
database = qqchat
username = qqchat
password = password

[redis]
host = localhost
port = 6379
db = 0

[logging]
level = INFO
path = /tmp/qqchat/
EOF
        echo -e "${GREEN}服务器配置完成${NC}"
    fi
    
    cd "$SCRIPT_DIR"
}

# 8. 启动服务器
start_server() {
    echo -e "${GREEN}[7/8] 启动服务器...${NC}"
    
    mkdir -p /tmp/qqchat
    
    cd "$SCRIPT_DIR/server/build"
    
    if [ -f "./qqchat_server" ]; then
        echo -e "${GREEN}========================================${NC}"
        echo -e "${GREEN}  服务器即将启动${NC}"
        echo -e "${GREEN}  监听端口: 8888${NC}"
        echo -e "${GREEN}  按 Ctrl+C 停止服务器${NC}"
        echo -e "${GREEN}========================================${NC}"
        
        ./qqchat_server "$SCRIPT_DIR/server/config/server.conf"
    else
        echo -e "${RED}错误: 找不到 qqchat_server 可执行文件${NC}"
        exit 1
    fi
}

# 显示帮助
show_help() {
    echo "QQ-Copy 服务器端一键启动脚本"
    echo ""
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  --install     仅安装依赖，不启动服务"
    echo "  --build       仅编译，不启动服务"
    echo "  --help        显示帮助信息"
    echo ""
}

# 主函数
main() {
    check_root
    
    if [ "$1" == "--help" ]; then
        show_help
        exit 0
    fi
    
    if [ "$1" == "--install" ]; then
        setup_mirrors
        install_dependencies
        setup_database
        start_redis
        configure_server
        echo -e "${GREEN}依赖安装完成！${NC}"
        exit 0
    fi
    
    if [ "$1" == "--build" ]; then
        build_server
        configure_server
        echo -e "${GREEN}编译完成！${NC}"
        exit 0
    fi
    
    # 默认完整流程
    setup_mirrors
    install_dependencies
    setup_database
    start_redis
    build_server
    configure_server
    start_server
}

main "$@"
