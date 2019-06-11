package com.android.launcher3;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;


public class AppReceiver extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {
    		Intent myIntent = new Intent();
         myIntent.setAction("app_changed");
         context.sendBroadcast(myIntent);

    }
};
