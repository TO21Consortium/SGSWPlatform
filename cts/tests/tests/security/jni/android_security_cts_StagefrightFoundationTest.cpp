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

#define LOG_TAG "AudioEffectBinderTest-JNI"

#include <cstdio>
#include <jni.h>
#include <binder/Parcel.h>
#include <media/stagefright/foundation/AMessage.h>

using namespace android;

/*
 * Native methods used by
 * cts/tests/tests/security/src/android/security/cts/StagefrightFoundationTest.java
 */

static jboolean android_security_cts_StagefrightFoundation_test_aMessageFromParcel(
        JNIEnv* env __unused, jobject thiz __unused)
{
    const int kMaxNumItems = 64;
    const int kNumItems = kMaxNumItems + 1 + 1000;
    char name[128];

    Parcel data;
    data.writeInt32(0);  // what
    data.writeInt32(kNumItems);  // numItems
    for (int i = 0; i < kMaxNumItems; ++i) {
        snprintf(name, sizeof(name), "item-%d", i);
        data.writeCString(name);  // name
        data.writeInt32(0);  // kTypeInt32
        data.writeInt32(i);  // value
    }
    data.writeCString("evil");  // name
    data.writeInt32(0);  // kTypeInt32
    data.writeInt32(0);  // value
    // NOTE: This could overwrite mNumItems!

    for (int i = 0; i < 1000; ++i) {
        snprintf(name, sizeof(name), "evil-%d", i);
        data.writeCString(name);  // name
        data.writeInt32(0);  // kTypeInt32
        data.writeInt32(0);  // value
    }

    data.setDataPosition(0);
    sp<AMessage> msg = AMessage::FromParcel(data);

    for (int i = 0; i < kMaxNumItems; ++i) {
        snprintf(name, sizeof(name), "item-%d", i);
        int32_t value;
        if (!msg->findInt32(name, &value)) {
            ALOGE("cannot find value for %s", name);
            return JNI_FALSE;
        }
        if (value != i) {
            ALOGE("value is changed: expected %d actual %d", i, value);
            return JNI_FALSE;
        }
    }
    return JNI_TRUE;
}

int register_android_security_cts_StagefrightFoundationTest(JNIEnv *env)
{
    static JNINativeMethod methods[] = {
        { "native_test_aMessageFromParcel", "()Z",
                (void *) android_security_cts_StagefrightFoundation_test_aMessageFromParcel},
    };

    jclass clazz = env->FindClass("android/security/cts/StagefrightFoundationTest");
    return env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0]));
}
