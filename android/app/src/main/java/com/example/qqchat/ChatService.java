package com.example.qqchat;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import java.util.ArrayList;
import java.util.List;

public class ChatService extends Service {
    private static final String TAG = "ChatService";
    private static final String SERVER_HOST = "192.168.1.100";
    private static final int SERVER_PORT = 8888;

    private static ChatService instance;
    private final IBinder binder = new LocalBinder();
    private ServiceListener listener;
    private int currentUserId;
    private String currentUsername;

    public interface ServiceListener {
        void onConnectionStatusChanged(int status);
        void onLoginResult(int errorCode, int userId, String username);
        void onMessageReceived(int fromId, String content, long timestamp);
        void onFriendListReceived(int errorCode, List<Integer> friendIds, List<String> friendNames);
        void onRegisterResult(int errorCode, int userId);
        void onSecurityQuestionResult(int errorCode, String question);
        void onResetPasswordResult(int errorCode);
        void onVersionCheckResult(int errorCode, int serverVersion, String updateUrl, String updateDesc);
    }

    public class LocalBinder extends Binder {
        ChatService getService() { return ChatService.this; }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        instance = this;
        nativeInit();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }

    @Override
    public void onDestroy() {
        nativeDestroy();
        instance = null;
        super.onDestroy();
    }

    public static ChatService getInstance() {
        return instance;
    }

    public void setListener(ServiceListener listener) {
        this.listener = listener;
    }

    public void connect() {
        nativeConnect(SERVER_HOST, SERVER_PORT);
    }

    public void disconnect() {
        nativeDisconnect();
    }

    public boolean isConnected() {
        return nativeIsConnected();
    }

    public void login(String username, String password) {
        nativeLogin(username, password);
    }

    public void sendMessage(int toId, String content) {
        nativeSendMessage(toId, content);
    }

    public void getFriendList() {
        nativeGetFriendList();
    }

    public void addFriend(int friendId) {
        nativeAddFriend(friendId);
    }

    public void register(String username, String password, String sq, String sa) {
        nativeRegister(username, password, sq, sa);
    }

    public void getSecurityQuestion(String username) {
        nativeGetSecurityQuestion(username);
    }

    public void resetPassword(String username, String np, String sa) {
        nativeResetPassword(username, np, sa);
    }

    public void checkVersion(int clientVersion) {
        nativeCheckVersion(clientVersion);
    }

    public int getCurrentUserId() {
        return currentUserId;
    }

    public String getCurrentUsername() {
        return currentUsername;
    }

    private void onConnectionStatusChanged(int status) {
        if (listener != null) {
            listener.onConnectionStatusChanged(status);
        }
    }

    private void onLoginResult(int errorCode, int userId, String username) {
        if (errorCode == 0) {
            this.currentUserId = userId;
            this.currentUsername = username;
        }
        if (listener != null) {
            listener.onLoginResult(errorCode, userId, username);
        }
    }

    private void onMessageReceived(int fromId, String content, long timestamp) {
        if (listener != null) {
            listener.onMessageReceived(fromId, content, timestamp);
        }
    }

    private void onFriendListReceived(int errorCode, int[] friendIds, String[] friendNames) {
        if (listener != null) {
            ArrayList<Integer> ids = new ArrayList<>();
            ArrayList<String> names = new ArrayList<>();
            for (int i = 0; i < friendIds.length; i++) {
                ids.add(friendIds[i]);
                names.add(friendNames[i]);
            }
            listener.onFriendListReceived(errorCode, ids, names);
        }
    }

    private void onRegisterResult(int errorCode, int userId) {
        if (listener != null) {
            listener.onRegisterResult(errorCode, userId);
        }
    }

    private void onSecurityQuestionResult(int errorCode, String question) {
        if (listener != null) {
            listener.onSecurityQuestionResult(errorCode, question);
        }
    }

    private void onResetPasswordResult(int errorCode) {
        if (listener != null) {
            listener.onResetPasswordResult(errorCode);
        }
    }

    private void onVersionCheckResult(int errorCode, int sv, String url, String desc) {
        if (listener != null) {
            listener.onVersionCheckResult(errorCode, sv, url, desc);
        }
    }

    static {
        System.loadLibrary("qqchat_core");
    }

    private native void nativeInit();
    private native void nativeConnect(String host, int port);
    private native void nativeDisconnect();
    private native boolean nativeIsConnected();
    private native void nativeLogin(String username, String password);
    private native void nativeSendMessage(int toId, String content);
    private native void nativeGetFriendList();
    private native void nativeAddFriend(int friendId);
    private native void nativeRegister(String u, String p, String sq, String sa);
    private native void nativeGetSecurityQuestion(String username);
    private native void nativeResetPassword(String u, String np, String sa);
    private native void nativeCheckVersion(int clientVersion);
    private native void nativeDestroy();
}
