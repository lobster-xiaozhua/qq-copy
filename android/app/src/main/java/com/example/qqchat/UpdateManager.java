package com.example.qqchat;

import android.app.DownloadManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.util.Log;
import androidx.core.content.FileProvider;
import java.io.File;

public class UpdateManager {
    private static final String TAG = "UpdateManager";
    private static final String APK_DIR = "updates";
    private static final String APK_NAME = "qqchat_update.apk";

    private Context context;
    private DownloadManager downloadManager;
    private long downloadId = -1;
    private DownloadCallback callback;

    public interface DownloadCallback {
        void onProgress(int progress, long bytesDownloaded, long bytesTotal);
        void onSuccess(File apkFile);
        void onFailure(String error);
    }

    public UpdateManager(Context context) {
        this.context = context.getApplicationContext();
        this.downloadManager = (DownloadManager) context.getSystemService(Context.DOWNLOAD_SERVICE);
    }

    public void setCallback(DownloadCallback callback) { this.callback = callback; }

    public void downloadApk(String url) {
        try {
            File dir = new File(context.getExternalFilesDir(null), APK_DIR);
            if (!dir.exists()) dir.mkdirs();
            File apkFile = new File(dir, APK_NAME);
            if (apkFile.exists()) apkFile.delete();

            DownloadManager.Request request = new DownloadManager.Request(Uri.parse(url));
            request.setTitle("QQ聊天 更新下载");
            request.setDescription("正在下载新版本APK...");
            request.setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED);
            request.setDestinationUri(Uri.fromFile(apkFile));
            request.setAllowedOverMetered(true);
            request.setAllowedOverRoaming(false);
            request.setMimeType("application/vnd.android.package-archive");

            downloadId = downloadManager.enqueue(request);
            if (callback != null) checkDownloadProgress();
        } catch (Exception e) {
            Log.e(TAG, "Download failed: " + e.getMessage());
            if (callback != null) callback.onFailure("下载失败: " + e.getMessage());
        }
    }

    private void checkDownloadProgress() {
        new Thread(() -> {
            try {
                while (true) {
                    DownloadManager.Query query = new DownloadManager.Query();
                    query.setFilterById(downloadId);
                    android.database.Cursor cursor = downloadManager.query(query);
                    if (cursor != null && cursor.moveToFirst()) {
                        int status = cursor.getInt(cursor.getColumnIndexOrThrow(DownloadManager.COLUMN_STATUS));
                        if (status == DownloadManager.STATUS_SUCCESSFUL) {
                            String uriString = cursor.getString(cursor.getColumnIndexOrThrow(DownloadManager.COLUMN_LOCAL_URI));
                            File apkFile = new File(Uri.parse(uriString).getPath());
                            long len = apkFile.length();
                            if (callback != null) { callback.onProgress(100, len, len); callback.onSuccess(apkFile); }
                            cursor.close(); break;
                        } else if (status == DownloadManager.STATUS_FAILED) {
                            int reason = cursor.getInt(cursor.getColumnIndexOrThrow(DownloadManager.COLUMN_REASON));
                            if (callback != null) callback.onFailure("下载失败，错误码: " + reason);
                            cursor.close(); break;
                        } else if (status == DownloadManager.STATUS_RUNNING || status == DownloadManager.STATUS_PAUSED) {
                            int bytesDown = cursor.getInt(cursor.getColumnIndexOrThrow(DownloadManager.COLUMN_BYTES_DOWNLOADED_SO_FAR));
                            int bytesTotal = cursor.getInt(cursor.getColumnIndexOrThrow(DownloadManager.COLUMN_TOTAL_SIZE_BYTES));
                            if (bytesTotal > 0 && callback != null) {
                                callback.onProgress((int)((bytesDown * 100L) / bytesTotal), bytesDown, bytesTotal);
                            }
                            Thread.sleep(500);
                        } else { Thread.sleep(500); }
                    } else { if (cursor != null) cursor.close(); break; }
                    if (cursor != null) cursor.close();
                }
            } catch (Exception e) {
                if (callback != null) callback.onFailure("检查进度失败: " + e.getMessage());
            }
        }).start();
    }

    public void cancelDownload() { if (downloadId != -1) { downloadManager.remove(downloadId); downloadId = -1; } }

    public static void installApk(Context context, File apkFile) {
        try {
            Uri apkUri;
            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_GRANT_READ_URI_PERMISSION);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                apkUri = FileProvider.getUriForFile(context, context.getPackageName() + ".fileprovider", apkFile);
            } else {
                apkUri = Uri.fromFile(apkFile);
            }
            intent.setDataAndType(apkUri, "application/vnd.android.package-archive");
            context.startActivity(intent);
        } catch (Exception e) {
            Log.e(TAG, "Install failed: " + e.getMessage());
        }
    }

    public static String getVersionName(Context context) {
        try { return context.getPackageManager().getPackageInfo(context.getPackageName(), 0).versionName;
        } catch (Exception e) { return "1.0"; }
    }

    public static int getVersionCode(Context context) {
        try { return context.getPackageManager().getPackageInfo(context.getPackageName(), 0).versionCode;
        } catch (Exception e) { return 1; }
    }
}
