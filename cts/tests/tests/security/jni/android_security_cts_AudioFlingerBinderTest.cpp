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

#define LOG_TAG "AudioFlingerBinderTest-JNI"

#include <jni.h>
#include <binder/IServiceManager.h>
#include <media/IAudioFlinger.h>
#include <media/AudioSystem.h>
#include <system/audio.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>

using namespace android;

/*
 * Native methods used by
 * cts/tests/tests/security/src/android/security/cts/AudioFlingerBinderTest.java
 */

#define TEST_ARRAY_SIZE 10000
#define MAX_ARRAY_SIZE 1024
#define TEST_PATTERN 0x55

class MyDeathClient: public IBinder::DeathRecipient
{
public:
    MyDeathClient() :
        mAfIsDead(false) {
    }

    bool afIsDead() const { return mAfIsDead; }

    // DeathRecipient
    virtual void binderDied(const wp<IBinder>& who __unused) { mAfIsDead = true; }

private:
    bool mAfIsDead;
};


static bool connectAudioFlinger(sp<IAudioFlinger>& af, sp<MyDeathClient> &dr)
{
    int64_t startTime = 0;
    while (af == 0) {
        sp<IBinder> binder = defaultServiceManager()->checkService(String16("media.audio_flinger"));
        if (binder == 0) {
            if (startTime == 0) {
                startTime = uptimeMillis();
            } else if ((uptimeMillis()-startTime) > 10000) {
                ALOGE("timeout while getting audio flinger service");
                return false;
            }
            sleep(1);
        } else {
            af = interface_cast<IAudioFlinger>(binder);
            dr = new MyDeathClient();
            binder->linkToDeath(dr);
        }
    }
    return true;
}

/*
 * Checks that AudioSystem::setMasterMute() does not crash mediaserver if a duplicated output
 * is opened.
 */
jboolean android_security_cts_AudioFlinger_test_setMasterMute(JNIEnv* env __unused,
                                                           jobject thiz __unused)
{
    sp<IAudioFlinger> af;
    sp<MyDeathClient> dr;

    if (!connectAudioFlinger(af, dr)) {
        return false;
    }

    // force opening of a duplicating output
    status_t status = AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_REMOTE_SUBMIX,
                                          AUDIO_POLICY_DEVICE_STATE_AVAILABLE,
                                          "0", "");
    if (status != NO_ERROR) {
        return false;
    }

    bool mute;
    status = AudioSystem::getMasterMute(&mute);
    if (status != NO_ERROR) {
        return false;
    }

    AudioSystem::setMasterMute(!mute);

    sleep(1);

    // Check that mediaserver did not crash
    if (dr->afIsDead()) {
        return false;
    }

    AudioSystem::setMasterMute(mute);

    AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_REMOTE_SUBMIX,
                                          AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE,
                                          "0", "");

    AudioSystem::setMasterMute(false);

    return true;
}

jboolean android_security_cts_AudioFlinger_test_setMasterVolume(JNIEnv* env __unused,
                                                           jobject thiz __unused)
{
    sp<IAudioFlinger> af;
    sp<MyDeathClient> dr;

    if (!connectAudioFlinger(af, dr)) {
        return false;
    }

    // force opening of a duplicating output
    status_t status = AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_REMOTE_SUBMIX,
                                          AUDIO_POLICY_DEVICE_STATE_AVAILABLE,
                                          "0", "");
    if (status != NO_ERROR) {
        return false;
    }

    float vol;
    status = AudioSystem::getMasterVolume(&vol);
    if (status != NO_ERROR) {
        return false;
    }

    AudioSystem::setMasterVolume(vol < 0.5 ? 1.0 : 0.0);

    sleep(1);

    // Check that mediaserver did not crash
    if (dr->afIsDead()) {
        return false;
    }

    AudioSystem::setMasterMute(vol);

    AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_REMOTE_SUBMIX,
                                          AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE,
                                          "0", "");

    return true;
}

