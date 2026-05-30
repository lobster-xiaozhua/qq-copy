package com.example.qqchat;

import android.app.Dialog;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

public class MirrorSelectionDialog extends DialogFragment {
    private MirrorSelectionListener listener;
    private Handler mainHandler = new Handler(Looper.getMainLooper());
    private List<MirrorItem> mirrors = new ArrayList<>();
    private LinearLayout mirrorContainer;
    private boolean isAutoSelecting = false;

    public interface MirrorSelectionListener {
        void onMirrorSelected(String url, String name);
        void onDismissed();
    }

    public void setMirrorSelectionListener(MirrorSelectionListener l) { this.listener = l; }

    @NonNull @Override
    public Dialog onCreateDialog(@Nullable Bundle savedInstanceState) {
        Dialog dialog = new Dialog(requireContext(), R.style.UpdateDialogTheme);
        dialog.setCancelable(true); dialog.setCanceledOnTouchOutside(true);
        dialog.getWindow().setFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE, WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);

        View view = LayoutInflater.from(getContext()).inflate(R.layout.dialog_mirror_selection, null);
        mirrorContainer = view.findViewById(R.id.mirror_container);
        Button btnCancel = view.findViewById(R.id.btn_cancel);
        Button btnAutoSelect = view.findViewById(R.id.btn_auto_select);

        btnCancel.setOnClickListener(v -> { dismiss(); if (listener != null) listener.onDismissed(); });
        btnAutoSelect.setOnClickListener(v -> autoSelectFastest());

        initMirrors();
        dialog.setContentView(view);
        dialog.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);
        return dialog;
    }

    private void initMirrors() {
        mirrors.clear(); mirrorContainer.removeAllViews();
        String baseUrl = "https://github.com/lobster-xiaozhua/qq-copy/releases/latest/download/qq-copy.apk";
        mirrors.add(new MirrorItem("Chjina 镜像下载", "https://ghproxy.net/" + baseUrl));
        mirrors.add(new MirrorItem("Tbedu 镜像下载", "https://tbedu.top/" + baseUrl));
        mirrors.add(new MirrorItem("Mrhjx 镜像下载", "https://mrhjx.com/" + baseUrl));
        mirrors.add(new MirrorItem("FastGit 镜像下载", "https://fastgit.org/" + baseUrl));
        mirrors.add(new MirrorItem("GitHub原始链接", baseUrl));
        mirrors.add(new MirrorItem("Gh1888866 镜像下载", "https://gh.1888866.xyz/" + baseUrl));
        mirrors.add(new MirrorItem("Llkk 镜像下载", "https://gh.llkk.cc/" + baseUrl));
        mirrors.add(new MirrorItem("BokiMoe 镜像下载", "https://boki.moe/" + baseUrl));
        mirrors.add(new MirrorItem("Cxxpro 镜像下载", "https://cxxpro.cn/" + baseUrl));

        for (MirrorItem m : mirrors) {
            View itemView = LayoutInflater.from(getContext()).inflate(R.layout.item_mirror, mirrorContainer, false);
            TextView tvName = itemView.findViewById(R.id.tv_mirror_name);
            TextView tvSpeed = itemView.findViewById(R.id.tv_mirror_speed);
            ProgressBar pbSpeed = itemView.findViewById(R.id.pb_speed);
            tvName.setText(m.name); pbSpeed.setVisibility(View.VISIBLE); tvSpeed.setVisibility(View.GONE);
            itemView.setOnClickListener(v -> {
                if (!isAutoSelecting) { dismiss(); if (listener != null) listener.onMirrorSelected(m.url, m.name); }
            });
            mirrorContainer.addView(itemView);
            testMirrorSpeed(m, tvSpeed, pbSpeed);
        }
    }

    private void testMirrorSpeed(MirrorItem m, TextView tvSpeed, ProgressBar pbSpeed) {
        new Thread(() -> {
            long start = System.currentTimeMillis();
            try {
                HttpURLConnection conn = (HttpURLConnection) new URL(m.url).openConnection();
                conn.setConnectTimeout(5000); conn.setReadTimeout(5000); conn.setRequestMethod("HEAD");
                int code = conn.getResponseCode(); long elapsed = System.currentTimeMillis() - start; conn.disconnect();
                if (code >= 200 && code < 400) {
                    mainHandler.post(() -> { tvSpeed.setVisibility(View.VISIBLE); pbSpeed.setVisibility(View.GONE);
                        tvSpeed.setText(elapsed + "ms"); tvSpeed.setTextColor(0xFF4CAF50); });
                    m.speed = elapsed;
                } else {
                    mainHandler.post(() -> { tvSpeed.setVisibility(View.VISIBLE); pbSpeed.setVisibility(View.GONE);
                        tvSpeed.setText("超时"); tvSpeed.setTextColor(0xFFFF4444); });
                }
            } catch (Exception e) {
                mainHandler.post(() -> { tvSpeed.setVisibility(View.VISIBLE); pbSpeed.setVisibility(View.GONE);
                    tvSpeed.setText("不可用"); tvSpeed.setTextColor(0xFFFF4444); });
            }
        }).start();
    }

    private void autoSelectFastest() {
        isAutoSelecting = true;
        new Thread(() -> {
            try { Thread.sleep(3000); } catch (InterruptedException e) {}
            MirrorItem fastest = null; long minSpeed = Long.MAX_VALUE;
            for (MirrorItem m : mirrors) { if (m.speed > 0 && m.speed < minSpeed) { minSpeed = m.speed; fastest = m; } }
            if (fastest != null) {
                mainHandler.post(() -> { dismiss(); isAutoSelecting = false;
                    if (listener != null) listener.onMirrorSelected(fastest.url, fastest.name); });
            }
        }).start();
    }

    private static class MirrorItem { String name, url; long speed = -1; MirrorItem(String n, String u) { name = n; url = u; } }
}
