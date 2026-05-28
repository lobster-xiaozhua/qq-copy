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
import android.widget.TextView;
import android.widget.Toast;

import com.example.qqchat.adapter.FriendListAdapter;
import com.example.qqchat.model.Friend;

import java.util.ArrayList;
import java.util.List;

public class FriendListActivity extends AppCompatActivity implements ChatService.ServiceListener, FriendListAdapter.OnFriendClickListener {
    private TextView tvTitle;
    private RecyclerView rvFriends;
    private FriendListAdapter adapter;
    private List<Friend> friendList = new ArrayList<>();
    private ChatService chatService;
    private boolean bound = false;
    private int currentUserId;

    private ServiceConnection connection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            ChatService.LocalBinder binder = (ChatService.LocalBinder) service;
            chatService = binder.getService();
            chatService.setListener(FriendListActivity.this);
            bound = true;
            chatService.getFriendList();
        }

        @Override
        public void onServiceDisconnected(ComponentName arg0) {
            bound = false;
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_friend_list);

        tvTitle = findViewById(R.id.tv_title);
        rvFriends = findViewById(R.id.rv_friends);

        Intent intent = getIntent();
        currentUserId = intent.getIntExtra("userId", 0);
        String username = intent.getStringExtra("username");
        
        tvTitle.setText(username + "的好友");

        adapter = new FriendListAdapter(friendList, this);
        rvFriends.setLayoutManager(new LinearLayoutManager(this));
        rvFriends.setAdapter(adapter);
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
    public void onFriendClick(Friend friend) {
        Intent intent = new Intent(this, ChatActivity.class);
        intent.putExtra("friendId", friend.getFriendId());
        intent.putExtra("friendName", friend.getFriendName());
        intent.putExtra("currentUserId", currentUserId);
        startActivity(intent);
    }

    @Override
    public void onConnectionStatusChanged(int status) {}

    @Override
    public void onLoginResult(int errorCode, int userId, String username) {}

    @Override
    public void onMessageReceived(final int fromId, final String content, final long timestamp) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                for (Friend friend : friendList) {
                    if (friend.getFriendId() == fromId) {
                        friend.incrementUnread();
                        adapter.notifyDataSetChanged();
                        break;
                    }
                }
            }
        });
    }

    @Override
    public void onFriendListReceived(final int errorCode, final List<Integer> friendIds, final List<String> friendNames) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (errorCode == 0) {
                    friendList.clear();
                    for (int i = 0; i < friendIds.size(); i++) {
                        friendList.add(new Friend(friendIds.get(i), friendNames.get(i), null));
                    }
                    adapter.notifyDataSetChanged();
                } else {
                    Toast.makeText(FriendListActivity.this, "获取好友列表失败", Toast.LENGTH_SHORT).show();
                }
            }
        });
    }
}
