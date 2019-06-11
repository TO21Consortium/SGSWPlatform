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

public class AudioFlingerBinderTest extends TestCase {

    static {
        System.loadLibrary("ctssecurity_jni");
    }

    /**
     * Checks that AudioSystem::setMasterMute() does not crash mediaserver if a duplicated output
     * is opened.
     */
    public void test_setMasterMute() throws Exception {
        assertTrue(native_test_setMasterMute());
    }

    /**
     * Checks that AudioSystem::setMasterVolume() does not crash mediaserver if a duplicated output
     * is opened.
     */
    public void test_setMasterVolume() throws Exception {
        assertTrue(native_test_setMasterVolume());
    }

    /**
     * Checks that IAudioFlinger::listAudioPorts() does not cause a memory overflow when passed a
     * large number of ports.
     */
    public void test_listAudioPorts() throws Exception {
        assertTrue(native_test_listAudioPorts());
    }

    /**
     * Checks that IAudioFlinger::listAudioPatches() does not cause a memory overflow when passed a
     * large number of ports.
     */
    public void test_listAudioPatches() throws Exception {
        assertTrue(native_test_listAudioPatches());
    }

    /**
     * Checks that IAudioFlinger::createEffect() does not leak information on the server side.
     */
    public void test_createEffect() throws Exception {
        assertTrue(native_test_createEffect());
    }

    private static native boolean native_test_setMasterMute();
    private static native boolean native_test_setMasterVolume();
    private static native boolean native_test_listAudioPorts();
    private static native boolean native_test_listAudioPatches();
    private static native boolean native_test_createEffect();
}
