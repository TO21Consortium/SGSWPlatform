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

import android.media.audiofx.AudioEffect;

import java.util.UUID;

import junit.framework.TestCase;

public class AudioEffectBinderTest extends TestCase {

    static {
        System.loadLibrary("ctssecurity_jni");
    }

    /**
     * Checks that IEffect::command() cannot leak data.
     */
    public void test_isCommandSecure() throws Exception {
        if (isEffectTypeAvailable(AudioEffect.EFFECT_TYPE_EQUALIZER)) {
            assertTrue(native_test_isCommandSecure());
        }
    }

    /* see AudioEffect.isEffectTypeAvailable(), implements hidden function */
    private static boolean isEffectTypeAvailable(UUID type) {
        AudioEffect.Descriptor[] desc = AudioEffect.queryEffects();
        if (desc == null) {
            return false;
        }

        for (int i = 0; i < desc.length; i++) {
            if (desc[i].type.equals(type)) {
                return true;
            }
        }
        return false;
    }

    private static native boolean native_test_isCommandSecure();
}
