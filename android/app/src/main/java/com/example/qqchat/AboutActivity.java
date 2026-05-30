package com.example.qqchat;

import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AlertDialog;

public class AboutActivity extends AppCompatActivity {
    private static final String PREFS = "app_prefs";
    private static final String PREF_BETA = "beta_enabled";
    private static final String PREF_SKIP_VERSION = "skip_version";

    private TextView tvVersion;
    private Switch swBeta;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_about);

        if (getSupportActionBar() != null) {
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
            getSupportActionBar().setTitle("关于");
        }

        tvVersion = findViewById(R.id.tv_version);
        swBeta = findViewById(R.id.sw_beta);

        String versionName = getVersionName();
        String versionCode = String.valueOf(getVersionCode());
        tvVersion.setText("版本: " + versionName + "+" + versionCode);

        SharedPreferences prefs = getSharedPreferences(PREFS, MODE_PRIVATE);
        boolean betaEnabled = prefs.getBoolean(PREF_BETA, false);
        swBeta.setChecked(betaEnabled);

        swBeta.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                prefs.edit().putBoolean(PREF_BETA, isChecked).apply();
                Toast.makeText(AboutActivity.this, 
                    isChecked ? "已加入Beta计划" : "已退出Beta计划", 
                    Toast.LENGTH_SHORT).show();
            }
        });

        findViewById(R.id.btn_check_update).setOnClickListener(v -> checkForUpdate());
        findViewById(R.id.btn_project).setOnClickListener(v -> openUrl("https://github.com/lobster-xiaozhua/qq-copy"));
        findViewById(R.id.btn_star).setOnClickListener(v -> openUrl("https://github.com/lobster-xiaozhua/qq-copy"));
        findViewById(R.id.btn_changelog).setOnClickListener(v -> showChangelog());
        findViewById(R.id.btn_license).setOnClickListener(v -> showLicense());
        findViewById(R.id.btn_contact).setOnClickListener(v -> showContact());
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private String getVersionName() {
        try {
            return getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
        } catch (Exception e) {
            return "1.0";
        }
    }

    private int getVersionCode() {
        try {
            return getPackageManager().getPackageInfo(getPackageName(), 0).versionCode;
        } catch (Exception e) {
            return 1;
        }
    }

    private void checkForUpdate() {
        Toast.makeText(this, "检查更新中...", Toast.LENGTH_SHORT).show();
        if (ChatService.getInstance() != null && ChatService.getInstance().isConnected()) {
            ChatService.getInstance().checkVersion(getVersionCode());
        }
    }

    private void openUrl(String url) {
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
        startActivity(intent);
    }

    private void showChangelog() {
        String text = "更新日志:\n\n" +
            "v1.0.2 (当前版本)\n" +
            "- 修复登录超时问题\n" +
            "- 优化消息发送体验\n\n" +
            "v1.0.1\n" +
            "- 新增好友列表功能\n" +
            "- 新增好友添加功能\n\n" +
            "v1.0.0\n" +
            "- 首次发布";
        new AlertDialog.Builder(this)
            .setTitle("更新日志")
            .setMessage(text)
            .setPositiveButton("确定", null)
            .show();
    }

    private void showLicense() {
        String text = "本项目采用 MIT 许可证\n\n" +
            "Copyright (c) 2024 QQChat\n\n" +
            "Permission is hereby granted, free of charge, to any person obtaining a copy\n" +
            "of this software and associated documentation files (the \"Software\"), to deal\n" +
            "in the Software without restriction...";
        new AlertDialog.Builder(this)
            .setTitle("开源许可声明")
            .setMessage(text)
            .setPositiveButton("确定", null)
            .show();
    }

    private void showContact() {
        new AlertDialog.Builder(this)
            .setTitle("联系方式")
            .setMessage("GitHub: https://github.com/lobster-xiaozhua/qq-copy")
            .setPositiveButton("确定", null)
            .show();
    }
}
