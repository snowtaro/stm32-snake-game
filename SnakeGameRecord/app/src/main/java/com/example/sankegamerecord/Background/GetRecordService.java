package com.example.sankegamerecord.Background;

import android.Manifest;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;
import android.util.Log;

import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import com.example.sankegamerecord.Adapter.BluetoothConnectionAdapter;
import com.example.sankegamerecord.Adapter.GameRecord;
import com.example.sankegamerecord.DataBaseAdapter.RankAdapter;
import com.example.sankegamerecord.DataBaseAdapter.RecordAdapter;

import java.nio.charset.StandardCharsets;

public class GetRecordService extends Service {

    private static final String TAG = "YourNameService";

    public static final String ACTION_CONNECT = "ACTION_CONNECT";
    public static final String EXTRA_DEVICE_ADDRESS = "EXTRA_DEVICE_ADDRESS";
    private BluetoothConnectionAdapter btAdapter;
    private ProtocolInterpreter ptInterpreter;
    private RankAdapter RankDB;
    private RecordAdapter RecordDB;
    private BroadcastReceiver btReceiver;

    @Override
    public void onCreate() {
        super.onCreate();
        ptInterpreter = new ProtocolInterpreter();
        RecordDB = new RecordAdapter(this);
        RankDB = new RankAdapter(this);
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
        if (intent != null && ACTION_CONNECT.equals(intent.getAction())) {
            String mac = intent.getStringExtra(EXTRA_DEVICE_ADDRESS);
            if (mac != null) {
                startCommunication(mac);
            } else {
                Log.w(TAG, "No MAC address received");
            }
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
                if (data != null) {
                    String received = new String(data, StandardCharsets.UTF_8);
                    Log.i(TAG, "RECEIVED: " + received);

                    GameRecord record = ptInterpreter.makeRecord(received);
                    if (record != null) {
                        // GameRecord 내부 확인 로그
                        Log.i(TAG, "GameRecord parsed: " +
                                "Success=" + record.Success() +
                                ", Timestamp=" + record.Playdate() +
                                ", AnyOtherField=" + record.Playtime()); // 필요하면 필드 추가

                        RankDB.addScore(record);
                        RecordDB.addRecord(record);
                    } else {
                        Log.e(TAG, "Failed to parse GameRecord from received data: " + received);
                    }
                }
            }
        };

        LocalBroadcastManager.getInstance(this).registerReceiver(
                btReceiver,
                new IntentFilter(BluetoothConnectionAdapter.ACTION_BT_EVENT)
        );

        btAdapter.startReceive(); // 내부에서 패킷 누적 로직 포함
    }


    protected void onStop() {
        if (btReceiver != null) {
            LocalBroadcastManager.getInstance(this).unregisterReceiver(btReceiver);
        }
    }


    @Override
    public void onDestroy() {
        if (btAdapter != null) btAdapter.stop();
        super.onDestroy();
    }
}
