package com.example.qqchat.model;

public class Friend {
    private int friendId;
    private String friendName;
    private String avatarUrl;
    private boolean online;
    private int unreadCount;

    public Friend() {}

    public Friend(int friendId, String friendName, String avatarUrl) {
        this.friendId = friendId;
        this.friendName = friendName;
        this.avatarUrl = avatarUrl;
        this.online = false;
        this.unreadCount = 0;
    }

    public int getFriendId() {
        return friendId;
    }

    public void setFriendId(int friendId) {
        this.friendId = friendId;
    }

    public String getFriendName() {
        return friendName;
    }

    public void setFriendName(String friendName) {
        this.friendName = friendName;
    }

    public String getAvatarUrl() {
        return avatarUrl;
    }

    public void setAvatarUrl(String avatarUrl) {
        this.avatarUrl = avatarUrl;
    }

    public boolean isOnline() {
        return online;
    }

    public void setOnline(boolean online) {
        this.online = online;
    }

    public int getUnreadCount() {
        return unreadCount;
    }

    public void setUnreadCount(int unreadCount) {
        this.unreadCount = unreadCount;
    }

    public void incrementUnread() {
        this.unreadCount++;
    }

    public void clearUnread() {
        this.unreadCount = 0;
    }
}
