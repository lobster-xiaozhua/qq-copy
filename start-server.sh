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

# 获取脚本所在目录的绝对路径
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# 1. 检查是否以 root 权限运行
check_root() {
    if [[ $EUID -ne 0 ]]; then
        echo -e "${YELLOW}提示: 建议使用 sudo 运行此脚本以安装依赖${NC}"
        NEED_SUDO=1
        CMD_PREFIX="sudo"
    else
        NEED_SUDO=0
        CMD_PREFIX=""
    fi
}

# 2. 配置阿里镜像源
setup_mirrors() {
    echo -e "${GREEN}[1/8] 配置阿里镜像源...${NC}"
    
    if [ -f /etc/apt/sources.list ] && [ -d /etc/apt ]; then
        if ! grep -q "mirrors.aliyun.com" /etc/apt/sources.list 2>/dev/null; then
            cp /etc/apt/sources.list /etc/apt/sources.list.backup 2>/dev/null || true
            if [ $NEED_SUDO -eq 1 ]; then
                $CMD_PREFIX sed -i 's|http://archive.ubuntu.com|https://mirrors.aliyun.com|g' /etc/apt/sources.list
                $CMD_PREFIX sed -i 's|http://security.ubuntu.com|https://mirrors.aliyun.com|g' /etc/apt/sources.list
                $CMD_PREFIX sed -i 's|http://deb.debian.org|https://mirrors.aliyun.com|g' /etc/apt/sources.list 2>/dev/null || true
            else
                sed -i 's|http://archive.ubuntu.com|https://mirrors.aliyun.com|g' /etc/apt/sources.list
                sed -i 's|http://security.ubuntu.com|https://mirrors.aliyun.com|g' /etc/apt/sources.list
                sed -i 's|http://deb.debian.org|https://mirrors.aliyun.com|g' /etc/apt/sources.list 2>/dev/null || true
            fi
            echo -e "${GREEN}镜像源已配置为阿里云${NC}"
        else
            echo -e "${GREEN}镜像源已是阿里云${NC}"
        fi
    else
        echo -e "${YELLOW}跳过镜像源配置（无 sources.list 文件）${NC}"
    fi
}

# 3. 安装基础依赖
install_dependencies() {
    echo -e "${GREEN}[2/8] 安装系统依赖...${NC}"
    
    if [ $NEED_SUDO -eq 1 ]; then
        $CMD_PREFIX apt-get update -y
        $CMD_PREFIX apt-get install -y \
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
            redis-server 2>&1 | tail -20
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
            redis-server 2>&1 | tail -20
    fi
    
    echo -e "${GREEN}基础依赖安装完成${NC}"
}

# 4. 配置数据库
setup_database() {
    echo -e "${GREEN}[3/8] 配置 MySQL 数据库...${NC}"
    
    DB_USER="qqchat"
    DB_PASS="password"
    DB_NAME="qqchat"
    
    # 尝试多种方式启动 MySQL
    echo -e "${GREEN}尝试启动 MySQL...${NC}"
    MYSQL_STARTED=0
    
    # 方式1: mysqld 直接启动
    if command -v mysqld &> /dev/null && [ $MYSQL_STARTED -eq 0 ]; then
        echo -e "${YELLOW}尝试直接启动 mysqld...${NC}"
        mkdir -p /var/run/mysqld
        chown mysql:mysql /var/run/mysqld
        mysqld --user=mysql --daemonize 2>/dev/null || true
        sleep 3
        if mysql -e "SELECT 1;" &>/dev/null; then
            MYSQL_STARTED=1
            echo -e "${GREEN}MySQL 已启动 (mysqld 方式)${NC}"
        fi
    fi
    
    # 方式2: service mysql
    if [ $MYSQL_STARTED -eq 0 ]; then
        if command -v service &> /dev/null; then
            $CMD_PREFIX service mysql start 2>/dev/null || \
            $CMD_PREFIX service mysqld start 2>/dev/null || true
            sleep 2
            if mysql -e "SELECT 1;" &>/dev/null; then
                MYSQL_STARTED=1
                echo -e "${GREEN}MySQL 已启动 (service 方式)${NC}"
            fi
        fi
    fi
    
    # 方式3: systemctl
    if [ $MYSQL_STARTED -eq 0 ] && command -v systemctl &> /dev/null; then
        $CMD_PREFIX systemctl start mysql 2>/dev/null || \
        $CMD_PREFIX systemctl start mysqld 2>/dev/null || true
        sleep 2
        if mysql -e "SELECT 1;" &>/dev/null; then
            MYSQL_STARTED=1
            echo -e "${GREEN}MySQL 已启动 (systemctl 方式)${NC}"
        fi
    fi
    
    # 检查 MySQL 是否可用
    if [ $MYSQL_STARTED -eq 0 ]; then
        if ! mysql -e "SELECT 1;" &>/dev/null; then
            echo -e "${RED}错误: MySQL 无法启动${NC}"
            echo -e "${YELLOW}请手动检查 MySQL 安装${NC}"
            echo -e "${YELLOW}提示: 可以运行 'mysqld --user=mysql --daemonize' 启动${NC}"
            return 1
        else
            MYSQL_STARTED=1
            echo -e "${GREEN}MySQL 已在运行${NC}"
        fi
    fi
    
    # 初始化数据库
    echo -e "${GREEN}正在初始化数据库...${NC}"
    
    SQL_FILE="$SCRIPT_DIR/sql/init.sql"
    
    if [ ! -f "$SQL_FILE" ]; then
        echo -e "${RED}错误: 找不到 SQL 文件: $SQL_FILE${NC}"
        return 1
    fi
    
    # 执行 SQL
    (
        echo "CREATE DATABASE IF NOT EXISTS $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
        echo "CREATE USER IF NOT EXISTS '$DB_USER'@'localhost' IDENTIFIED BY '$DB_PASS';"
        echo "GRANT ALL PRIVILEGES ON $DB_NAME.* TO '$DB_USER'@'localhost';"
        echo "FLUSH PRIVILEGES;"
        echo "USE $DB_NAME;"
        cat "$SQL_FILE"
    ) | mysql 2>/dev/null || true
    
    # 验证数据库
    if mysql -e "USE $DB_NAME; SELECT COUNT(*) FROM users;" &>/dev/null; then
        echo -e "${GREEN}数据库初始化成功${NC}"
    else
        echo -e "${YELLOW}数据库可能未完全初始化（非致命错误）${NC}"
    fi
}

