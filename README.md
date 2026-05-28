# QQ-Copy 聊天应用

模仿QQ的聊天应用，使用C++作为核心底层，Android作为客户端。

## 技术栈

### 后端
- C++17
- Boost.Asio (网络通信)
- MySQL (数据存储)
- Redis (缓存)

### Android客户端
- Java + JNI
- C++ 网络客户端核心
- RecyclerView (列表)
- Material Components (UI)

## 功能特性

- 用户登录/注册
- 好友列表
- 实时消息收发
- TCP长连接推送

## 项目结构

```
├── server/          # C++ 后端服务器
│   ├── src/         # 源代码
│   ├── include/     # 头文件
│   └── config/     # 配置文件
├── android/         # Android 客户端
│   └── app/
│       └── src/
│           ├── main/
│           │   ├── cpp/      # C++ JNI 代码
│           │   ├── java/     # Java 代码
│           │   └── res/      # 资源文件
│           └── AndroidManifest.xml
└── sql/             # 数据库脚本
```

## 构建

### 服务器
```bash
cd server
mkdir build && cd build
cmake ..
make
./qqchat_server
```

### Android
```bash
cd android
./gradlew assembleDebug
```

## 许可证

MIT