package com.example.qqchat.model;

public class Message {
    private int msgId;
    private int fromId;
    private int toId;
    private String content;
    private long timestamp;
    private boolean isSelf;

    public Message() {}

    public Message(int fromId, String content, long timestamp, boolean isSelf) {
        this.fromId = fromId;
        this.content = content;
        this.timestamp = timestamp;
        this.isSelf = isSelf;
    }

    public int getMsgId() {
        return msgId;
    }

    public void setMsgId(int msgId) {
        this.msgId = msgId;
    }

    public int getFromId() {
        return fromId;
    }

    public void setFromId(int fromId) {
        this.fromId = fromId;
    }

    public int getToId() {
        return toId;
    }

    public void setToId(int toId) {
        this.toId = toId;
    }

    public String getContent() {
        return content;
    }

    public void setContent(String content) {
        this.content = content;
    }

    public long getTimestamp() {
        return timestamp;
    }

    public void setTimestamp(long timestamp) {
        this.timestamp = timestamp;
    }

    public boolean isSelf() {
        return isSelf;
    }

    public void setSelf(boolean self) {
        isSelf = self;
    }
}