# 5. 启动 Redis
start_redis() {
    echo -e "${GREEN}[4/8] 启动 Redis...${NC}"
    
    # 尝试多种方式启动 Redis
    REDIS_STARTED=0
    
    # 方式1: redis-server 直接启动
    if command -v redis-server &> /dev/null && [ $REDIS_STARTED -eq 0 ]; then
        if ! redis-cli ping &>/dev/null; then
            redis-server --daemonize yes 2>/dev/null || true
            sleep 1
        fi
        if redis-cli ping &>/dev/null; then
            REDIS_STARTED=1
            echo -e "${GREEN}Redis 已启动 (直接启动)${NC}"
        fi
    fi
    
    # 方式2: service
    if [ $REDIS_STARTED -eq 0 ]; then
        if command -v service &> /dev/null; then
            $CMD_PREFIX service redis-server start 2>/dev/null || \
            $CMD_PREFIX service redis start 2>/dev/null || true
            sleep 1
            if redis-cli ping &>/dev/null; then
                REDIS_STARTED=1
                echo -e "${GREEN}Redis 已启动 (service 方式)${NC}"
            fi
        fi
    fi
    
    # 方式3: systemctl
    if [ $REDIS_STARTED -eq 0 ] && command -v systemctl &> /dev/null; then
        $CMD_PREFIX systemctl start redis-server 2>/dev/null || \
        $CMD_PREFIX systemctl start redis 2>/dev/null || true
        sleep 1
        if redis-cli ping &>/dev/null; then
            REDIS_STARTED=1
            echo -e "${GREEN}Redis 已启动 (systemctl 方式)${NC}"
        fi
    fi
    
    if [ $REDIS_STARTED -eq 0 ]; then
        echo -e "${YELLOW}Redis 可能未启动，服务器可能无法连接 Redis${NC}"
    else
        echo -e "${GREEN}Redis 启动完成${NC}"
    fi
}

# 6. 编译服务器
build_server() {
    echo -e "${GREEN}[5/8] 编译服务器...${NC}"
    
    SERVER_DIR="$SCRIPT_DIR/server"
    
    if [ ! -d "$SERVER_DIR" ]; then
        echo -e "${RED}错误: 找不到服务器目录: $SERVER_DIR${NC}"
        exit 1
    fi
    
    if [ ! -f "$SERVER_DIR/CMakeLists.txt" ]; then
        echo -e "${RED}错误: 找不到 CMakeLists.txt: $SERVER_DIR/CMakeLists.txt${NC}"
        exit 1
    fi
    
    cd "$SERVER_DIR"
    
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
    
    CONFIG_DIR="$SCRIPT_DIR/server/config"
    
    if [ -d "$CONFIG_DIR" ]; then
        cp "$CONFIG_DIR/server.conf" "$CONFIG_DIR/server.conf.backup" 2>/dev/null || true
        
        cat > "$CONFIG_DIR/server.conf" << 'EOF'
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
    else
        echo -e "${RED}错误: 找不到配置目录: $CONFIG_DIR${NC}"
    fi
}

# 8. 启动服务器
start_server() {
    echo -e "${GREEN}[7/8] 启动服务器...${NC}"
    
    mkdir -p /tmp/qqchat
    
    SERVER_BIN="$SCRIPT_DIR/server/build/qqchat_server"
    
    if [ -f "$SERVER_BIN" ]; then
        echo -e "${GREEN}========================================${NC}"
        echo -e "${GREEN}  服务器即将启动${NC}"
        echo -e "${GREEN}  监听端口: 8888${NC}"
        echo -e "${GREEN}  按 Ctrl+C 停止服务器${NC}"
        echo -e "${GREEN}========================================${NC}"
        
        cd "$SCRIPT_DIR"
        exec "$SERVER_BIN" "$SCRIPT_DIR/server/config/server.conf"
    else
        echo -e "${RED}错误: 找不到 qqchat_server 可执行文件${NC}"
        echo -e "${YELLOW}请先运行: $0 --build${NC}"
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
    echo "示例:"
    echo "  sudo $0              # 完整安装并启动"
    echo "  sudo $0 --install    # 仅安装依赖"
    echo "  $0 --build           # 仅编译服务器"
    echo ""
}

# 主函数
main() {
    check_root
    
    # 解析命令行参数
    if [ "$1" == "--help" ]; then
        show_help
        exit 0
    fi
    
    if [ "$1" == "--install" ]; then
        setup_mirrors
        install_dependencies
        setup_database || true
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
    setup_database || true
    start_redis
    build_server
    configure_server
    start_server
}

main "$@"
