package com.example.qqchat;

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;

public class UpdateCheckDialog extends DialogFragment {
    private static final String ARG_SERVER_VERSION = "server_version";
    private static final String ARG_UPDATE_URL = "update_url";
    private static final String ARG_UPDATE_DESC = "update_desc";

    private String updateUrl;
    private String updateDesc;
    private UpdateActionListener listener;

    public interface UpdateActionListener {
        void onDownloadSelected();
        void onBrowserSelected();
        void onDismissed();
    }

    public static UpdateCheckDialog newInstance(int serverVersion, String updateUrl, String updateDesc) {
        UpdateCheckDialog fragment = new UpdateCheckDialog();
        Bundle args = new Bundle();
        args.putInt(ARG_SERVER_VERSION, serverVersion);
        args.putString(ARG_UPDATE_URL, updateUrl);
        args.putString(ARG_UPDATE_DESC, updateDesc);
        fragment.setArguments(args);
        return fragment;
    }

    public void setActionListener(UpdateActionListener listener) {
        this.listener = listener;
    }

    @NonNull
    @Override
    public Dialog onCreateDialog(@Nullable Bundle savedInstanceState) {
        if (getArguments() != null) {
            int serverVersion = getArguments().getInt(ARG_SERVER_VERSION);
            updateUrl = getArguments().getString(ARG_UPDATE_URL);
            updateDesc = getArguments().getString(ARG_UPDATE_DESC);

            Dialog dialog = new Dialog(requireContext(), R.style.UpdateDialogTheme);
            dialog.setCancelable(true);
            dialog.setCanceledOnTouchOutside(false);
            dialog.getWindow().setFlags(
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
            );

            View view = LayoutInflater.from(getContext()).inflate(R.layout.dialog_update_check, null);

            TextView tvTitle = view.findViewById(R.id.tv_update_title);
            TextView tvVersion = view.findViewById(R.id.tv_update_version);
            TextView tvDesc = view.findViewById(R.id.tv_update_desc);
            Button btnClose = view.findViewById(R.id.btn_close);
            Button btnDownload = view.findViewById(R.id.btn_download);

            String currentVersion = UpdateManager.getVersionName(requireContext());
            tvTitle.setText("发现新版本");
            tvVersion.setText(currentVersion + " -> " + serverVersion);
            tvDesc.setText(updateDesc != null ? updateDesc : "有新版本可用，请及时更新以获得更好的体验");

            btnClose.setOnClickListener(v -> {
                dismiss();
                if (listener != null) listener.onDismissed();
            });

            btnDownload.setOnClickListener(v -> {
                dismiss();
                showMethodDialog();
            });

            dialog.setContentView(view);
            dialog.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);
            return dialog;
        }
        return super.onCreateDialog(savedInstanceState);
    }

    private void showMethodDialog() {
        UpdateMethodDialog methodDialog = new UpdateMethodDialog();
        methodDialog.setOnActionListener(new UpdateMethodDialog.MethodActionListener() {
            @Override
            public void onInAppSelected() {
                if (listener != null) listener.onDownloadSelected();
            }

            @Override
            public void onBrowserSelected() {
                if (listener != null) listener.onBrowserSelected();
            }
        });
        methodDialog.show(getParentFragmentManager(), "method_dialog");
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }
}
