package com.android.launcher3;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.IBinder;
import android.os.PowerManager;
import android.support.annotation.Nullable;
import android.util.Log;

public class ScreenService extends Service {

    private SensorManager sensorManager;
    private SensorEventListener mySensorListener;
    private Sensor accelerSensor;
    private PowerManager.WakeLock mWakeLock;

    private float temp1 = 0;
    private float temp2 = 0;

    private int count = 0;

    private boolean flag = true;


    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();

        sensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
        accelerSensor = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);

        mySensorListener = new SensorEventListener() {
            @Override
            public void onSensorChanged(SensorEvent event) {

                float x = event.values[0];
                float y = event.values[1];
                float z = event.values[2];

                System.out.println(z);

                temp2 = temp1;
                temp1 = z;

                if(Math.abs(temp1 - temp2)<1){
                    count++;
                } else if(Math.abs(temp1) < 3 && Math.abs(temp2) < 3){
                    count++;
                } else {
                    count = 1;
                }

                if(Math.abs(z) < 3 && count > 4){
                    if(flag){
                        Intent intent = new Intent();
                        intent.setAction("G-sensor");
                        ScreenService.this.sendBroadcast(intent);

                        flag = false;
                    }
                } else if(Math.abs(z) > 3){
                    flag = true;
                }

            }

            @Override
            public void onAccuracyChanged(Sensor sensor, int accuracy) {

            }
        };
    }

    @Override
    public void onStart(Intent intent, int startId) {
        super.onStart(intent, startId);
        //acquireWakeLock();
        sensorManager.registerListener(mySensorListener,accelerSensor,SensorManager.SENSOR_DELAY_UI);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        sensorManager.unregisterListener(mySensorListener);
    }

    private void acquireWakeLock()
    {
        if (null == mWakeLock)
        {
            PowerManager pm = (PowerManager)this.getSystemService(Context.POWER_SERVICE);
            mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK|PowerManager.ON_AFTER_RELEASE,"");
            if (null != mWakeLock)
            {
                mWakeLock.acquire();
            }
        }
    }

    private void releaseWakeLock() {
        if (null != mWakeLock) {
            mWakeLock.release();
            mWakeLock = null;
        }
    }
}
