package com.example.qqchat;

import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import com.example.qqchat.adapter.ChatMessageAdapter;
import com.example.qqchat.model.Message;

import java.util.ArrayList;
import java.util.List;

public class ChatActivity extends AppCompatActivity implements ChatService.ServiceListener {
    private TextView tvTitle;
    private RecyclerView rvMessages;
    private EditText etMessage;
    private Button btnSend;
    private ChatMessageAdapter adapter;
    private List<Message> messages = new ArrayList<>();
    private ChatService chatService;
    private boolean bound = false;
    private int friendId;
    private String friendName;
    private int currentUserId;

    private ServiceConnection connection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            ChatService.LocalBinder binder = (ChatService.LocalBinder) service;
            chatService = binder.getService();
            chatService.setListener(ChatActivity.this);
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
        setContentView(R.layout.activity_chat);

        tvTitle = findViewById(R.id.tv_title);
        rvMessages = findViewById(R.id.rv_messages);
        etMessage = findViewById(R.id.et_message);
        btnSend = findViewById(R.id.btn_send);

        Intent intent = getIntent();
        friendId = intent.getIntExtra("friendId", 0);
        friendName = intent.getStringExtra("friendName");
        currentUserId = intent.getIntExtra("currentUserId", 0);

        tvTitle.setText(friendName);

        adapter = new ChatMessageAdapter(messages);
        rvMessages.setLayoutManager(new LinearLayoutManager(this));
        rvMessages.setAdapter(adapter);

        btnSend.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String content = etMessage.getText().toString().trim();
                if (!content.isEmpty() && chatService != null) {
                    chatService.sendMessage(friendId, content);
                    
                    Message message = new Message(currentUserId, content, System.currentTimeMillis(), true);
                    adapter.addMessage(message);
                    rvMessages.scrollToPosition(messages.size() - 1);
                    
                    etMessage.setText("");
                }
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
    public void onConnectionStatusChanged(int status) {}

    @Override
    public void onLoginResult(int errorCode, int userId, String username) {}

    @Override
    public void onMessageReceived(final int fromId, final String content, final long timestamp) {
        if (fromId == friendId) {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    Message message = new Message(fromId, content, timestamp, false);
                    adapter.addMessage(message);
                    rvMessages.scrollToPosition(messages.size() - 1);
                }
            });
        }
    }

    @Override
    public void onFriendListReceived(int errorCode, List<Integer> friendIds, List<String> friendNames) {}
}
