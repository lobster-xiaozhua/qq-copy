package com.example.qqchat;

import androidx.appcompat.app.AppCompatActivity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

public class LoginActivity extends AppCompatActivity implements ChatService.ServiceListener {
    private EditText etUsername, etPassword;
    private Button btnLogin, btnRegister, btnForgotPassword;
    private ChatService chatService;
    private boolean bound = false;

    private ServiceConnection connection = new ServiceConnection() {
        @Override public void onServiceConnected(ComponentName cn, IBinder s) {
            chatService = ((ChatService.LocalBinder)s).getService();
            chatService.setListener(LoginActivity.this); bound = true; chatService.connect();
        }
        @Override public void onServiceDisconnected(ComponentName a) { bound = false; }
    };

    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState); setContentView(R.layout.activity_login);
        etUsername = findViewById(R.id.et_username); etPassword = findViewById(R.id.et_password);
        btnLogin = findViewById(R.id.btn_login); btnRegister = findViewById(R.id.btn_register);
        btnForgotPassword = findViewById(R.id.btn_forgot_password);

        btnLogin.setOnClickListener(v -> {
            if (chatService != null && chatService.isConnected()) {
                String u = etUsername.getText().toString().trim(), p = etPassword.getText().toString().trim();
                if (!u.isEmpty() && !p.isEmpty()) { chatService.login(u, p); btnLogin.setEnabled(false); }
                else Toast.makeText(this, "请输入用户名和密码", Toast.LENGTH_SHORT).show();
            } else Toast.makeText(this, "正在连接服务器...", Toast.LENGTH_SHORT).show();
        });
        btnRegister.setOnClickListener(v -> startActivity(new Intent(this, RegisterActivity.class)));
        btnForgotPassword.setOnClickListener(v -> startActivity(new Intent(this, ForgotPasswordActivity.class)));
    }

    @Override protected void onStart() { super.onStart(); bindService(new Intent(this, ChatService.class), connection, Context.BIND_AUTO_CREATE); }
    @Override protected void onStop() { super.onStop(); if (bound) { unbindService(connection); bound = false; } }
    @Override public void onConnectionStatusChanged(int s) {
        runOnUiThread(() -> {
            if (s == 1) { Toast.makeText(this, "连接成功", Toast.LENGTH_SHORT).show(); btnLogin.setEnabled(true); }
            else { Toast.makeText(this, "连接失败", Toast.LENGTH_SHORT).show(); btnLogin.setEnabled(true); }
        });
    }
    @Override public void onLoginResult(int e, int u, String n) {
        runOnUiThread(() -> {
            btnLogin.setEnabled(true);
            if (e == 0) { Intent i = new Intent(this, FriendListActivity.class); i.putExtra("userId", u); i.putExtra("username", n); startActivity(i); finish(); }
            else { String m; switch(e) { case 1: m="用户不存在"; break; case 2: m="密码错误"; break; case 3: m="用户已在线"; break; default: m="登录失败: "+e; } Toast.makeText(this, m, Toast.LENGTH_SHORT).show(); }
        });
    }
    @Override public void onMessageReceived(int f, String c, long t) {}
    @Override public void onFriendListReceived(int e, java.util.List<Integer> ids, java.util.List<String> names) {}
    @Override public void onRegisterResult(int e, int u) {}
    @Override public void onSecurityQuestionResult(int e, String q) {}
    @Override public void onResetPasswordResult(int e) {}
    @Override public void onVersionCheckResult(int e, int sv, String url, String desc) {}
}
