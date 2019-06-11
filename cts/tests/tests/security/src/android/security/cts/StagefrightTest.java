/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 * This code was provided to AOSP by Zimperium Inc and was
 * written by:
 *
 * Simone "evilsocket" Margaritelli
 * Joshua "jduck" Drake
 */
package android.security.cts;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.MediaPlayer;
import android.os.Looper;
import android.test.InstrumentationTestCase;
import android.util.Log;

import java.io.IOException;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

import com.android.cts.security.R;


/**
 * Verify that the device is not vulnerable to any known Stagefright
 * vulnerabilities.
 */
public class StagefrightTest extends InstrumentationTestCase {
    static final String TAG = "StagefrightTest";

    public StagefrightTest() {
    }

    public void testStagefright_cve_2015_1538_1() throws Exception {
        doStagefrightTest(R.raw.cve_2015_1538_1);
    }

    public void testStagefright_cve_2015_1538_2() throws Exception {
        doStagefrightTest(R.raw.cve_2015_1538_2);
    }

    public void testStagefright_cve_2015_1538_3() throws Exception {
        doStagefrightTest(R.raw.cve_2015_1538_3);
    }

    public void testStagefright_cve_2015_1538_4() throws Exception {
        doStagefrightTest(R.raw.cve_2015_1538_4);
    }

    public void testStagefright_cve_2015_1539() throws Exception {
        doStagefrightTest(R.raw.cve_2015_1539);
    }

    public void testStagefright_cve_2015_3824() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3824);
    }

    public void testStagefright_cve_2015_3826() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3826);
    }

    public void testStagefright_cve_2015_3827() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3827);
    }

    public void testStagefright_cve_2015_3828() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3828);
    }

    public void testStagefright_cve_2015_3829() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3829);
    }

    public void testStagefright_cve_2015_3864() throws Exception {
        doStagefrightTest(R.raw.cve_2015_3864);
    }

    public void testStagefright_cve_2015_6598() throws Exception {
        doStagefrightTest(R.raw.cve_2015_6598);
    }

    private void doStagefrightTest(final int rid) throws Exception {
        class MediaPlayerCrashListener
                implements MediaPlayer.OnErrorListener,
                    MediaPlayer.OnPreparedListener,
                    MediaPlayer.OnCompletionListener {
            @Override
            public boolean onError(MediaPlayer mp, int newWhat, int extra) {
                what = newWhat;
                lock.lock();
                condition.signal();
                lock.unlock();

                return true; // don't call oncompletion
            }

            @Override
            public void onPrepared(MediaPlayer mp) {
                mp.start();
            }

            @Override
            public void onCompletion(MediaPlayer mp) {
                what = 0;
                lock.lock();
                condition.signal();
                lock.unlock();
            }

            public int waitForError() throws InterruptedException {
                lock.lock();
                condition.await();
                lock.unlock();
                return what;
            }

            ReentrantLock lock = new ReentrantLock();
            Condition condition = lock.newCondition();
            int what;
        }

        final MediaPlayerCrashListener mpcl = new MediaPlayerCrashListener();

        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                Looper.prepare();

                MediaPlayer mp = new MediaPlayer();
                mp.setOnErrorListener(mpcl);
                mp.setOnPreparedListener(mpcl);
                mp.setOnCompletionListener(mpcl);
                try {
                    AssetFileDescriptor fd = getInstrumentation().getContext().getResources()
                        .openRawResourceFd(rid);

                    mp.setDataSource(fd.getFileDescriptor(),
                                     fd.getStartOffset(),
                                     fd.getLength());

                    mp.prepareAsync();
                } catch (Exception e) {
                }

                Looper.loop();
                mp.release();
            }
        });

        t.start();
        String name = getInstrumentation().getContext().getResources().getResourceEntryName(rid);
        String cve = name.replace("_", "-").toUpperCase();
        assertFalse("Device *IS* vulnerable to " + cve,
                    mpcl.waitForError() == MediaPlayer.MEDIA_ERROR_SERVER_DIED);
        t.interrupt();
    }
}
