package com.example.qqchat;

import android.app.DownloadManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
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
        void onProgress(int progress);
        void onSuccess(File apkFile);
        void onFailure(String error);
    }

    public UpdateManager(Context context) {
        this.context = context.getApplicationContext();
        this.downloadManager = (DownloadManager) context.getSystemService(Context.DOWNLOAD_SERVICE);
    }

    public void setCallback(DownloadCallback callback) {
        this.callback = callback;
    }

    public void downloadApk(String url) {
        try {
            File dir = new File(context.getExternalFilesDir(null), APK_DIR);
            if (!dir.exists()) {
                dir.mkdirs();
            }

            File apkFile = new File(dir, APK_NAME);
            if (apkFile.exists()) {
                apkFile.delete();
            }

            Uri uri = Uri.parse(url);
            DownloadManager.Request request = new DownloadManager.Request(uri);
            request.setTitle("QQ聊天 更新下载");
            request.setDescription("正在下载新版本APK...");
            request.setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED);
            request.setDestinationUri(Uri.fromFile(apkFile));
            request.setAllowedOverMetered(true);
            request.setAllowedOverRoaming(false);
            request.setMimeType("application/vnd.android.package-archive");

            downloadId = downloadManager.enqueue(request);
            Log.d(TAG, "Download started, id=" + downloadId);

            if (callback != null) {
                checkDownloadProgress();
            }
        } catch (Exception e) {
            Log.e(TAG, "Download failed: " + e.getMessage());
            if (callback != null) {
                callback.onFailure("下载失败: " + e.getMessage());
            }
        }
    }

    private void checkDownloadProgress() {
        new Thread(() -> {
            try {
                while (true) {
                    DownloadManager.Query query = new DownloadManager.Query();
                    query.setFilterById(downloadId);
                    Cursor cursor = downloadManager.query(query);

                    if (cursor != null && cursor.moveToFirst()) {
                        int status = cursor.getInt(cursor.getColumnIndexOrThrow(DownloadManager.COLUMN_STATUS));
                        
                        if (status == DownloadManager.STATUS_SUCCESSFUL) {
                            String uriString = cursor.getString(cursor.getColumnIndexOrThrow(DownloadManager.COLUMN_LOCAL_URI));
                            File apkFile = new File(Uri.parse(uriString).getPath());
                            if (callback != null) {
                                callback.onProgress(100);
                                callback.onSuccess(apkFile);
                            }
                            cursor.close();
                            break;
                        } else if (status == DownloadManager.STATUS_FAILED) {
                            int reason = cursor.getInt(cursor.getColumnIndexOrThrow(DownloadManager.COLUMN_REASON));
                            if (callback != null) {
                                callback.onFailure("下载失败，错误码: " + reason);
                            }
                            cursor.close();
                            break;
                        } else if (status == DownloadManager.STATUS_RUNNING || status == DownloadManager.STATUS_PAUSED) {
                            int bytesDownloaded = cursor.getInt(cursor.getColumnIndexOrThrow(DownloadManager.COLUMN_BYTES_DOWNLOADED_SO_FAR));
                            int bytesTotal = cursor.getInt(cursor.getColumnIndexOrThrow(DownloadManager.COLUMN_TOTAL_SIZE_BYTES));
                            if (bytesTotal > 0 && callback != null) {
                                int progress = (int) ((bytesDownloaded * 100L) / bytesTotal);
                                callback.onProgress(progress);
                            }
                            Thread.sleep(500);
                        } else {
                            Thread.sleep(500);
                        }
                    } else {
                        if (cursor != null) cursor.close();
                        break;
                    }
                    
                    if (cursor != null) cursor.close();
                }
            } catch (Exception e) {
                Log.e(TAG, "Progress check error: " + e.getMessage());
                if (callback != null) {
                    callback.onFailure("检查进度失败: " + e.getMessage());
                }
            }
        }).start();
    }

    public void cancelDownload() {
        if (downloadId != -1) {
            downloadManager.remove(downloadId);
            downloadId = -1;
        }
    }

    public static void installApk(Context context, File apkFile) {
        try {
            Uri apkUri;
            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                apkUri = FileProvider.getUriForFile(context,
                    context.getPackageName() + ".fileprovider", apkFile);
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
        try {
            return context.getPackageManager()
                .getPackageInfo(context.getPackageName(), 0).versionName;
        } catch (Exception e) {
            return "1.0";
        }
    }

    public static int getVersionCode(Context context) {
        try {
            return context.getPackageManager()
                .getPackageInfo(context.getPackageName(), 0).versionCode;
        } catch (Exception e) {
            return 1;
        }
    }
}
