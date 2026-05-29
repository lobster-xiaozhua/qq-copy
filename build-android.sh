#!/bin/bash

# QQ-Copy Android 端一键编译脚本
# 包含依赖配置、编译、打包

set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   QQ-Copy Android 端一键编译${NC}"
echo -e "${BLUE}========================================${NC}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/android"

# 1. 检查 JDK
check_jdk() {
    echo -e "${GREEN}[1/6] 检查 Java 环境...${NC}"
    
    if command -v java &> /dev/null; then
        JAVA_VERSION=$(java -version 2>&1 | head -n 1 | cut -d'"' -f2 | cut -d'.' -f1)
        echo -e "${GREEN}Java 版本: $(java -version 2>&1 | head -n 1)${NC}"
        
        if [ "$JAVA_VERSION" -ge "17" ]; then
            echo -e "${GREEN}Java 版本符合要求${NC}"
        else
            echo -e "${YELLOW}提示: 建议使用 Java 17 或更高版本${NC}"
        fi
    else
        echo -e "${RED}错误: 未找到 Java${NC}"
        exit 1
    fi
}

# 2. 检查 Android SDK
check_android_sdk() {
    echo -e "${GREEN}[2/6] 检查 Android SDK...${NC}"
    
    if [ -z "$ANDROID_HOME" ]; then
        echo -e "${YELLOW}ANDROID_HOME 未设置，尝试默认路径...${NC}"
        
        if [ -d "$HOME/Android/Sdk" ]; then
            export ANDROID_HOME="$HOME/Android/Sdk"
            echo -e "${GREEN}使用 $ANDROID_HOME${NC}"
        elif [ -d "/opt/android-sdk" ]; then
            export ANDROID_HOME="/opt/android-sdk"
            echo -e "${GREEN}使用 $ANDROID_HOME${NC}"
        else
            echo -e "${RED}错误: 未找到 Android SDK${NC}"
            echo -e "${YELLOW}请设置 ANDROID_HOME 环境变量${NC}"
            exit 1
        fi
    fi
    
    if [ ! -d "$ANDROID_HOME" ]; then
        echo -e "${RED}错误: Android SDK 目录不存在: $ANDROID_HOME${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}Android SDK 检查通过${NC}"
}

# 3. 配置 Gradle 使用阿里镜像
setup_gradle_mirrors() {
    echo -e "${GREEN}[3/6] 配置 Gradle 镜像...${NC}"
    
    if [ ! -f "build.gradle" ]; then
        echo -e "${RED}错误: 未找到 build.gradle${NC}"
        exit 1
    fi
    
    # 备份原始配置
    if [ -f "build.gradle" ] && [ ! -f "build.gradle.backup" ]; then
        cp build.gradle build.gradle.backup
    fi
    
    if [ -f "settings.gradle" ] && [ ! -f "settings.gradle.backup" ]; then
        cp settings.gradle settings.gradle.backup
    fi
    
    # 配置 gradle.properties 使用本地仓库
    if [ -f "gradle.properties" ]; then
        echo -e "${GREEN}已配置本地 Maven 仓库${NC}"
    fi
    
    echo -e "${GREEN}Gradle 镜像配置完成${NC}"
}

# 4. 检查 Gradle wrapper
setup_gradle_wrapper() {
    echo -e "${GREEN}[4/6] 检查 Gradle Wrapper...${NC}"
    
    if [ -f "gradlew" ]; then
        chmod +x gradlew
        echo -e "${GREEN}Gradle Wrapper 已准备${NC}"
    else
        echo -e "${YELLOW}Gradle Wrapper 不存在，使用系统 Gradle${NC}"
    fi
}

# 5. 编译 APK
build_apk() {
    echo -e "${GREEN}[5/6] 编译 APK...${NC}"
    
    if [ -f "gradlew" ]; then
        ./gradlew clean assembleDebug
    else
        echo -e "${YELLOW}使用系统 Gradle 编译${NC}"
        if command -v gradle &> /dev/null; then
            gradle clean assembleDebug
        else
            echo -e "${RED}错误: 未找到 Gradle${NC}"
            exit 1
        fi
    fi
    
    echo -e "${GREEN}编译完成${NC}"
}

# 6. 查找并显示 APK
find_apk() {
    echo -e "${GREEN}[6/6] 查找 APK 文件...${NC}"
    
    APK_PATH=$(find "$SCRIPT_DIR/android/app/build/outputs/apk" -name "*.apk" -type f 2>/dev/null | head -n 1)
    
    if [ -n "$APK_PATH" ]; then
        APK_SIZE=$(du -h "$APK_PATH" | cut -f1)
        echo -e "${GREEN}========================================${NC}"
        echo -e "${GREEN}  APK 编译成功！${NC}"
        echo -e "${GREEN}  文件: $APK_PATH${NC}"
        echo -e "${GREEN}  大小: $APK_SIZE${NC}"
        echo -e "${GREEN}========================================${NC}"
        
        # 复制到根目录方便访问
        if [ ! -d "$SCRIPT_DIR/outputs" ]; then
            mkdir -p "$SCRIPT_DIR/outputs"
        fi
        
        APK_NAME=$(basename "$APK_PATH")
        cp "$APK_PATH" "$SCRIPT_DIR/outputs/qq-copy.apk"
        echo -e "${GREEN}已复制到: $SCRIPT_DIR/outputs/qq-copy.apk${NC}"
    else
        echo -e "${RED}错误: 未找到 APK 文件${NC}"
        echo -e "${YELLOW}检查日志以获取更多信息${NC}"
        exit 1
    fi
}

# 显示帮助
show_help() {
    echo "QQ-Copy Android 端一键编译脚本"
    echo ""
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  --clean     清理项目"
    echo "  --release   编译 Release 版本"
    echo "  --help      显示帮助信息"
    echo ""
}

# 主函数
main() {
    if [ "$1" == "--help" ]; then
        show_help
        exit 0
    fi
    
    if [ "$1" == "--clean" ]; then
        cd "$SCRIPT_DIR/android"
        if [ -f "gradlew" ]; then
            ./gradlew clean
        fi
        echo -e "${GREEN}项目已清理${NC}"
        exit 0
    fi
    
    # 默认流程
    check_jdk
    check_android_sdk
    setup_gradle_mirrors
    setup_gradle_wrapper
    build_apk
    find_apk
}

main "$@"
