package com.example.qqchat;

import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import java.io.File;
import java.text.DecimalFormat;

public class UpdateDownloadDialog extends DialogFragment {
    private DownloadActionListener listener;
    private ProgressBar progressBar;
    private TextView tvProgress, tvDownloaded, tvTotal, tvSpeed, tvStatus;
    private Button btnCancel, btnInstall;
    private File apkFile;
    private long lastBytes = 0;
    private long lastTime = 0;

    public interface DownloadActionListener {
        void onCancelled();
        void onInstallRequested(File apkFile);
    }

    public void setActionListener(DownloadActionListener listener) {
        this.listener = listener;
    }

    @NonNull @Override
    public Dialog onCreateDialog(@Nullable Bundle savedInstanceState) {
        Dialog dialog = new Dialog(requireContext(), R.style.UpdateDialogTheme);
        dialog.setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);
        dialog.getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
        );

        View view = LayoutInflater.from(getContext()).inflate(R.layout.dialog_download_progress, null);
        progressBar = view.findViewById(R.id.pb_download);
        tvProgress = view.findViewById(R.id.tv_progress);
        tvDownloaded = view.findViewById(R.id.tv_downloaded);
        tvTotal = view.findViewById(R.id.tv_total);
        tvSpeed = view.findViewById(R.id.tv_speed);
        tvStatus = view.findViewById(R.id.tv_download_status);
        btnCancel = view.findViewById(R.id.btn_cancel_download);
        btnInstall = view.findViewById(R.id.btn_install);

        btnCancel.setOnClickListener(v -> {
            if (listener != null) listener.onCancelled();
            dismiss();
        });

        btnInstall.setOnClickListener(v -> {
            if (listener != null && apkFile != null) {
                listener.onInstallRequested(apkFile);
            }
            dismiss();
        });

        dialog.setContentView(view);
        dialog.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);
        return dialog;
    }

    public void updateProgress(int progress, long bytesDownloaded, long bytesTotal) {
        if (getDialog() == null) return;
        requireActivity().runOnUiThread(() -> {
            progressBar.setProgress(progress);
            tvProgress.setText(progress + "%");
            tvDownloaded.setText(formatSize(bytesDownloaded));
            tvTotal.setText(formatSize(bytesTotal));

            long now = System.currentTimeMillis();
            if (now - lastTime >= 1000) {
                double speedMBps = (bytesDownloaded - lastBytes) / 1024.0 / 1024.0 / ((now - lastTime) / 1000.0);
                tvSpeed.setText(String.format("%.1f MB/s", speedMBps));
                lastBytes = bytesDownloaded;
                lastTime = now;
            }

            if (progress >= 100) {
                tvStatus.setText("下载完成");
                btnCancel.setText("取消");
                btnInstall.setVisibility(View.VISIBLE);
            } else {
                tvStatus.setText("正在下载...");
            }
        });
    }

    public void setApkFile(File file) {
        this.apkFile = file;
    }

    private String formatSize(long bytes) {
        if (bytes <= 0) return "0 MB";
        DecimalFormat df = new DecimalFormat("#.##");
        return df.format(bytes / 1024.0 / 1024.0) + " MB";
    }
}
