package com.example.qqchat;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Handler;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;

public class ForgotPasswordActivity extends AppCompatActivity implements ChatService.ServiceListener {
    private EditText etUsername;
    private Button btnNext;
    private Button btnBack;
    private LinearLayout layoutStep1;
    private LinearLayout layoutStep2;
    private TextView tvSecurityQuestion;
    private EditText etSecurityAnswer;
    private EditText etNewPassword;
    private EditText etConfirmPassword;
    private Button btnResetPassword;
    private ChatService chatService;
    private boolean bound = false;
    private String currentQuestion = "";

    private ServiceConnection connection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            ChatService.LocalBinder binder = (ChatService.LocalBinder) service;
            chatService = binder.getService();
            chatService.setListener(ForgotPasswordActivity.this);
            bound = true;
        }

        @Override
        public void onServiceDisconnected(ComponentName arg0) {
            bound = false;
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_forgot_password);

        etUsername = findViewById(R.id.et_forgot_username);
        btnNext = findViewById(R.id.btn_forgot_next);
        btnBack = findViewById(R.id.btn_forgot_back);
        layoutStep1 = findViewById(R.id.layout_forgot_step1);
        layoutStep2 = findViewById(R.id.layout_forgot_step2);
        tvSecurityQuestion = findViewById(R.id.tv_security_question);
        etSecurityAnswer = findViewById(R.id.et_forgot_answer);
        etNewPassword = findViewById(R.id.et_forgot_new_password);
        etConfirmPassword = findViewById(R.id.et_forgot_confirm_password);
        btnResetPassword = findViewById(R.id.btn_reset_password);

        btnNext.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                querySecurityQuestion();
            }
        });

        btnBack.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (layoutStep2.getVisibility() == View.VISIBLE) {
                    layoutStep2.setVisibility(View.GONE);
                    layoutStep1.setVisibility(View.VISIBLE);
                } else {
                    finish();
                }
            }
        });

        btnResetPassword.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                performPasswordReset();
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

    private void querySecurityQuestion() {
        if (chatService == null || !chatService.isConnected()) {
            Toast.makeText(this, "正在连接服务器...", Toast.LENGTH_SHORT).show();
            return;
        }

        String username = etUsername.getText().toString().trim();
        if (username.isEmpty()) {
            Toast.makeText(this, "请输入用户名", Toast.LENGTH_SHORT).show();
            return;
        }

        chatService.getSecurityQuestion(username);
        btnNext.setEnabled(false);
    }

    private void performPasswordReset() {
        String answer = etSecurityAnswer.getText().toString().trim();
        String newPassword = etNewPassword.getText().toString().trim();
        String confirmPassword = etConfirmPassword.getText().toString().trim();

        if (answer.isEmpty() || newPassword.isEmpty()) {
            Toast.makeText(this, "请填写所有必填项", Toast.LENGTH_SHORT).show();
            return;
        }

        if (newPassword.length() < 6) {
            Toast.makeText(this, "密码长度不能少于6位", Toast.LENGTH_SHORT).show();
            return;
        }

        if (!newPassword.equals(confirmPassword)) {
            Toast.makeText(this, "两次输入的密码不一致", Toast.LENGTH_SHORT).show();
            return;
        }

        String username = etUsername.getText().toString().trim();
        chatService.resetPassword(username, newPassword, answer);
        btnResetPassword.setEnabled(false);
    }

    @Override
    public void onConnectionStatusChanged(int status) {}

    @Override
    public void onLoginResult(int errorCode, int userId, String username) {}

    @Override
    public void onMessageReceived(int fromId, String content, long timestamp) {}

    @Override
    public void onFriendListReceived(int errorCode, java.util.List<Integer> friendIds, java.util.List<String> friendNames) {}

    @Override
    public void onSecurityQuestionResult(final int errorCode, final String question) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                btnNext.setEnabled(true);
                if (errorCode == 0) {
                    currentQuestion = question;
                    tvSecurityQuestion.setText(question);
                    layoutStep1.setVisibility(View.GONE);
                    layoutStep2.setVisibility(View.VISIBLE);
                } else {
                    String message;
                    switch (errorCode) {
                        case 1: message = "用户不存在"; break;
                        case 5: message = "请求格式错误"; break;
                        default: message = "查询失败: " + errorCode;
                    }
                    Toast.makeText(ForgotPasswordActivity.this, message, Toast.LENGTH_SHORT).show();
                }
            }
        });
    }

    @Override
    public void onResetPasswordResult(final int errorCode) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                btnResetPassword.setEnabled(true);
                if (errorCode == 0) {
                    Toast.makeText(ForgotPasswordActivity.this, "密码重置成功!", Toast.LENGTH_LONG).show();
                    new Handler().postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            finish();
                        }
                    }, 1500);
                } else {
                    String message;
                    switch (errorCode) {
                        case 1: message = "用户不存在"; break;
                        case 10: message = "安全问题答案错误"; break;
                        case 5: message = "请求格式错误"; break;
                        case 6: message = "服务器错误"; break;
                        default: message = "重置失败: " + errorCode;
                    }
                    Toast.makeText(ForgotPasswordActivity.this, message, Toast.LENGTH_SHORT).show();
                }
            }
        });
    }
}
