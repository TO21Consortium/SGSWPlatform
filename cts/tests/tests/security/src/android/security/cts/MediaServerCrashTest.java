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
 */

package android.security.cts;

import android.content.res.AssetFileDescriptor;
import android.media.MediaPlayer;
import android.os.ConditionVariable;
import android.test.AndroidTestCase;
import android.util.Log;

import com.android.cts.security.R;

public class MediaServerCrashTest extends AndroidTestCase {
    private static final String TAG = "MediaServerCrashTest";

    public void testInvalidMidiNullPointerAccess() throws Exception {
        testIfMediaServerDied(R.raw.midi_crash);
    }

    private void testIfMediaServerDied(int res) throws Exception {
        final MediaPlayer mediaPlayer = new MediaPlayer();
        final ConditionVariable onPrepareCalled = new ConditionVariable();
        final ConditionVariable onCompletionCalled = new ConditionVariable();

        onPrepareCalled.close();
        onCompletionCalled.close();
        mediaPlayer.setOnErrorListener(new MediaPlayer.OnErrorListener() {
            @Override
            public boolean onError(MediaPlayer mp, int what, int extra) {
                assertTrue(mp == mediaPlayer);
                assertTrue("mediaserver process died", what != MediaPlayer.MEDIA_ERROR_SERVER_DIED);
                Log.w(TAG, "onError " + what);
                return false;
            }
        });

        mediaPlayer.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {
            @Override
            public void onPrepared(MediaPlayer mp) {
                assertTrue(mp == mediaPlayer);
                onPrepareCalled.open();
            }
        });

        mediaPlayer.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
            @Override
            public void onCompletion(MediaPlayer mp) {
                assertTrue(mp == mediaPlayer);
                onCompletionCalled.open();
            }
        });

        AssetFileDescriptor afd = getContext().getResources().openRawResourceFd(res);
        mediaPlayer.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
        afd.close();
        try {
            mediaPlayer.prepareAsync();
            if (!onPrepareCalled.block(5000)) {
                Log.w(TAG, "testIfMediaServerDied: Timed out waiting for prepare");
                return;
            }
            mediaPlayer.start();
            if (!onCompletionCalled.block(5000)) {
                Log.w(TAG, "testIfMediaServerDied: Timed out waiting for Error/Completion");
            }
        } catch (Exception e) {
            Log.w(TAG, "playback failed", e);
        } finally {
            mediaPlayer.release();
        }
    }
}
