#include <jni.h>
#include <android/log.h>
#include <string>
#include <memory>
#include <mutex>
#include "chat_client.hpp"

#define TAG "QQChatNative"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

static std::unique_ptr<qqchat::ChatClient> g_client;
static std::mutex g_mutex;

static JavaVM* g_jvm = nullptr;
static jclass g_service_class = nullptr;
static jobject g_service_obj = nullptr;

static jmethodID g_on_conn_status = nullptr;
static jmethodID g_on_login_result = nullptr;
static jmethodID g_on_message_received = nullptr;
static jmethodID g_on_friend_list_received = nullptr;
static jmethodID g_on_register_result = nullptr;
static jmethodID g_on_security_question_result = nullptr;
static jmethodID g_on_reset_password_result = nullptr;
static jmethodID g_on_version_check_result = nullptr;

static void attach_callbacks(JNIEnv* env, jobject service) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_service_obj = env->NewGlobalRef(service);
    g_service_class = (jclass)env->NewGlobalRef(env->GetObjectClass(service));

    g_on_conn_status = env->GetMethodID(g_service_class, "onConnectionStatusChanged", "(I)V");
    g_on_login_result = env->GetMethodID(g_service_class, "onLoginResult", "(IILjava/lang/String;)V");
    g_on_message_received = env->GetMethodID(g_service_class, "onMessageReceived", "(ILjava/lang/String;J)V");
    g_on_friend_list_received = env->GetMethodID(g_service_class, "onFriendListReceived", "(I[I[Ljava/lang/String;)V");
    g_on_register_result = env->GetMethodID(g_service_class, "onRegisterResult", "(II)V");
    g_on_security_question_result = env->GetMethodID(g_service_class, "onSecurityQuestionResult", "(ILjava/lang/String;)V");
    g_on_reset_password_result = env->GetMethodID(g_service_class, "onResetPasswordResult", "(I)V");
    g_on_version_check_result = env->GetMethodID(g_service_class, "onVersionCheckResult", "(IILjava/lang/String;Ljava/lang/String;)V");
}

static void detach_callbacks(JNIEnv* env) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_service_obj) env->DeleteGlobalRef(g_service_obj);
    if (g_service_class) env->DeleteGlobalRef(g_service_class);
    g_service_obj = nullptr;
    g_service_class = nullptr;
}

static void notify_status(int status) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_jvm || !g_service_obj) return;

    JNIEnv* env = nullptr;
    bool attached = false;
    jint result = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attached = true;
    }
    if (!env) return;

    env->CallVoidMethod(g_service_obj, g_on_conn_status, status);
    if (attached) g_jvm->DetachCurrentThread();
}

static void notify_login(int error_code, int user_id, const std::string& username) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_jvm || !g_service_obj) return;

    JNIEnv* env = nullptr;
    bool attached = false;
    if (g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attached = true;
    }
    if (!env) return;

    jstring j_username = env->NewStringUTF(username.c_str());
    env->CallVoidMethod(g_service_obj, g_on_login_result, error_code, user_id, j_username);
    env->DeleteLocalRef(j_username);
    if (attached) g_jvm->DetachCurrentThread();
}

static void notify_message(int from_id, const std::string& content, long long timestamp) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_jvm || !g_service_obj) return;

    JNIEnv* env = nullptr;
    bool attached = false;
    if (g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attached = true;
    }
    if (!env) return;

    jstring j_content = env->NewStringUTF(content.c_str());
    env->CallVoidMethod(g_service_obj, g_on_message_received, from_id, j_content, (jlong)timestamp);
    env->DeleteLocalRef(j_content);
    if (attached) g_jvm->DetachCurrentThread();
}