jboolean android_security_cts_AudioFlinger_test_listAudioPorts(JNIEnv* env __unused,
                                                           jobject thiz __unused)
{
    sp<IAudioFlinger> af;
    sp<MyDeathClient> dr;

    if (!connectAudioFlinger(af, dr)) {
        return false;
    }

    unsigned int num_ports = TEST_ARRAY_SIZE;
    struct audio_port *ports =
            (struct audio_port *)calloc(TEST_ARRAY_SIZE, sizeof(struct audio_port));

    memset(ports, TEST_PATTERN, TEST_ARRAY_SIZE * sizeof(struct audio_port));

    status_t status = af->listAudioPorts(&num_ports, ports);

    sleep(1);

    // Check that the memory content above the max allowed array size was not changed
    char *ptr = (char *)(ports + MAX_ARRAY_SIZE);
    for (size_t i = 0; i < TEST_ARRAY_SIZE - MAX_ARRAY_SIZE; i++) {
        if (ptr[i * sizeof(struct audio_port)] != TEST_PATTERN) {
            free(ports);
            return false;
        }
    }

    free(ports);

    // Check that mediaserver did not crash
    if (dr->afIsDead()) {
        return false;
    }

    return true;
}

jboolean android_security_cts_AudioFlinger_test_listAudioPatches(JNIEnv* env __unused,
                                                           jobject thiz __unused)
{
    sp<IAudioFlinger> af;
    sp<MyDeathClient> dr;

    if (!connectAudioFlinger(af, dr)) {
        return false;
    }

    unsigned int num_patches = TEST_ARRAY_SIZE;
    struct audio_patch *patches =
            (struct audio_patch *)calloc(TEST_ARRAY_SIZE, sizeof(struct audio_patch));

    memset(patches, TEST_PATTERN, TEST_ARRAY_SIZE * sizeof(struct audio_patch));

    status_t status = af->listAudioPatches(&num_patches, patches);

    sleep(1);

    // Check that the memory content above the max allowed array size was not changed
    char *ptr = (char *)(patches + MAX_ARRAY_SIZE);
    for (size_t i = 0; i < TEST_ARRAY_SIZE - MAX_ARRAY_SIZE; i++) {
        if (ptr[i * sizeof(struct audio_patch)] != TEST_PATTERN) {
            free(patches);
            return false;
        }
    }

    free(patches);

    // Check that mediaserver did not crash
    if (dr->afIsDead()) {
        return false;
    }

    return true;
}

jboolean android_security_cts_AudioFlinger_test_createEffect(JNIEnv* env __unused,
                                                             jobject thiz __unused)
{
    sp<IAudioFlinger> af;
    sp<MyDeathClient> dr;

    if (!connectAudioFlinger(af, dr)) {
        return false;
    }

    for (int j = 0; j < 10; ++j) {
        Parcel data, reply;
        data.writeInterfaceToken(af->getInterfaceDescriptor());
        data.writeInt32((int32_t)j);
        status_t status = af->asBinder(af)->transact(40, data, &reply); // 40 is CREATE_EFFECT
        if (status != NO_ERROR) {
            return false;
        }

        status = (status_t)reply.readInt32();
        if (status == NO_ERROR) {
            continue;
        }

        int id = reply.readInt32();
        int enabled = reply.readInt32();
        sp<IEffect> effect = interface_cast<IEffect>(reply.readStrongBinder());
        effect_descriptor_t desc;
        effect_descriptor_t descTarget;
        memset(&desc, 0, sizeof(effect_descriptor_t));
        memset(&descTarget, 0, sizeof(effect_descriptor_t));
        reply.read(&desc, sizeof(effect_descriptor_t));
        if (id != 0 || enabled != 0 || memcmp(&desc, &descTarget, sizeof(effect_descriptor_t))) {
            return false;
        }
    }

    sleep(1);

    // Check that mediaserver did not crash
    if (dr->afIsDead()) {
        return false;
    }

    return true;
}

static JNINativeMethod gMethods[] = {
    {  "native_test_setMasterMute", "()Z",
            (void *) android_security_cts_AudioFlinger_test_setMasterMute },
    {  "native_test_setMasterVolume", "()Z",
            (void *) android_security_cts_AudioFlinger_test_setMasterVolume },
    {  "native_test_listAudioPorts", "()Z",
            (void *) android_security_cts_AudioFlinger_test_listAudioPorts },
    {  "native_test_listAudioPatches", "()Z",
            (void *) android_security_cts_AudioFlinger_test_listAudioPatches },
    {  "native_test_createEffect", "()Z",
            (void *) android_security_cts_AudioFlinger_test_createEffect },
};

int register_android_security_cts_AudioFlingerBinderTest(JNIEnv* env)
{
    jclass clazz = env->FindClass("android/security/cts/AudioFlingerBinderTest");
    return env->RegisterNatives(clazz, gMethods,
            sizeof(gMethods) / sizeof(JNINativeMethod));
}
