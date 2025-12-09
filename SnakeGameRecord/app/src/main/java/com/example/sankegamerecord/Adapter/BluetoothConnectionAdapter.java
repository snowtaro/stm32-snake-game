package com.example.sankegamerecord.Adapter;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import androidx.annotation.RequiresPermission;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Set;
import java.util.UUID;

public class BluetoothConnectionAdapter {

    public static final String TAG = "BTConnectionAdapter";

    // Broadcast Action / Extras
    public static final String ACTION_BT_EVENT = "com.example.sankegamerecord.BT_EVENT";
    public static final String EXTRA_STATE = "BT_STATE";
    public static final String EXTRA_MESSAGE = "BT_MESSAGE";
    public static final String EXTRA_DATA = "BT_DATA";

    public static final int STATE_NONE = 0;
    public static final int STATE_CONNECTED = 1;
    public static final int STATE_DISCONNECTED = 2;
    public static final int STATE_FINISHED = 3;
    public static int CurrentState=STATE_NONE;

    private Context context;
    private BluetoothAdapter adapter;
    private BluetoothSocket socket;
    private BluetoothDevice connectedDevice;

    private InputStream inStream;
    private OutputStream outStream;
    private boolean running = false;
    private Thread readThread;
    private final StringBuilder buffer = new StringBuilder();

    public BluetoothConnectionAdapter(Context ctx) {
        context = ctx;
        adapter = BluetoothAdapter.getDefaultAdapter();
    }

    /** 상태 브로드캐스트 */
    private void broadcastState(int state, String msg) {
        Intent intent = new Intent(ACTION_BT_EVENT);
        intent.putExtra(EXTRA_STATE, state);
        intent.putExtra(EXTRA_MESSAGE, msg);
        LocalBroadcastManager.getInstance(context).sendBroadcast(intent);
    }

    /** 데이터 브로드캐스트 */
    private void broadcastData(byte[] data, int length) {
        Intent intent = new Intent(ACTION_BT_EVENT);
        intent.putExtra(EXTRA_STATE, STATE_CONNECTED);
        intent.putExtra(EXTRA_DATA, Arrays.copyOf(data, length));
        LocalBroadcastManager.getInstance(context).sendBroadcast(intent);
    }

    /** 장치 선택 다이얼로그용: 이름/주소 배열 반환 */
    @RequiresPermission(Manifest.permission.BLUETOOTH_CONNECT)
    public String[] getPairedDeviceNames() {
        Set<BluetoothDevice> paired = adapter.getBondedDevices();
        String[] names = new String[paired.size()];
        int i = 0;
        for (BluetoothDevice d : paired) {
            names[i++] = d.getName() + "\n" + d.getAddress();
        }
        return names;
    }

    @RequiresPermission(Manifest.permission.BLUETOOTH_CONNECT)
    public String[] getPairedDeviceMACs() {
        Set<BluetoothDevice> paired = adapter.getBondedDevices();
        String[] macs = new String[paired.size()];
        int i = 0;
        for (BluetoothDevice d : paired) {
            macs[i++] = d.getAddress();
        }
        return macs;
    }

    public int getCurrentState(){
        return CurrentState;
    }

    /** 실제 연결 */
    @RequiresPermission(Manifest.permission.BLUETOOTH_CONNECT)
    public void connect(String mac) {
        new Thread(() -> {
            try {
                BluetoothDevice device = adapter.getRemoteDevice(mac);
                socket = device.createRfcommSocketToServiceRecord(
                        UUID.fromString("00001101-0000-1000-8000-00805F9B34FB")
                );
                socket.connect();

                inStream = socket.getInputStream();
                outStream = socket.getOutputStream();
                connectedDevice = device;

                broadcastState(STATE_CONNECTED, device.getName());
                CurrentState=STATE_CONNECTED;
                startReceive();

            } catch (Exception e) {
                Log.e(TAG, "Connect failed", e);
                broadcastState(STATE_DISCONNECTED, e.getMessage());
                CurrentState=STATE_DISCONNECTED;
            }
        }).start();
    }

    /** 데이터 수신 스레드 */
    public void startReceive() {
        new Thread(() -> {
            byte[] readBuffer = new byte[1024];
            int read;

            try {
                while (!Thread.currentThread().isInterrupted()) {
                    read = inStream.read(readBuffer);
                    if (read > 0) {
                        String chunk = new String(readBuffer, 0, read, StandardCharsets.UTF_8);
                        buffer.append(chunk);

                        int newlineIndex;
                        while ((newlineIndex = buffer.indexOf("\n")) != -1) {
                            String completeMessage = buffer.substring(0, newlineIndex).trim();
                            buffer.delete(0, newlineIndex + 1);

                            // 완전 메시지만 브로드캐스트
                            Intent intent = new Intent(ACTION_BT_EVENT);
                            intent.putExtra(EXTRA_DATA, completeMessage.getBytes(StandardCharsets.UTF_8));
                            LocalBroadcastManager.getInstance(context).sendBroadcast(intent);
                        }
                    }
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }).start();
    }

    /** 연결 종료 */
    public void stop() {
        running = false;
        try {
            if (inStream != null) inStream.close();
            if (outStream != null) outStream.close();
            if (socket != null) socket.close();
            broadcastState(STATE_FINISHED, "disconnect");
            CurrentState=STATE_FINISHED;
        } catch (Exception e) {
            Log.e(TAG, "Close error", e);
        }
    }

    public void ackFinished(){
        CurrentState=STATE_NONE;
    }

    /** 연결된 장치 MAC 반환 */
    public String getConnectedDeviceAddress() {
        return connectedDevice != null ? connectedDevice.getAddress() : null;
    }
}
