#!/bin/bash

# QQ-Copy 完整一键安装脚本
# 包含服务器和 Android 端的完整部署

set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   QQ-Copy 完整一键安装${NC}"
echo -e "${BLUE}========================================${NC}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 添加执行权限
chmod +x "$SCRIPT_DIR/start-server.sh"
chmod +x "$SCRIPT_DIR/build-android.sh"

# 显示菜单
show_menu() {
    echo ""
    echo "请选择要执行的操作："
    echo "1. 完整安装（服务器 + Android APK）"
    echo "2. 仅安装服务器端"
    echo "3. 仅编译 Android APK"
    echo "4. 查看帮助"
    echo "0. 退出"
    echo ""
}

# 完整安装
full_install() {
    echo -e "${GREEN}开始完整安装...${NC}"
    echo ""
    
    # 先安装服务器
    "$SCRIPT_DIR/start-server.sh" --install
    
    # 再编译 APK
    if [ -d "$SCRIPT_DIR/android" ]; then
        echo ""
        echo -e "${GREEN}现在开始编译 Android APK...${NC}"
        echo -e "${YELLOW}注意: 需要 Android SDK${NC}"
        read -p "是否继续编译 APK? (y/n): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            "$SCRIPT_DIR/build-android.sh"
        fi
    fi
}

# 仅安装服务器
install_server() {
    "$SCRIPT_DIR/start-server.sh"
}

# 仅编译 APK
build_apk_only() {
    "$SCRIPT_DIR/build-android.sh"
}

# 显示帮助
show_help() {
    echo ""
    echo "QQ-Copy 完整一键安装脚本"
    echo ""
    echo "项目结构："
    echo "  ├── start-server.sh    - 服务器端一键启动"
    echo "  ├── build-android.sh   - Android 端一键编译"
    echo "  ├── install.sh         - 本脚本，完整安装"
    echo "  ├── server/            - C++ 后端源码"
    echo "  ├── android/           - Android 客户端源码"
    echo "  └── sql/               - 数据库初始化脚本"
    echo ""
    echo "使用说明："
    echo ""
    echo "方式一：直接运行菜单"
    echo "  ./install.sh"
    echo ""
    echo "方式二：直接调用子脚本"
    echo "  ./start-server.sh      - 启动服务器"
    echo "  ./build-android.sh     - 编译 Android APK"
    echo ""
    echo "子脚本参数："
    echo "  start-server.sh:"
    echo "    --install         仅安装依赖"
    echo "    --build           仅编译"
    echo ""
    echo "  build-android.sh:"
    echo "    --clean           清理项目"
    echo "    --release         编译 Release 版本"
    echo ""
    echo "默认配置："
    echo "  MySQL: qqchat/password"
    echo "  服务器端口: 8888"
    echo "  Redis: localhost:6379"
    echo ""
}

# 主菜单循环
while true; do
    show_menu
    read -p "请输入选项 (0-4): " choice
    
    case $choice in
        1)
            full_install
            break
            ;;
        2)
            install_server
            break
            ;;
        3)
            build_apk_only
            break
            ;;
        4)
            show_help
            ;;
        0)
            echo "退出"
            exit 0
            ;;
        *)
            echo -e "${RED}无效选项${NC}"
            ;;
    esac
done
