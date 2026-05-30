#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

NPROC=$(nproc 2>/dev/null || echo 4)

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   QQ-Copy Android 端一键编译${NC}"
echo -e "${BLUE}========================================${NC}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR/android"

check_ccache() {
    echo -e "${GREEN}[1/7] 检查 ccache...${NC}"
    if command -v ccache &> /dev/null; then
        ccache -M 2G &>/dev/null || true
        export USE_CCACHE=1
        export CCACHE_DIR="${CCACHE_DIR:-$HOME/.ccache}"
        echo -e "${GREEN}ccache 已启用${NC}"
    else
        echo -e "${YELLOW}ccache 未安装，跳过${NC}"
    fi
}

check_jdk() {
    echo -e "${GREEN}[2/7] 检查 Java 环境...${NC}"
    
    if ! command -v java &> /dev/null; then
        echo -e "${RED}错误: 未找到 Java${NC}"
        exit 1
    fi
    
    JAVA_MAJOR=$(java -version 2>&1 | head -n 1 | cut -d'"' -f2 | cut -d'.' -f1)
    echo -e "${GREEN}Java 版本: $(java -version 2>&1 | head -n 1)${NC}"
    
    if [ "$JAVA_MAJOR" -ge "17" ]; then
        echo -e "${GREEN}Java 版本符合要求${NC}"
    else
        echo -e "${YELLOW}提示: 建议使用 Java 17 或更高版本${NC}"
    fi
}

check_android_sdk() {
    echo -e "${GREEN}[3/7] 检查 Android SDK...${NC}"
    
    if [ -z "$ANDROID_HOME" ]; then
        echo -e "${YELLOW}ANDROID_HOME 未设置，尝试默认路径...${NC}"
        
        for dir in "$HOME/Android/Sdk" "/opt/android-sdk" "/usr/lib/android-sdk"; do
            if [ -d "$dir" ]; then
                export ANDROID_HOME="$dir"
                echo -e "${GREEN}使用 $ANDROID_HOME${NC}"
                break
            fi
        done
    fi
    
    if [ -z "$ANDROID_HOME" ] || [ ! -d "$ANDROID_HOME" ]; then
        echo -e "${RED}错误: 未找到 Android SDK${NC}"
        echo -e "${YELLOW}请安装 Android SDK 或设置 ANDROID_HOME 环境变量${NC}"
        echo -e "${YELLOW}下载: https://developer.android.com/studio#command-line-tools-only${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}Android SDK: $ANDROID_HOME${NC}"
}

setup_android_ndk() {
    echo -e "${GREEN}[4/7] 检查 Android NDK...${NC}"
    
    if [ -z "$ANDROID_NDK_HOME" ] && [ -d "$ANDROID_HOME/ndk" ]; then
        LATEST_NDK=$(ls -d "$ANDROID_HOME/ndk/"* 2>/dev/null | sort -V | tail -1)
        if [ -n "$LATEST_NDK" ]; then
            export ANDROID_NDK_HOME="$LATEST_NDK"
            echo -e "${GREEN}使用 NDK: $ANDROID_NDK_HOME${NC}"
        fi
    elif [ -n "$ANDROID_NDK_HOME" ]; then
        echo -e "${GREEN}ANDROID_NDK_HOME: $ANDROID_NDK_HOME${NC}"
    else
        echo -e "${YELLOW}警告: 未找到 NDK，NDK 编译可能失败${NC}"
    fi
}

setup_gradle() {
    echo -e "${GREEN}[5/7] 检查 Gradle Wrapper...${NC}"
    
    if [ -f "gradlew" ]; then
        chmod +x gradlew
        echo -e "${GREEN}Gradle Wrapper 已准备${NC}"
    else
        echo -e "${YELLOW}Gradle Wrapper 不存在${NC}"
        if ! command -v gradle &> /dev/null; then
            echo -e "${RED}错误: 未找到 Gradle${NC}"
            exit 1
        fi
        echo -e "${YELLOW}使用系统 Gradle${NC}"
    fi
}

build_apk() {
    echo -e "${GREEN}[6/7] 编译 APK (并行: ${NPROC})...${NC}"
    
    BUILD_CMD="./gradlew assembleDebug"
    if [ "$1" == "--release" ]; then
        BUILD_CMD="./gradlew assembleRelease"
    fi
    
    if [ -f "gradlew" ]; then
        $BUILD_CMD --no-daemon -Pandroid.disableResourceValidation=true 2>&1 | tail -20
    else
        gradle clean assembleDebug --no-daemon 2>&1 | tail -20
    fi
    
    echo -e "${GREEN}编译完成${NC}"
}

find_apk() {
    echo -e "${GREEN}[7/7] 查找 APK 文件...${NC}"
    
    APK_PATH=$(find "$SCRIPT_DIR/android/app/build/outputs/apk" -name "*.apk" -type f 2>/dev/null | head -n 1)
    
    if [ -n "$APK_PATH" ]; then
        APK_SIZE=$(du -h "$APK_PATH" | cut -f1)
        echo -e "${GREEN}========================================${NC}"
        echo -e "${GREEN}  APK 编译成功！${NC}"
        echo -e "${GREEN}  文件: $APK_PATH${NC}"
        echo -e "${GREEN}  大小: $APK_SIZE${NC}"
        echo -e "${GREEN}========================================${NC}"
        
        mkdir -p "$SCRIPT_DIR/outputs"
        APK_NAME=$(basename "$APK_PATH")
        cp "$APK_PATH" "$SCRIPT_DIR/outputs/qq-copy.apk"
        echo -e "${GREEN}已复制到: $SCRIPT_DIR/outputs/qq-copy.apk${NC}"
    else
        echo -e "${RED}错误: 未找到 APK 文件${NC}"
        echo -e "${YELLOW}检查日志以获取更多信息${NC}"
        exit 1
    fi
}

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

main() {
    if [ "$1" == "--help" ]; then
        show_help
        exit 0
    fi
    
    if [ "$1" == "--clean" ]; then
        cd "$SCRIPT_DIR/android"
        if [ -f "gradlew" ]; then
            ./gradlew clean --no-daemon
        fi
        echo -e "${GREEN}项目已清理${NC}"
        exit 0
    fi
    
    check_ccache
    check_jdk
    check_android_sdk
    setup_android_ndk
    setup_gradle
    build_apk "$1"
    find_apk
}

main "$@"
