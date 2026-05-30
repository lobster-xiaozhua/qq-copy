package com.example.qqchat;

import android.app.Dialog;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.TextView;
import android.widget.Button;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;

public class UpdateDialogFragment extends DialogFragment {
    private static final String ARG_SERVER_VERSION = "server_version";
    private static final String ARG_UPDATE_URL = "update_url";
    private static final String ARG_UPDATE_DESC = "update_desc";

    private String updateUrl;
    private String updateDesc;
    private UpdateDismissListener listener;

    public interface UpdateDismissListener {
        void onUpdateDialogDismissed();
    }

    public static UpdateDialogFragment newInstance(int serverVersion, String updateUrl, String updateDesc) {
        UpdateDialogFragment fragment = new UpdateDialogFragment();
        Bundle args = new Bundle();
        args.putInt(ARG_SERVER_VERSION, serverVersion);
        args.putString(ARG_UPDATE_URL, updateUrl);
        args.putString(ARG_UPDATE_DESC, updateDesc);
        fragment.setArguments(args);
        return fragment;
    }

    public void setDismissListener(UpdateDismissListener listener) {
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

            View view = LayoutInflater.from(getContext()).inflate(R.layout.dialog_update, null);

            TextView tvTitle = view.findViewById(R.id.tv_update_title);
            TextView tvDesc = view.findViewById(R.id.tv_update_desc);
            TextView tvVersion = view.findViewById(R.id.tv_update_version);
            Button btnUpdate = view.findViewById(R.id.btn_update_now);
            Button btnSkip = view.findViewById(R.id.btn_skip);

            String currentVersion = getCurrentVersion();
            tvTitle.setText("发现新版本 v" + serverVersion);
            tvVersion.setText("当前版本: v" + currentVersion + " → 最新版本: v" + serverVersion);
            tvDesc.setText(updateDesc != null ? updateDesc : "有新版本可用，请及时更新以获得更好的体验");

            btnUpdate.setOnClickListener(v -> {
                dismiss();
                if (updateUrl != null && !updateUrl.isEmpty()) {
                    Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(updateUrl));
                    startActivity(intent);
                }
            });

            btnSkip.setOnClickListener(v -> {
                dismiss();
                saveSkipVersion(serverVersion);
            });

            dialog.setContentView(view);
            dialog.setOnDismissListener(dialogInterface -> {
                if (listener != null) {
                    listener.onUpdateDialogDismissed();
                }
            });

            dialog.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);
            return dialog;
        }
        return super.onCreateDialog(savedInstanceState);
    }

    private String getCurrentVersion() {
        try {
            PackageInfo packageInfo = requireContext().getPackageManager()
                .getPackageInfo(requireContext().getPackageName(), 0);
            return packageInfo.versionName;
        } catch (PackageManager.NameNotFoundException e) {
            return "1.0";
        }
    }

    private void saveSkipVersion(int version) {
        requireContext().getSharedPreferences("app_prefs", android.content.Context.MODE_PRIVATE)
            .edit()
            .putInt("skip_version", version)
            .apply();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }
}
