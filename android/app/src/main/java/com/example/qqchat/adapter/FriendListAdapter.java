package com.example.qqchat.adapter;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.example.qqchat.R;
import com.example.qqchat.model.Friend;

import java.util.List;

public class FriendListAdapter extends RecyclerView.Adapter<FriendListAdapter.FriendViewHolder> {

    private List<Friend> friendList;
    private OnFriendClickListener listener;

    public interface OnFriendClickListener {
        void onFriendClick(Friend friend);
    }

    public FriendListAdapter(List<Friend> friendList, OnFriendClickListener listener) {
        this.friendList = friendList;
        this.listener = listener;
    }

    @NonNull
    @Override
    public FriendViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.item_friend, parent, false);
        return new FriendViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull FriendViewHolder holder, int position) {
        Friend friend = friendList.get(position);
        holder.bind(friend, listener);
    }

    @Override
    public int getItemCount() {
        return friendList.size();
    }

    public void updateFriends(List<Friend> newFriends) {
        this.friendList = newFriends;
        notifyDataSetChanged();
    }

    public static class FriendViewHolder extends RecyclerView.ViewHolder {
        private ImageView avatar;
        private TextView name;
        private TextView status;
        private TextView unreadCount;

        public FriendViewHolder(@NonNull View itemView) {
            super(itemView);
            avatar = itemView.findViewById(R.id.avatar);
            name = itemView.findViewById(R.id.name);
            status = itemView.findViewById(R.id.status);
            unreadCount = itemView.findViewById(R.id.unread_count);
        }

        public void bind(Friend friend, OnFriendClickListener listener) {
            name.setText(friend.getFriendName());
            
            if (friend.isOnline()) {
                status.setText("在线");
                status.setTextColor(itemView.getContext().getColor(R.color.green));
            } else {
                status.setText("离线");
                status.setTextColor(itemView.getContext().getColor(R.color.gray));
            }

            if (friend.getUnreadCount() > 0) {
                unreadCount.setVisibility(View.VISIBLE);
                unreadCount.setText(String.valueOf(friend.getUnreadCount()));
            } else {
                unreadCount.setVisibility(View.GONE);
            }

            itemView.setOnClickListener(v -> listener.onFriendClick(friend));
        }
    }
}
