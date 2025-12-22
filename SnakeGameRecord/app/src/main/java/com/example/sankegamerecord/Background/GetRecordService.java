package com.example.sankegamerecord.Background;

import android.Manifest;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.util.Log;

import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.core.app.ActivityCompat;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.core.app.RemoteInput;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import com.example.sankegamerecord.Adapter.BluetoothConnectionAdapter;
import com.example.sankegamerecord.Adapter.GameRecord;
import com.example.sankegamerecord.DataBaseAdapter.RankAdapter;
import com.example.sankegamerecord.DataBaseAdapter.RecordAdapter;

import java.nio.charset.StandardCharsets;
import java.util.LinkedList;
import java.util.Queue;

public class GetRecordService extends Service {

    private static final String TAG = "GetRecordService";

    public static final String ACTION_CONNECT = "ACTION_CONNECT";
    public static final String ACTION_REPLY_RECEIVED = "ACTION_REPLY_RECEIVED";
    public static final String EXTRA_DEVICE_ADDRESS = "EXTRA_DEVICE_ADDRESS";
    private BluetoothConnectionAdapter btAdapter;
    private ProtocolInterpreter ptInterpreter;
    private RankAdapter RankDB;
    private RecordAdapter RecordDB;
    private BroadcastReceiver btReceiver;
    private SharedPreferences sp;


    private static final long REPLY_TIMEOUT_MS = 20_000;
    private static final String PREF_NAME = "user_cache";
    private static final String KEY_USERNAME = "last_username";

    private String currentName = "AAA";
    private boolean waitingReply = false;
    private Handler handler = new Handler(Looper.getMainLooper());
    private Runnable replyTimeoutRunnable;


    private final Queue<String> pendingPackets = new LinkedList<>();

