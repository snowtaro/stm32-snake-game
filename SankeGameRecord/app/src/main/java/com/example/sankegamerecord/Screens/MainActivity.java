package com.example.sankegamerecord.Screens;

import android.Manifest;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.widget.Button;
import android.widget.Toast;

import androidx.activity.EdgeToEdge;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresPermission;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import com.example.sankegamerecord.Adapter.BluetoothConnectionAdapter;
import com.example.sankegamerecord.Background.GetRecordService;
import com.example.sankegamerecord.R;

public class MainActivity extends AppCompatActivity {

    private static final int REQ_BT_CONNECT = 1001;

    private Button connectBtn, rankingBtn;
    private ProgressDialog asyncDialog;

    private BluetoothConnectionAdapter btAdapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_main);

        ButtonTouchEffect effect= new ButtonTouchEffect(this);

        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        connectBtn = findViewById(R.id.Connect);
        rankingBtn = findViewById(R.id.Ranking);

        btAdapter = new BluetoothConnectionAdapter(this);
        connectBtn.setOnTouchListener(effect);

        // 🔥 showDeviceList 직접 호출 X → 권한 체크 먼저
        connectBtn.setOnClickListener(v -> {
            if (btAdapter.getCurrentState()==btAdapter.STATE_NONE || btAdapter.getCurrentState()==btAdapter.STATE_DISCONNECTED) {
                // 연결 상태가 아니면 장치 목록 보여주고 연결 시도
                checkBluetoothPermission();
            } else {
                // 이미 연결된 상태 → Disconnect
                btAdapter.stop();
                connectBtn.setText("연결하기");
            }
        });


        rankingBtn.setOnTouchListener(effect);
        rankingBtn.setOnClickListener(v -> {
            startActivity(new Intent(MainActivity.this, RankingActivity.class));
        });

        LocalBroadcastManager.getInstance(this).registerReceiver(
                btEventReceiver,
                new IntentFilter(BluetoothConnectionAdapter.ACTION_BT_EVENT)
        );
    }

    /**
     * 🔥 Android 12+ Bluetooth 권한 체크
     */
    private void checkBluetoothPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT)
                    != PackageManager.PERMISSION_GRANTED) {

                // → 권한 요청 (팝업 뜸)
                requestPermissions(
                        new String[]{ Manifest.permission.BLUETOOTH_CONNECT },
                        REQ_BT_CONNECT
                );
                return;
            }
        }

        // 권한 있음 → 장치 목록 보여줌
        showDeviceList();
    }

    /**
     * 🔥 권한 요청 결과 처리
     */
    @RequiresPermission(Manifest.permission.BLUETOOTH_CONNECT)
    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode == REQ_BT_CONNECT) {
            if (grantResults.length > 0 &&
                    grantResults[0] == PackageManager.PERMISSION_GRANTED) {

                // 권한 허용됨 → 장치 목록 띄우기
                showDeviceList();
            } else {
                Toast.makeText(this, "블루투스 권한이 필요합니다.", Toast.LENGTH_SHORT).show();
            }
        }
    }

    private final BroadcastReceiver btEventReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            int state = intent.getIntExtra(BluetoothConnectionAdapter.EXTRA_STATE,
                    BluetoothConnectionAdapter.STATE_NONE);
            String msg = intent.getStringExtra(BluetoothConnectionAdapter.EXTRA_MESSAGE);

            if (state == BluetoothConnectionAdapter.STATE_CONNECTED) {
                asyncDialog.dismiss();
                Toast.makeText(MainActivity.this, "연결 성공: " + msg, Toast.LENGTH_SHORT).show();

                Intent serviceIntent = new Intent(MainActivity.this, GetRecordService.class);
                serviceIntent.setAction(GetRecordService.ACTION_CONNECT);
                serviceIntent.putExtra(GetRecordService.EXTRA_DEVICE_ADDRESS,
                        btAdapter.getConnectedDeviceAddress());
                startService(serviceIntent);

                connectBtn.setText("연결 해제");

            } else if (state == BluetoothConnectionAdapter.STATE_DISCONNECTED) {
                asyncDialog.dismiss();
                Toast.makeText(MainActivity.this, "연결 실패. 다시 시도해주세요", Toast.LENGTH_SHORT).show();
                connectBtn.setText("연결하기");
            } else if (state == BluetoothConnectionAdapter.STATE_NONE){
                Toast.makeText(MainActivity.this, "연결 해제되었습니다.", Toast.LENGTH_SHORT).show();
                connectBtn.setText("연결하기");
            }
        }
    };

    @RequiresPermission(Manifest.permission.BLUETOOTH_CONNECT)
    private void showDeviceList() {
        String[] names = btAdapter.getPairedDeviceNames();
        String[] macs = btAdapter.getPairedDeviceMACs();

        if (names.length == 0) {
            Toast.makeText(this, "페어링된 장치가 없습니다.", Toast.LENGTH_SHORT).show();
            return;
        }

        new AlertDialog.Builder(this)
                .setTitle("연결할 장치 선택")
                .setItems(names, (dialog, which) -> {
                    String mac = macs[which];
                    connectToService(mac);
                })
                .setCancelable(true)
                .show();
    }

    private void connectToService(String mac) {
        asyncDialog = new ProgressDialog(this);
        asyncDialog.setMessage("블루투스 연결중...");
        asyncDialog.setCancelable(false);
        asyncDialog.show();

        Intent intent = new Intent(this, GetRecordService.class);
        intent.setAction(GetRecordService.ACTION_CONNECT);
        intent.putExtra(GetRecordService.EXTRA_DEVICE_ADDRESS, mac);
        startService(intent);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        LocalBroadcastManager.getInstance(this).unregisterReceiver(btEventReceiver);
        if (btAdapter != null) btAdapter.stop();
    }
}
