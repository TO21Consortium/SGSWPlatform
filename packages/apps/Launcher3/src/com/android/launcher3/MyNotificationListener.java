package com.android.launcher3;


import android.app.Notification;
import android.content.Context;
import android.os.Message;
import android.service.notification.NotificationListenerService;
import android.service.notification.NotificationListenerService.Ranking;
import android.service.notification.NotificationListenerService.RankingMap;
import android.service.notification.StatusBarNotification;
import android.util.Log;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import android.content.Intent;

public class MyNotificationListener extends NotificationListenerService {
    private static final String TAG = "MyNotificationListener";

    private static MyNotificationListener instance;
    private MyNotificationListener() {
        //Do Nothing
    }

    public static MyNotificationListener getInstance() {
        if (instance == null) {
            instance = new MyNotificationListener();
        }
        return instance;
    }

    private List<StatusBarNotification> sNotifications;

    public List<StatusBarNotification> getNotifications() {
        return sNotifications;
    }

    private final Ranking mTmpRanking = new Ranking();

    private final Comparator<StatusBarNotification> mRankingComparator =
            new Comparator<StatusBarNotification>() {

                private final Ranking mLhsRanking = new Ranking();
                private final Ranking mRhsRanking = new Ranking();

                @Override
                public int compare(StatusBarNotification lhs, StatusBarNotification rhs) {
                    mRankingMap.getRanking(lhs.getKey(), mLhsRanking);
                    mRankingMap.getRanking(rhs.getKey(), mRhsRanking);
                    return Integer.compare(mLhsRanking.getRank(), mRhsRanking.getRank());
                }
            };


    public interface StateListener {
        public void onStateChanged();
    }

    private StateListener mStateListener;

    public void setListener(StateListener stateListener) {
        mStateListener = stateListener;
    }

    private RankingMap mRankingMap;

    @Override
    public void onListenerConnected() {
    	Log.i(TAG,"================");
        mRankingMap = getCurrentRanking();
        sNotifications = new ArrayList<StatusBarNotification>();
        for (StatusBarNotification sbn : getActiveNotifications()) {
            sNotifications.add(sbn);
        }
        Collections.sort(sNotifications, mRankingComparator);
    }

    @Override
    public void onNotificationRankingUpdate(RankingMap rankingMap) {
        synchronized (sNotifications) {
            mRankingMap = rankingMap;
            Collections.sort(sNotifications, mRankingComparator);
        }
        if (mStateListener != null) mStateListener.onStateChanged();
    }

    @Override
    public void onNotificationPosted(StatusBarNotification sbn, RankingMap rankingMap) {
        synchronized (sNotifications) {
            boolean exists = mRankingMap.getRanking(sbn.getKey(), mTmpRanking);
            if (!exists) {
                sNotifications.add(sbn);
            } else {
                int position = mTmpRanking.getRank();
                sNotifications.set(position, sbn);
            }
            mRankingMap = rankingMap;
            Collections.sort(sNotifications, mRankingComparator);
            Log.i(TAG, "finish with: " + sNotifications.size());
        }
        if (mStateListener != null) mStateListener.onStateChanged();
        // if ("com.google.android.calendar".equals(sbn.getPackageName())) {
        //     ViewHelper mHelper = ViewHelper.getInstance();
        //     Message tmpMsg = new Message();
        //     tmpMsg.what = ViewHelper.MSG_CALENDAR;
        //     tmpMsg.obj = sbn.getNotification().extras;
        //     mHelper.sendMessage(tmpMsg);
        // }
    }

    @Override
    public void onNotificationRemoved(StatusBarNotification sbn, RankingMap rankingMap) {

        synchronized (sNotifications) {
            boolean exists = mRankingMap.getRanking(sbn.getKey(), mTmpRanking);
            if (exists) {
                sNotifications.remove(mTmpRanking.getRank());
            }
            mRankingMap = rankingMap;
            Collections.sort(sNotifications, mRankingComparator);
        }
        if (mStateListener != null) mStateListener.onStateChanged();
    }

}