    @Override
    public void onCreate() {
        super.onCreate();

        createNotificationChannel();   // ★ 꼭 필요 ★

        ptInterpreter = new ProtocolInterpreter();
        RankDB = new RankAdapter(this);
        RecordDB = new RecordAdapter(this);

        sp = getSharedPreferences(PREF_NAME, MODE_PRIVATE);
        currentName = sp.getString(KEY_USERNAME, "AAA");

        IntentFilter filter = new IntentFilter("HEADSUP_REPLY");
        registerReceiver(new ReplyInputReceiver(), filter, Context.RECEIVER_NOT_EXPORTED);
    }


    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    // Activity에서 보낸 Intent 처리
    @RequiresPermission(Manifest.permission.BLUETOOTH_CONNECT)
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent != null)
            if (ACTION_CONNECT.equals(intent.getAction())) {
                String mac = intent.getStringExtra(EXTRA_DEVICE_ADDRESS);
                if (mac != null) {
                    startCommunication(mac);
                } else {
                    Log.w(TAG, "No MAC address received");
                }
            } else if (ACTION_REPLY_RECEIVED.equals(intent.getAction())) {

                // 입력이 온 경우에만 이 블록 실행됨
                String replyName = intent.getStringExtra("EXTRA_REPLY_NAME");
                Log.i(TAG, replyName);
                currentName = replyName;
                sp.edit().putString(KEY_USERNAME, currentName).apply();

                waitingReply = false;

                if (replyTimeoutRunnable != null)
                    handler.removeCallbacks(replyTimeoutRunnable);
                Log.i(TAG, "CANCELING NOTIFICATION");
                flushPendingPackets();

                NotificationManagerCompat.from(this).cancel(2001);

            }
        return START_STICKY;
    }

    /**
     * Bluetooth 연결 + 수신 처리
     */
    @RequiresPermission(Manifest.permission.BLUETOOTH_CONNECT)
    private void startCommunication(String mac) {
        btAdapter = new BluetoothConnectionAdapter(this);

        // 연결 시작
        btAdapter.connect(mac);

        // 수신 Receiver 등록
        btReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                byte[] data = intent.getByteArrayExtra(BluetoothConnectionAdapter.EXTRA_DATA);
                if (data == null) return;
                // === HEARTBEAT 필터링 추가 (핵심) ===
                // 1) 길이가 1이고 값이 0x00 → heartbeat → 버림
                String received = new String(data, StandardCharsets.UTF_8);
                if (received.equals("HEARTBEAT") || (!received.startsWith("RPL"))) {
                    Log.i(TAG, "Heartbeat received. Ignored.");

                } else{
                    Log.i(TAG, "RECEIVED raw bytes");
                    if (!pendingPackets.contains(received)) {
                        pendingPackets.add(received);
                    } else {
                        Log.i(TAG, "Duplicate packet skipped: " + received);
                    }

                    Log.i(TAG, received);
                    if (waitingReply) {
                        Log.i(TAG, "Queued. size=" + pendingPackets.size());
                    } else {
                        showHeadsUpReplyNotification();
                    }
                }
            }


        };

        LocalBroadcastManager.getInstance(this).registerReceiver(
                btReceiver,
                new IntentFilter(BluetoothConnectionAdapter.ACTION_BT_EVENT)
        );

        btAdapter.startReceive(); // 내부에서 패킷 누적/수신 로직 포함
    }


    private void flushPendingPackets() {
        while (!pendingPackets.isEmpty()) {
            String packet = pendingPackets.poll();
            Log.i("TAG","flushing "+packet);
            processPacket(packet);
        }
    }

    private void processPacket(String received) {
        GameRecord record = ptInterpreter.makeRecord(received, currentName);
        RankDB.addScore(record);
        RecordDB.addRecord(record);
    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(
                    "reply_channel",
                    "Reply Channel",
                    NotificationManager.IMPORTANCE_HIGH   // ★★ HEADS-UP 핵심 ★★
            );
            channel.setDescription("Heads-up input for player name");

            NotificationManager manager = getSystemService(NotificationManager.class);
            manager.createNotificationChannel(channel);
        }
    }

    private void showHeadsUpReplyNotification() {
        // --- Reply Intent: 명시적 컴포넌트 대신 액션 + 패키지 ---
        Intent replyIntent = new Intent("HEADSUP_REPLY");
        // 명시적으로 패키지를 지정하면 시스템이 같은 앱으로 브로드캐스트를 전달합니다.
        replyIntent.setPackage(getPackageName());

        int flags = PendingIntent.FLAG_UPDATE_CURRENT;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) flags |= PendingIntent.FLAG_MUTABLE;

        PendingIntent replyPendingIntent = PendingIntent.getBroadcast(this, 0, replyIntent, flags);

        RemoteInput remoteInput = new RemoteInput.Builder("key_name")
                .setLabel("이름 입력하기")
                .build();

        NotificationCompat.Action action = new NotificationCompat.Action.Builder(
                android.R.drawable.ic_menu_send,
                "이름 입력",
                replyPendingIntent)
                .addRemoteInput(remoteInput)
                .build();

        Notification notification = new NotificationCompat.Builder(this, "reply_channel")
                .setSmallIcon(android.R.drawable.ic_dialog_info)
                .setContentTitle("플레이어 이름 입력")
                .setContentText("기록할 이름을 입력하세요")
                .setPriority(NotificationCompat.PRIORITY_HIGH)
                .addAction(action)
                .setAutoCancel(true)
                .build();

        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS)
                != PackageManager.PERMISSION_GRANTED) {
            Log.e(TAG, "POST_NOTIFICATIONS 권한 없음. 알림 취소");
            return;
        }

        NotificationManagerCompat.from(this).notify(2001, notification);
        waitingReply = true;

        replyTimeoutRunnable = () -> {
            if (waitingReply) {
                waitingReply = false;
                Log.i(TAG, "HeadsUp reply timed out.");
                NotificationManagerCompat.from(this).cancel(2001);
                flushPendingPackets();
            }
        };
        handler.postDelayed(replyTimeoutRunnable, REPLY_TIMEOUT_MS);
    }


    // ---------------------------------------
    // Static inner Receiver
    // ---------------------------------------
    public static class ReplyInputReceiver extends BroadcastReceiver {


        public ReplyInputReceiver() {
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            if (!"HEADSUP_REPLY".equals(intent.getAction())) return;


            Bundle results = RemoteInput.getResultsFromIntent(intent);
            if (results == null) return;

            CharSequence nameInput = results.getCharSequence("key_name");
            if (nameInput == null) return;

            Intent svc = new Intent(context, GetRecordService.class);
            svc.setAction(GetRecordService.ACTION_REPLY_RECEIVED);
            svc.putExtra("EXTRA_REPLY_NAME", nameInput);
            context.startService(svc);
        }
    }


    @Override
    public void onDestroy() {
        if (btAdapter != null) btAdapter.stop();
        super.onDestroy();
    }
}
