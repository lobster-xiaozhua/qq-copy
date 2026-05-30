-keep class com.example.qqchat.model.** { *; }
-keep class com.example.qqchat.adapter.** { *; }
-keepclassmembers class * {
    @com.google.gson.annotations.SerializedName <fields>;
}
-dontwarn okhttp3.**
-dontwarn okio.**