static void notify_friend_list(int error_code, const std::vector<std::pair<int, std::string>>& friends) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_jvm || !g_service_obj) return;

    JNIEnv* env = nullptr;
    bool attached = false;
    if (g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attached = true;
    }
    if (!env) return;

    jsize len = (jsize)friends.size();
    jintArray j_ids = env->NewIntArray(len);
    jobjectArray j_names = env->NewObjectArray(len, env->FindClass("java/lang/String"), nullptr);

    jint* ids = new jint[len];
    for (int i = 0; i < len; ++i) {
        ids[i] = friends[i].first;
        jstring name = env->NewStringUTF(friends[i].second.c_str());
        env->SetObjectArrayElement(j_names, i, name);
        env->DeleteLocalRef(name);
    }
    env->SetIntArrayRegion(j_ids, 0, len, ids);
    delete[] ids;

    env->CallVoidMethod(g_service_obj, g_on_friend_list_received, error_code, j_ids, j_names);
    env->DeleteLocalRef(j_ids);
    env->DeleteLocalRef(j_names);
    if (attached) g_jvm->DetachCurrentThread();
}

static void notify_register(int error_code, int user_id) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_jvm || !g_service_obj) return;

    JNIEnv* env = nullptr;
    bool attached = false;
    if (g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attached = true;
    }
    if (!env) return;

    env->CallVoidMethod(g_service_obj, g_on_register_result, error_code, user_id);
    if (attached) g_jvm->DetachCurrentThread();
}

static void notify_security_question(int error_code, const std::string& question) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_jvm || !g_service_obj) return;

    JNIEnv* env = nullptr;
    bool attached = false;
    if (g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attached = true;
    }
    if (!env) return;

    jstring j_question = env->NewStringUTF(question.c_str());
    env->CallVoidMethod(g_service_obj, g_on_security_question_result, error_code, j_question);
    env->DeleteLocalRef(j_question);
    if (attached) g_jvm->DetachCurrentThread();
}

static void notify_reset_password(int error_code) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_jvm || !g_service_obj) return;

    JNIEnv* env = nullptr;
    bool attached = false;
    if (g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attached = true;
    }
    if (!env) return;

    env->CallVoidMethod(g_service_obj, g_on_reset_password_result, error_code);
    if (attached) g_jvm->DetachCurrentThread();
}

static void notify_version_check(int error_code, int server_version, const std::string& update_url, const std::string& update_desc) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_jvm || !g_service_obj) return;

    JNIEnv* env = nullptr;
    bool attached = false;
    if (g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attached = true;
    }
    if (!env) return;

    jstring j_url = env->NewStringUTF(update_url.c_str());
    jstring j_desc = env->NewStringUTF(update_desc.c_str());
    env->CallVoidMethod(g_service_obj, g_on_version_check_result, error_code, server_version, j_url, j_desc);
    env->DeleteLocalRef(j_url);
    env->DeleteLocalRef(j_desc);
    if (attached) g_jvm->DetachCurrentThread();
}

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    g_jvm = vm;
    LOGI("QQChat native library loaded");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL
Java_com_example_qqchat_ChatService_nativeInit(JNIEnv* env, jobject thiz) {
    std::lock_guard<std::mutex> lock(g_mutex);
    LOGI("nativeInit called");
    g_client = std::make_unique<qqchat::ChatClient>();

    g_client->set_status_callback([](int status) {
        notify_status(status);
    });
    g_client->set_message_callback([](int from_id, const std::string& content, long long timestamp) {
        notify_message(from_id, content, timestamp);
    });

    attach_callbacks(env, thiz);
}

JNIEXPORT void JNICALL
Java_com_example_qqchat_ChatService_nativeConnect(JNIEnv* env, jobject, jstring host, jint port) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_client) return;

    const char* host_str = env->GetStringUTFChars(host, nullptr);
    LOGI("Connecting to %s:%d", host_str, port);
    g_client->connect(host_str, port);
    env->ReleaseStringUTFChars(host, host_str);
}

JNIEXPORT void JNICALL
Java_com_example_qqchat_ChatService_nativeDisconnect(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_client) g_client->disconnect();
    LOGI("Disconnected");
}

JNIEXPORT jboolean JNICALL
Java_com_example_qqchat_ChatService_nativeIsConnected(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_client ? (jboolean)g_client->is_connected() : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_example_qqchat_ChatService_nativeLogin(JNIEnv* env, jobject, jstring username, jstring password) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_client) return;

    const char* user = env->GetStringUTFChars(username, nullptr);
    const char* pass = env->GetStringUTFChars(password, nullptr);
    LOGI("Login: %s", user);

    g_client->login(user, pass, [](int error_code, int user_id, const std::string& uname) {
        notify_login(error_code, user_id, uname);
    });

    env->ReleaseStringUTFChars(username, user);
    env->ReleaseStringUTFChars(password, pass);
}

