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
import android.widget.ListView;
import android.widget.PopupWindow;
import android.widget.TextView;
import android.widget.ImageView;
import android.os.Bundle;

import android.app.Notification;
import android.app.PendingIntent;
import android.content.Intent;

import java.util.List;

import android.service.notification.StatusBarNotification;

import android.graphics.drawable.Icon;

import java.sql.Date;
import java.text.SimpleDateFormat;

import android.widget.AdapterView;

import com.android.internal.statusbar.IStatusBarService;
import android.os.RemoteException;
import android.os.ServiceManager;

import android.os.Handler;
import android.os.Message;

public class NotificationPopupView extends PopupWindow {

    private View mPopupView;
    private ListView noList;
    private TextView tv_notice;

    protected IStatusBarService mBarService;

    private final MyNotificationListener mNotificationListener = MyNotificationListener.getInstance();

    private MyAdapter mAdapter;

    private MyStateListener mMyStateListener;

    public NotificationPopupView(final Activity context, final List<StatusBarNotification> data) {
        super(context);
        LayoutInflater inflater = (LayoutInflater) context
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mPopupView = inflater.inflate(R.layout.popup_view, null);

        noList = (ListView) mPopupView.findViewById(R.id.lv_pop);

        tv_notice = (TextView) mPopupView.findViewById(R.id.tv_notice);

        tv_notice.setText("No Notifications");

        if(data == null || data.size() == 0){
            tv_notice.setVisibility(View.VISIBLE);
        } else {
            tv_notice.setVisibility(View.GONE);
        }

        mMyStateListener = new MyStateListener();
        mNotificationListener.setListener(mMyStateListener);

        mAdapter = new MyAdapter(context,data);
        noList.setAdapter(mAdapter);

        this.setContentView(mPopupView);
        this.setWidth(ViewGroup.LayoutParams.MATCH_PARENT);
        this.setHeight(ViewGroup.LayoutParams.MATCH_PARENT);
        this.setFocusable(true);
        this.setAnimationStyle(R.style.AnimBottom);
        ColorDrawable dw = new ColorDrawable(0xb0000000);
        this.setBackgroundDrawable(dw);
        mPopupView.setOnTouchListener(new View.OnTouchListener() {

            public boolean onTouch(View v, MotionEvent event) {

                int height = mPopupView.findViewById(R.id.pop_layout).getTop();
                int y=(int) event.getY();
                if(event.getAction()==MotionEvent.ACTION_UP){
                    if(y<height){
                        dismiss();
                    }
                }

                return true;
            }
        });

        noList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {

                PendingIntent pendingIntent = data.get(position).getNotification().contentIntent;
                Intent intent = pendingIntent.getIntent();
                if(pendingIntent.isActivity()){
                    context.startActivity(intent);
                } else {
                    context.sendBroadcast(intent);
                }

                mNotificationListener.cancelNotification(data.get(position).getKey());
                dismiss();
            }
        });

    }

    private class MyStateListener implements MyNotificationListener.StateListener {
        @Override
        public void onStateChanged() {
            mHandler.sendEmptyMessage(0);
        }
    };

    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            mAdapter.notifyDataSetChanged();
        }
    };

    private class MyAdapter extends BaseAdapter{

        private Context mContext;
        private List<StatusBarNotification> mData;

        public MyAdapter(Context context, List<StatusBarNotification> data) {
            this.mContext = context;
            this.mData = data;
        }

        @Override
        public int getCount() {
            return mData == null ? 0 : mData.size();
        }

        @Override
        public Object getItem(int position) {
            return mData.get(position);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(final int position, View convertView, ViewGroup parent) {
            ViewHolder holder;
            if (convertView == null) {
                holder = new ViewHolder();
                convertView = LayoutInflater.from(mContext).inflate(R.layout.notification_item, null);
                holder.tv_name = (TextView) convertView.findViewById(R.id.tv_name);
                holder.tv_time = (TextView) convertView.findViewById(R.id.tv_time);
                holder.tv_message = (TextView) convertView.findViewById(R.id.tv_message);
                holder.iv_icon = (ImageView) convertView.findViewById(R.id.iv_icon);
                convertView.setTag(holder);
            } else {
                holder = (ViewHolder) convertView.getTag();
            }

            Date date = new Date(mData.get(position).getPostTime());
            SimpleDateFormat sd = new SimpleDateFormat("HH:mm");
            sd.format(date);

            Bundle extras = mData.get(position).getNotification().extras;

            holder.tv_name.setText(extras.getString(Notification.EXTRA_TITLE));
            holder.tv_time.setText(sd.format(date)+"");
            holder.tv_message.setText(extras.getCharSequence(Notification.EXTRA_TEXT));
            Icon notificationIcon = Icon.createWithResource(mData.get(position).getPackageName(), mData.get(position).getNotification().getSmallIcon().getResId());
            holder.iv_icon.setImageIcon(notificationIcon);

            return convertView;
        }

        class ViewHolder {
            TextView tv_name;
            TextView tv_time;
            TextView tv_message;
            ImageView iv_icon;
        }
    }

}

