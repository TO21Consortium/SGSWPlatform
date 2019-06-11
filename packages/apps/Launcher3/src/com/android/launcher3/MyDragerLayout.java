package com.android.launcher3;

import android.content.Context;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import android.widget.FrameLayout;
import android.widget.ListView;

import java.util.List;
import java.util.ArrayList;

import android.service.notification.StatusBarNotification;

import android.widget.BaseAdapter;
import android.widget.TextView;
import android.view.LayoutInflater;
import android.view.ViewGroup;

import android.util.Log;


public class MyDragerLayout extends FrameLayout {

    private final String TAG = "MyDragerLayout";

    private final static int SHOW_NONE_MODE = 0xffffff;

    private final static int SHOW_TOP_MODE = 0xffffff + 1;

    private final static int SHOW_BOTTOM_MODE = 0xffffff - 1;

    private final static int CANCEL_TOP_MODE = 0xffffff + 2;

    private final static int CANCEL_BOTTOM_MODE = 0xffffff - 2;

    private final static long ANIMATION_DURATION = 200;

    private int windowHeight;

    private int windowWidth;

    private View topView;

    private View bottomView;

    private View mainView;

    private int currentMode;


    public MyDragerLayout(Context context) {
        super(context);
    }

    public MyDragerLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        WindowManager wm = (WindowManager) getContext()
                .getSystemService(Context.WINDOW_SERVICE);

        DisplayMetrics dm = new DisplayMetrics();

        wm.getDefaultDisplay().getMetrics(dm);

        windowHeight = dm.heightPixels;
        windowWidth = dm.widthPixels;

        currentMode = SHOW_NONE_MODE;

    }


    @Override
    protected void onLayout(boolean b, int i, int i1, int i2, int i3) {
        initLayout();
    }

    private ListView noList;
    private List<StatusBarNotification> data;
    // private MyAdapter mAdapter;

    private TextView tv_notices;

    private final MyNotificationListener mNotificationListener = MyNotificationListener.getInstance();

    private void initLayout() {
        topView.layout(0, -windowHeight, windowWidth, 0);
        mainView.layout(0, 0, windowWidth, windowHeight);
        bottomView.layout(0, windowHeight, windowWidth, windowHeight * 2);

        // noList = (ListView) bottomView.findViewById(R.id.lv_notices);

        // tv_notices = (TextView) bottomView.findViewById(R.id.tv_notices);

        // tv_notices.setText("No Notification");

        // data = new ArrayList<StatusBarNotification>();
        // mAdapter = new MyAdapter(mContext,data);

        // noList.setAdapter(mAdapter);
    }

    // private class MyAdapter extends BaseAdapter{

    //     private Context mContext;
    //     private List<StatusBarNotification> mData;

    //     public MyAdapter(Context context, List<StatusBarNotification> data) {
    //         this.mContext = context;
    //         this.mData = data;
    //     }

    //     @Override
    //     public int getCount() {
    //         return mData == null ? 0 : mData.size();
    //     }

    //     @Override
    //     public Object getItem(int position) {
    //         return mData.get(position);
    //     }

    //     @Override
    //     public long getItemId(int position) {
    //         return position;
    //     }

    //     @Override
    //     public View getView(int position, View convertView, ViewGroup parent) {
    //         ViewHolder holder;
    //         if (convertView == null) {
    //             holder = new ViewHolder();
    //             convertView = LayoutInflater.from(mContext).inflate(R.layout.notification_item, null);
    //             holder.tv_text = (TextView) convertView.findViewById(R.id.tv_text);
    //             convertView.setTag(holder);
    //         } else {
    //             holder = (ViewHolder) convertView.getTag();
    //         }

    //         holder.tv_text.setText(mData.get(position).getPackageName());
    //         return convertView;
    //     }

    //     class ViewHolder {
    //         TextView tv_text;
    //     }
    // }



    private float downX;

    private float downY;

    private boolean bottomMove = false;

    private boolean topMove = false;


    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        return true;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                downX = event.getX();
                downY = event.getY();
                if (downY < 200) {
                    topMove = true;
                    bottomMove = false;
                } else if (downY > windowHeight - 200) {
                    topMove = false;
                    bottomMove = true;
                }
                break;
            case MotionEvent.ACTION_UP:
                if (currentMode == SHOW_NONE_MODE && bottomMove) {
                    if (bottomView.getTop() <= windowHeight * 2 / 3) {
                        bottomView.startAnimation(getTransAnimation(SHOW_BOTTOM_MODE, new AnimationEndListener() {
                            @Override
                            public void onAnimationEnd() {
                                bottomView.layout(0, 0, windowWidth, windowHeight);
                                currentMode = SHOW_BOTTOM_MODE;
                            }
                        }));

                    } else {
                        bottomView.startAnimation(getTransAnimation(CANCEL_BOTTOM_MODE, new AnimationEndListener() {
                            @Override
                            public void onAnimationEnd() {
                                bottomView.layout(0, windowHeight, windowWidth, 2 * windowHeight);
                            }
                        }));
                    }

                } else if (currentMode == SHOW_NONE_MODE && topMove) {

                    if (topView.getBottom() <= windowHeight / 3) {
                        topView.startAnimation(getTransAnimation(CANCEL_TOP_MODE, new AnimationEndListener() {
                            @Override
                            public void onAnimationEnd() {
                                topView.layout(0, -windowHeight, windowWidth, 0);
                            }
                        }));

                    } else {
                        topView.startAnimation(getTransAnimation(SHOW_TOP_MODE, new AnimationEndListener() {
                            @Override
                            public void onAnimationEnd() {
                                topView.layout(0, 0, windowWidth, windowHeight);
                                currentMode = SHOW_TOP_MODE;
                            }
                        }));
                    }


                } else if (currentMode == SHOW_TOP_MODE && bottomMove) {

                    topView.startAnimation(getTransAnimation(CANCEL_TOP_MODE, new AnimationEndListener() {
                        @Override
                        public void onAnimationEnd() {
                            topView.layout(0, -windowHeight, windowWidth, 0);
                            currentMode = SHOW_NONE_MODE;
                        }
                    }));


                } else if (currentMode == SHOW_BOTTOM_MODE && topMove) {
                    bottomView.startAnimation(getTransAnimation(CANCEL_BOTTOM_MODE, new AnimationEndListener() {
                        @Override
                        public void onAnimationEnd() {
                            bottomView.layout(0, windowHeight, windowWidth, windowHeight * 2);
                            currentMode = SHOW_NONE_MODE;
                        }
                    }));

                }


                topMove = false;
                bottomMove = false;
