package com.example.qqchat;

import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.Uri;
import android.os.Bundle;
import android.os.IBinder;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TextView;
import android.widget.Toast;
import com.example.qqchat.adapter.FriendListAdapter;
import com.example.qqchat.model.Friend;
import java.io.File;
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
    private UpdateManager updateManager;
    private UpdateDownloadDialog downloadDialog;
    private int pendingServerVersion = 0;
    private String pendingUpdateUrl = "";
    private String pendingUpdateDesc = "";

    private ServiceConnection connection = new ServiceConnection() {
        @Override public void onServiceConnected(ComponentName cn, IBinder s) {
            chatService = ((ChatService.LocalBinder)s).getService();
            chatService.setListener(FriendListActivity.this); bound = true; chatService.getFriendList();
        }
        @Override public void onServiceDisconnected(ComponentName a) { bound = false; }
    };

    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState); setContentView(R.layout.activity_friend_list);
        if (getSupportActionBar() != null) getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        tvTitle = findViewById(R.id.tv_title); rvFriends = findViewById(R.id.rv_friends);
        currentUserId = getIntent().getIntExtra("userId", 0);
        tvTitle.setText(getIntent().getStringExtra("username") + "的好友");
        adapter = new FriendListAdapter(friendList, this);
        rvFriends.setLayoutManager(new LinearLayoutManager(this)); rvFriends.setAdapter(adapter);
        updateManager = new UpdateManager(this);
    }

    @Override public boolean onCreateOptionsMenu(Menu menu) { getMenuInflater().inflate(R.menu.menu_friend_list, menu); return true; }
    @Override public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) { finish(); return true; }
        else if (item.getItemId() == R.id.action_about) { startActivity(new Intent(this, AboutActivity.class)); return true; }
        else if (item.getItemId() == R.id.action_logout) { if (chatService != null) chatService.disconnect(); finish(); return true; }
        return super.onOptionsItemSelected(item);
    }

    @Override protected void onStart() { super.onStart(); bindService(new Intent(this, ChatService.class), connection, Context.BIND_AUTO_CREATE); }
    @Override protected void onStop() { super.onStop(); if (bound) { unbindService(connection); bound = false; } }
    @Override public void onFriendClick(Friend f) {
        Intent i = new Intent(this, ChatActivity.class);
        i.putExtra("friendId", f.getFriendId()); i.putExtra("friendName", f.getFriendName()); i.putExtra("currentUserId", currentUserId);
        startActivity(i);
    }
    @Override public void onConnectionStatusChanged(int s) {}
    @Override public void onLoginResult(int e, int u, String n) {}
    @Override public void onMessageReceived(int from, String c, long t) {
        runOnUiThread(() -> { for (Friend f : friendList) { if (f.getFriendId() == from) { f.incrementUnread(); adapter.notifyDataSetChanged(); break; } } });
    }
    @Override public void onFriendListReceived(int e, List<Integer> ids, List<String> names) {
        runOnUiThread(() -> {
            if (e == 0) { friendList.clear(); for (int i = 0; i < ids.size(); i++) friendList.add(new Friend(ids.get(i), names.get(i), null)); adapter.notifyDataSetChanged(); }
            else Toast.makeText(this, "获取好友列表失败", Toast.LENGTH_SHORT).show();
        });
    }
    @Override public void onRegisterResult(int e, int u) {}
    @Override public void onSecurityQuestionResult(int e, String q) {}
    @Override public void onResetPasswordResult(int e) {}
    @Override public void onVersionCheckResult(int e, int sv, String url, String desc) {
        if (e == 0 && sv > UpdateManager.getVersionCode(this)) {
            pendingServerVersion = sv; pendingUpdateUrl = url; pendingUpdateDesc = desc;
            runOnUiThread(this::showUpdateCheckDialog);
        }
    }

    private void showUpdateCheckDialog() {
        UpdateCheckDialog dialog = UpdateCheckDialog.newInstance(pendingServerVersion, pendingUpdateUrl, pendingUpdateDesc);
        dialog.setActionListener(new UpdateCheckDialog.UpdateActionListener() {
            @Override public void onDownloadSelected() {
                MirrorSelectionDialog md = new MirrorSelectionDialog();
                md.setMirrorSelectionListener(new MirrorSelectionDialog.MirrorSelectionListener() {
                    @Override public void onMirrorSelected(String url, String name) { startInAppDownload(url); }
                    @Override public void onDismissed() {}
                });
                md.show(getSupportFragmentManager(), "mirror_dialog");
            }
            @Override public void onBrowserSelected() { startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(pendingUpdateUrl))); }
            @Override public void onDismissed() {}
        });
        dialog.show(getSupportFragmentManager(), "update_check");
    }

    private void startInAppDownload(String url) {
        downloadDialog = new UpdateDownloadDialog();
        downloadDialog.setActionListener(() -> { if (updateManager != null) updateManager.cancelDownload(); });
        downloadDialog.show(getSupportFragmentManager(), "download_progress");
        updateManager.setCallback(new UpdateManager.DownloadCallback() {
            @Override public void onProgress(int p) { if (downloadDialog != null) downloadDialog.updateProgress(p); }
            @Override public void onSuccess(File apkFile) {
                if (downloadDialog != null) downloadDialog.showSuccess();
                runOnUiThread(() -> { UpdateManager.installApk(FriendListActivity.this, apkFile); if (downloadDialog != null) downloadDialog.dismiss(); });
            }
            @Override public void onFailure(String error) {
                runOnUiThread(() -> { Toast.makeText(FriendListActivity.this, "下载失败: " + error, Toast.LENGTH_LONG).show(); if (downloadDialog != null) downloadDialog.dismiss(); });
            }
        });
        updateManager.downloadApk(url);
    }
}
