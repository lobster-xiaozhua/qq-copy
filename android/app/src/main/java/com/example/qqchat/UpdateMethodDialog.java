package com.example.qqchat;

import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;

public class UpdateMethodDialog extends DialogFragment {
    private MethodActionListener listener;

    public interface MethodActionListener {
        void onInAppSelected();
        void onBrowserSelected();
    }

    public void setActionListener(MethodActionListener l) { this.listener = l; }

    @NonNull @Override
    public Dialog onCreateDialog(@Nullable Bundle savedInstanceState) {
        Dialog dialog = new Dialog(requireContext(), R.style.UpdateDialogTheme);
        dialog.setCancelable(true); dialog.setCanceledOnTouchOutside(false);
        dialog.getWindow().setFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE, WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);

        View view = LayoutInflater.from(getContext()).inflate(R.layout.dialog_update_method, null);
        Button btnInApp = view.findViewById(R.id.btn_in_app);
        Button btnBrowser = view.findViewById(R.id.btn_browser);
        Button btnCancel = view.findViewById(R.id.btn_cancel);

        btnInApp.setOnClickListener(v -> { dismiss(); if (listener != null) listener.onInAppSelected(); });
        btnBrowser.setOnClickListener(v -> { dismiss(); if (listener != null) listener.onBrowserSelected(); });
        btnCancel.setOnClickListener(v -> dismiss());

        dialog.setContentView(view);
        dialog.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);
        return dialog;
    }
}
