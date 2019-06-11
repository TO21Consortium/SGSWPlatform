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

import junit.framework.TestCase;

public class MediaPlayerInfoLeakTest extends TestCase {

    static {
        System.loadLibrary("ctssecurity_jni");
    }


    /**
     * Checks that IMediaPlayer::getCurrentPosition() does not leak info in error case
     */
    public void test_getCurrentPositionLeak() throws Exception {
        int pos = native_test_getCurrentPositionLeak();
        assertTrue(String.format("Leaked pos 0x%08X", pos), pos == 0);
    }

    /**
     * Checks that IMediaPlayer::getDuration() does not leak info in error case
     */
    public void test_getDurationLeak() throws Exception {
        int dur = native_test_getDurationLeak();
        assertTrue(String.format("Leaked dur 0x%08X", dur), dur == 0);
    }

    private static native int native_test_getCurrentPositionLeak();
    private static native int native_test_getDurationLeak();
}
