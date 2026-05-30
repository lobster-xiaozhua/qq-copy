package com.example.qqchat;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;

public class RegisterActivity extends AppCompatActivity implements ChatService.ServiceListener {
    private EditText etUsername;
    private EditText etPassword;
    private EditText etConfirmPassword;
    private Spinner spSecurityQuestion;
    private EditText etSecurityAnswer;
    private Button btnRegister;
    private Button btnBack;
    private ChatService chatService;
    private boolean bound = false;

    private static final String[] DEFAULT_QUESTIONS = {
        "请选择安全问题",
        "您最喜欢的宠物叫什么名字?",
        "您第一所学校叫什么?",
        "您最好朋友的名字是什么?",
        "您父亲的生日月份?",
        "您母亲的生日月份?",
        "您最喜欢的食物是什么?",
        "您的生日是哪天?"
    };

    private ServiceConnection connection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            ChatService.LocalBinder binder = (ChatService.LocalBinder) service;
            chatService = binder.getService();
            chatService.setListener(RegisterActivity.this);
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
        setContentView(R.layout.activity_register);

        etUsername = findViewById(R.id.et_register_username);
        etPassword = findViewById(R.id.et_register_password);
        etConfirmPassword = findViewById(R.id.et_register_confirm_password);
        spSecurityQuestion = findViewById(R.id.sp_security_question);
        etSecurityAnswer = findViewById(R.id.et_security_answer);
        btnRegister = findViewById(R.id.btn_register);
        btnBack = findViewById(R.id.btn_back);

        ArrayAdapter<String> adapter = new ArrayAdapter<>(this,
                android.R.layout.simple_spinner_item, DEFAULT_QUESTIONS);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spSecurityQuestion.setAdapter(adapter);

        btnRegister.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                performRegister();
            }
        });

        btnBack.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                finish();
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

    private void performRegister() {
        if (chatService == null || !chatService.isConnected()) {
            Toast.makeText(this, "正在连接服务器...", Toast.LENGTH_SHORT).show();
            return;
        }

        String username = etUsername.getText().toString().trim();
        String password = etPassword.getText().toString().trim();
        String confirmPassword = etConfirmPassword.getText().toString().trim();
        String question = spSecurityQuestion.getSelectedItem().toString();
        String answer = etSecurityAnswer.getText().toString().trim();

        if (username.isEmpty() || password.isEmpty() || answer.isEmpty()) {
            Toast.makeText(this, "请填写所有必填项", Toast.LENGTH_SHORT).show();
            return;
        }

        if (username.length() < 3 || username.length() > 20) {
            Toast.makeText(this, "用户名长度需在3-20字符之间", Toast.LENGTH_SHORT).show();
            return;
        }

        if (password.length() < 6) {
            Toast.makeText(this, "密码长度不能少于6位", Toast.LENGTH_SHORT).show();
            return;
        }

        if (!password.equals(confirmPassword)) {
            Toast.makeText(this, "两次输入的密码不一致", Toast.LENGTH_SHORT).show();
            return;
        }

        if (spSecurityQuestion.getSelectedItemPosition() == 0) {
            Toast.makeText(this, "请选择安全问题", Toast.LENGTH_SHORT).show();
            return;
        }

        if (answer.length() < 2) {
            Toast.makeText(this, "安全问题答案不能少于2个字符", Toast.LENGTH_SHORT).show();
            return;
        }

        chatService.register(username, password, question, answer);
        btnRegister.setEnabled(false);
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
    public void onRegisterResult(final int errorCode, final int userId) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                btnRegister.setEnabled(true);
                if (errorCode == 0) {
                    Toast.makeText(RegisterActivity.this, "注册成功!您的用户ID是: " + userId, Toast.LENGTH_LONG).show();
                    new android.os.Handler().postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            finish();
                        }
                    }, 2000);
                } else {
                    String message;
                    switch (errorCode) {
                        case 9: message = "用户名已存在"; break;
                        case 5: message = "请求格式错误"; break;
                        case 6: message = "服务器错误"; break;
                        default: message = "注册失败: " + errorCode;
                    }
                    Toast.makeText(RegisterActivity.this, message, Toast.LENGTH_SHORT).show();
                }
            }
        });
    }
}