//                initLayout();

                break;
            case MotionEvent.ACTION_MOVE:
                float moveY = event.getY() - downY;
                if (currentMode == SHOW_NONE_MODE && bottomMove) {
                    bottomView.layout(0, (int) (windowHeight + moveY) <= 0 ? 0 : (int) (windowHeight + moveY), windowWidth, (int) (windowHeight * 2 + moveY) <= windowHeight ? windowHeight : (int) (windowHeight * 2 + moveY));
                } else if (currentMode == SHOW_NONE_MODE && topMove) {
                    topView.layout(0, (int) (-windowHeight + moveY) >= 0 ? 0 : (int) (-windowHeight + moveY), windowWidth, (int) (moveY) >= windowHeight ? windowHeight : (int) (moveY));
                } else if (currentMode == SHOW_TOP_MODE && bottomMove) {
                    topView.layout(0, (int) (moveY <= -windowHeight ? -windowHeight : moveY), windowWidth, (int) ((windowHeight + moveY) <= 0 ? 0 : (windowHeight + moveY)));
                } else if (currentMode == SHOW_BOTTOM_MODE && topMove) {
                    bottomView.layout(0, (int) (moveY >= windowHeight ? windowHeight : moveY), windowWidth, (int) ((windowHeight + moveY) >= 2 * windowHeight ? 2 * windowHeight : windowHeight + moveY));
                }
                break;


        }


        return true;
    }

    private interface AnimationEndListener {
        void onAnimationEnd();
    }


    private Animation getTransAnimation(int typeMode, final AnimationEndListener listener) {

        TranslateAnimation animation;

        switch (typeMode) {
            case SHOW_BOTTOM_MODE:
                animation = new TranslateAnimation(0, 0, 0, mainView.getTop() - bottomView.getTop());
                break;
            case SHOW_TOP_MODE:
                animation = new TranslateAnimation(0, 0, 0, mainView.getBottom() - topView.getBottom());
                break;
            case CANCEL_BOTTOM_MODE:
                animation = new TranslateAnimation(0, 0, 0, mainView.getBottom() - bottomView.getTop());
                break;
            case CANCEL_TOP_MODE:
                animation = new TranslateAnimation(0, 0, 0, mainView.getTop() - topView.getBottom());
                break;
            default:
                animation = null;
        }

        if (null != animation) {
            animation.setDuration(ANIMATION_DURATION);
            animation.setFillAfter(true);
            animation.setAnimationListener(new android.view.animation.Animation.AnimationListener() {
                @Override
                public void onAnimationStart(Animation animation) {

                }

                @Override
                public void onAnimationEnd(Animation animation) {
                    listener.onAnimationEnd();
                    bottomView.clearAnimation();
                    topView.clearAnimation();
                }

                @Override
                public void onAnimationRepeat(Animation animation) {

                }
            });
        }
        return animation;
    }


    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        topView = getChildAt(1);
        mainView = getChildAt(0);
        bottomView = getChildAt(2);
        View views[] = new View[]{topView, mainView, bottomView};

        for (int i = 0; i < 3; i++) {
            FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) views[i].getLayoutParams();
            params.width = windowWidth;
            params.height = windowHeight;
            views[i].setLayoutParams(params);
        }


    }
/**
 * 提供接口
 */
//    public interface show


}
