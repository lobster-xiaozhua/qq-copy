package com.example.qqchat;

import androidx.appcompat.app.AppCompatActivity;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.IBinder;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

public class LoginActivity extends AppCompatActivity implements ChatService.ServiceListener, UpdateDialogFragment.UpdateDismissListener {
    private EditText etUsername;
    private EditText etPassword;
    private Button btnLogin;
    private Button btnRegister;
    private Button btnForgotPassword;
    private ChatService chatService;
    private boolean bound = false;
    private boolean versionChecked = false;
    private boolean updateDialogShown = false;

    private static final int CLIENT_VERSION = 2;

    private ServiceConnection connection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            ChatService.LocalBinder binder = (ChatService.LocalBinder) service;
            chatService = binder.getService();
            chatService.setListener(LoginActivity.this);
            bound = true;
            chatService.connect();
        }

        @Override
        public void onServiceDisconnected(ComponentName arg0) {
            bound = false;
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login);

        etUsername = findViewById(R.id.et_username);
        etPassword = findViewById(R.id.et_password);
        btnLogin = findViewById(R.id.btn_login);
        btnRegister = findViewById(R.id.btn_register);
        btnForgotPassword = findViewById(R.id.btn_forgot_password);

        btnLogin.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (chatService != null && chatService.isConnected()) {
                    String username = etUsername.getText().toString().trim();
                    String password = etPassword.getText().toString().trim();
                    if (!username.isEmpty() && !password.isEmpty()) {
                        chatService.login(username, password);
                        btnLogin.setEnabled(false);
                    } else {
                        Toast.makeText(LoginActivity.this, "请输入用户名和密码", Toast.LENGTH_SHORT).show();
                    }
                } else {
                    Toast.makeText(LoginActivity.this, "正在连接服务器...", Toast.LENGTH_SHORT).show();
                }
            }
        });

        btnRegister.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(LoginActivity.this, RegisterActivity.class);
                startActivity(intent);
            }
        });

        btnForgotPassword.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(LoginActivity.this, ForgotPasswordActivity.class);
                startActivity(intent);
            }
        });
    }

    @Override
    protected void onStart() {
        super.onStart();
        Intent intent = new Intent(this, ChatService.class);
        bindService(intent, connection, Context.BIND_AUTO_CREATE);
    }

    @Override
    protected void onStop() {
        super.onStop();
        if (bound) {
            unbindService(connection);
            bound = false;
        }
    }

    @Override
    public void onConnectionStatusChanged(int status) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (status == 1) {
                    Toast.makeText(LoginActivity.this, "连接成功", Toast.LENGTH_SHORT).show();
                    btnLogin.setEnabled(true);
                    
                    if (!versionChecked) {
                        versionChecked = true;
                        checkAppVersion();
                    }
                } else {
                    Toast.makeText(LoginActivity.this, "连接失败", Toast.LENGTH_SHORT).show();
                    btnLogin.setEnabled(true);
                }
            }
        });
    }

    private void checkAppVersion() {
        if (chatService != null && chatService.isConnected()) {
            chatService.checkVersion(CLIENT_VERSION);
        }
    }

    @Override
    public void onVersionCheckResult(int errorCode, int serverVersion, String updateUrl, String updateDesc) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (errorCode == 0 && serverVersion > CLIENT_VERSION) {
                    if (!updateDialogShown && shouldShowUpdateDialog(serverVersion)) {
                        updateDialogShown = true;
                        showUpdateDialog(serverVersion, updateUrl, updateDesc);
                    }
                }
            }
        });
    }

    private boolean shouldShowUpdateDialog(int serverVersion) {
        SharedPreferences prefs = getSharedPreferences("app_prefs", Context.MODE_PRIVATE);
        int skipVersion = prefs.getInt("skip_version", 0);
        return skipVersion < serverVersion;
    }

    private void showUpdateDialog(int serverVersion, String updateUrl, String updateDesc) {
        UpdateDialogFragment dialog = UpdateDialogFragment.newInstance(serverVersion, updateUrl, updateDesc);
        dialog.setDismissListener(this);
        dialog.show(getSupportFragmentManager(), "update_dialog");
    }

    @Override
    public void onUpdateDialogDismissed() {
    }

    @Override
    public void onLoginResult(final int errorCode, final int userId, final String username) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                btnLogin.setEnabled(true);
                if (errorCode == 0) {
                    Intent intent = new Intent(LoginActivity.this, FriendListActivity.class);
                    intent.putExtra("userId", userId);
                    intent.putExtra("username", username);
                    startActivity(intent);
                    finish();
                } else {
                    String message = "";
                    switch (errorCode) {
                        case 1: message = "用户不存在"; break;
                        case 2: message = "密码错误"; break;
                        case 3: message = "用户已在线"; break;
                        default: message = "登录失败: " + errorCode;
                    }
                    Toast.makeText(LoginActivity.this, message, Toast.LENGTH_SHORT).show();
                }
            }
        });
    }

    @Override
    public void onMessageReceived(int fromId, String content, long timestamp) {}

    @Override
    public void onFriendListReceived(int errorCode, java.util.List<Integer> friendIds, java.util.List<String> friendNames) {}

    @Override
    public void onRegisterResult(int errorCode, int userId) {}

    @Override
    public void onSecurityQuestionResult(int errorCode, String question) {}

    @Override
    public void onResetPasswordResult(int errorCode) {}
}
