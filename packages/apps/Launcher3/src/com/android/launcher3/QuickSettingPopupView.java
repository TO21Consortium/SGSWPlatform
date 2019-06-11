package com.android.launcher3;


import android.app.Activity;
import android.content.Context;
import android.graphics.drawable.ColorDrawable;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.PopupWindow;
import android.widget.TextView;
import android.widget.ImageView;
import android.widget.CompoundButton;
import android.os.Bundle;
import android.os.Handler;

import android.app.Notification;
import android.app.PendingIntent;
import android.content.Intent;

import android.provider.Settings;

import android.net.wifi.WifiManager;

import android.bluetooth.BluetoothAdapter;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;

public class QuickSettingPopupView extends PopupWindow {

    private View mPopupView;
    private SeekBar mSeekBar;
    private Context mContext;
    private boolean mode;

    private Switch sw_wifi;
    private Switch sw_bluetooth;

    private float mXDown;
    private float mYDown;

    private WifiManager mWifiManager;
    private BluetoothAdapter mBluetoothAdapter;

    public QuickSettingPopupView(final Activity context) {
        super(context);
        this.mContext = context;
        LayoutInflater inflater = (LayoutInflater) context
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mPopupView = inflater.inflate(R.layout.quick_setting, null);

        mSeekBar = (SeekBar) mPopupView.findViewById(R.id.bright_seekbar);

        sw_wifi = (Switch) mPopupView.findViewById(R.id.sw_wifi);

        sw_bluetooth = (Switch) mPopupView.findViewById(R.id.sw_bluetooth);

        mWifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);

        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

        if(Launcher.wifiFlag.equals("true")){
            sw_wifi.setChecked(true);
        } else {
            sw_wifi.setEnabled(true);
            if(mWifiManager.isWifiEnabled()){
                sw_wifi.setChecked(true);
                Launcher.wifiFlag = "false";
            } else {
                sw_wifi.setChecked(false);
            }
        }

        if(Launcher.blueToothFlag.equals("true")){
            sw_bluetooth.setChecked(true);
        } else {
            sw_bluetooth.setEnabled(true);
            switch(mBluetoothAdapter.getState()){
            case BluetoothAdapter.STATE_ON:
                sw_bluetooth.setChecked(true);
                Launcher.blueToothFlag = "false";
                sw_bluetooth.setEnabled(true);
                break;
            case BluetoothAdapter.STATE_TURNING_ON:
                sw_bluetooth.setChecked(true);
                break;
            case BluetoothAdapter.STATE_OFF:
                sw_bluetooth.setChecked(false);
                break;
            case BluetoothAdapter.STATE_TURNING_OFF:
                sw_bluetooth.setChecked(false);
                break;
            }
        }

        IntentFilter bluetoothFilter = new IntentFilter();
        bluetoothFilter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        context.registerReceiver(BlueToothReceiver, bluetoothFilter);

        IntentFilter wifiFilter = new IntentFilter("android.net.wifi.WIFI_STATE_CHANGED");
        context.registerReceiver(WifiReceiver, wifiFilter);

        try {
            mSeekBar.setProgress(Settings.System.getInt(context.getContentResolver(), Settings.System.SCREEN_BRIGHTNESS));
        } catch (Settings.SettingNotFoundException e) {
            e.printStackTrace();
        }

        this.setContentView(mPopupView);
        this.setWidth(ViewGroup.LayoutParams.MATCH_PARENT);
        this.setHeight(ViewGroup.LayoutParams.MATCH_PARENT);
        this.setFocusable(true);
        // this.setAnimationStyle(R.style.AnimTop);
        ColorDrawable dw = new ColorDrawable(0xb0000000);
        this.setBackgroundDrawable(dw);

        
        mPopupView.setOnTouchListener(new View.OnTouchListener() {

            public boolean onTouch(View v, MotionEvent event) {

                switch (event.getAction() & MotionEvent.ACTION_MASK) {
                case MotionEvent.ACTION_DOWN:
                    mXDown = event.getX();
                    mYDown = event.getY();
                    break;
                case MotionEvent.ACTION_UP:
                    float mLastY = event.getY();
                    float yDiff = (float)(mLastY - mYDown);
                    if(yDiff < - 50){
                        dismiss();
                    }
                    break;
                }

                return true;
            }
        });

        sw_wifi.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked){
                    mWifiManager.setWifiEnabled(true);
                    Launcher.wifiFlag = "true";
                    sw_wifi.setEnabled(false);
                } else {
                    mWifiManager.setWifiEnabled(false);
                    Launcher.wifiFlag = "false";
                }
            }
        });


        sw_bluetooth.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked){
                    mBluetoothAdapter.enable();
                    Launcher.blueToothFlag = "true";
                    sw_bluetooth.setEnabled(false);
                } else {
                    mBluetoothAdapter.disable();
                    Launcher.blueToothFlag = "false";
                }
            }
        });


        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                if(!mode){
                    try {
                        Settings.System.putInt(context.getContentResolver(), Settings.System.SCREEN_BRIGHTNESS_MODE, 0);
                    } catch (Exception localException) {
                        localException.printStackTrace();
                    }

                }

                Settings.System.putInt(context.getContentResolver(), Settings.System.SCREEN_BRIGHTNESS, i + 1);

            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });

    }

    private void getScreenMode(){
        int modeInt = Settings.System.getInt(((Launcher) mContext).getContentResolver(), Settings.System.SCREEN_BRIGHTNESS_MODE,-1);
        if(modeInt==0){
            mode =true;
        }else{
            mode = false;
        }
    }

    private BroadcastReceiver BlueToothReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            BluetoothAdapter mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
            switch(mBluetoothAdapter.getState()){
            case BluetoothAdapter.STATE_ON:
                sw_bluetooth.setChecked(true);
                Launcher.blueToothFlag = "false";
                sw_bluetooth.setEnabled(true);
                break;
            case BluetoothAdapter.STATE_TURNING_ON:
                sw_bluetooth.setChecked(true);
                Launcher.blueToothFlag = "false";
                sw_bluetooth.setEnabled(true);
                break;
            }
        }
    };

    private BroadcastReceiver WifiReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            WifiManager mWifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
            if(mWifiManager.isWifiEnabled()){
                sw_wifi.setChecked(true);
                Launcher.wifiFlag = "false";
                sw_wifi.setEnabled(true);
            }
        }
    };

}


