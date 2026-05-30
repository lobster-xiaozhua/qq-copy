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

public class UpdateDownloadDialog extends DialogFragment {
    private DownloadActionListener listener;
    private ProgressBar progressBar;
    private TextView tvProgress;
    private Button btnCancel;

    public interface DownloadActionListener {
        void onCancelled();
    }

    public void setActionListener(DownloadActionListener listener) {
        this.listener = listener;
    }

    @NonNull
    @Override
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
        btnCancel = view.findViewById(R.id.btn_cancel_download);

        btnCancel.setOnClickListener(v -> {
            if (listener != null) listener.onCancelled();
            dismiss();
        });

        dialog.setContentView(view);
        dialog.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);
        return dialog;
    }

    public void updateProgress(int progress) {
        if (getDialog() != null && progressBar != null && tvProgress != null) {
            requireActivity().runOnUiThread(() -> {
                progressBar.setProgress(progress);
                tvProgress.setText(progress + "%");
                if (progress >= 100) {
                    btnCancel.setText("安装");
                }
            });
        }
    }

    public void showSuccess() {
        if (getDialog() != null) {
            requireActivity().runOnUiThread(() -> {
                tvProgress.setText("下载完成");
                progressBar.setProgress(100);
                btnCancel.setText("安装");
            });
        }
    }
}