JNIEXPORT void JNICALL
Java_com_example_qqchat_ChatService_nativeSendMessage(JNIEnv* env, jobject, jint to_id, jstring content) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_client) return;

    const char* msg = env->GetStringUTFChars(content, nullptr);
    g_client->send_message(to_id, msg);
    env->ReleaseStringUTFChars(content, msg);
}

JNIEXPORT void JNICALL
Java_com_example_qqchat_ChatService_nativeGetFriendList(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_client) return;

    LOGI("Requesting friend list");
    g_client->get_friend_list([](int error_code, const std::vector<std::pair<int, std::string>>& friends) {
        notify_friend_list(error_code, friends);
    });
}

JNIEXPORT void JNICALL
Java_com_example_qqchat_ChatService_nativeAddFriend(JNIEnv*, jobject, jint friend_id) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_client) {
        LOGI("Adding friend: %d", friend_id);
        g_client->add_friend(friend_id);
    }
}

JNIEXPORT void JNICALL
Java_com_example_qqchat_ChatService_nativeRegister(JNIEnv* env, jobject, jstring username, jstring password,
                                                   jstring security_question, jstring security_answer) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_client) return;

    const char* user = env->GetStringUTFChars(username, nullptr);
    const char* pass = env->GetStringUTFChars(password, nullptr);
    const char* question = env->GetStringUTFChars(security_question, nullptr);
    const char* answer = env->GetStringUTFChars(security_answer, nullptr);
    LOGI("Register: %s", user);

    g_client->register_user(user, pass, question, answer, [](int error_code, int user_id) {
        notify_register(error_code, user_id);
    });

    env->ReleaseStringUTFChars(username, user);
    env->ReleaseStringUTFChars(password, pass);
    env->ReleaseStringUTFChars(security_question, question);
    env->ReleaseStringUTFChars(security_answer, answer);
}

JNIEXPORT void JNICALL
Java_com_example_qqchat_ChatService_nativeGetSecurityQuestion(JNIEnv* env, jobject, jstring username) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_client) return;

    const char* user = env->GetStringUTFChars(username, nullptr);
    LOGI("Get security question: %s", user);

    g_client->get_security_question(user, [](int error_code, const std::string& question) {
        notify_security_question(error_code, question);
    });

    env->ReleaseStringUTFChars(username, user);
}

JNIEXPORT void JNICALL
Java_com_example_qqchat_ChatService_nativeResetPassword(JNIEnv* env, jobject, jstring username,
                                                       jstring new_password, jstring security_answer) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_client) return;

    const char* user = env->GetStringUTFChars(username, nullptr);
    const char* new_pass = env->GetStringUTFChars(new_password, nullptr);
    const char* answer = env->GetStringUTFChars(security_answer, nullptr);
    LOGI("Reset password: %s", user);

    g_client->reset_password(user, new_pass, answer, [](int error_code) {
        notify_reset_password(error_code);
    });

    env->ReleaseStringUTFChars(username, user);
    env->ReleaseStringUTFChars(new_password, new_pass);
    env->ReleaseStringUTFChars(security_answer, answer);
}

JNIEXPORT void JNICALL
Java_com_example_qqchat_ChatService_nativeCheckVersion(JNIEnv* env, jobject, jint client_version) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_client) return;

    LOGI("Checking version: %d", client_version);
    g_client->check_version(client_version, [](int error_code, int server_version, const std::string& update_url, const std::string& update_desc) {
        notify_version_check(error_code, server_version, update_url, update_desc);
    });
}

JNIEXPORT void JNICALL
Java_com_example_qqchat_ChatService_nativeDestroy(JNIEnv* env, jobject) {
    std::lock_guard<std::mutex> lock(g_mutex);
    LOGI("nativeDestroy called");
    detach_callbacks(env);
    g_client.reset();
}

} // extern "C"