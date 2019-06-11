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

#include <jni.h>
#include <media/AudioEffect.h>
#include <media/IEffect.h>

using namespace android;

/*
 * Native methods used by
 * cts/tests/tests/security/src/android/security/cts/AudioEffectBinderTest.java
 */

struct EffectClient : public BnEffectClient {
    EffectClient() { }
    virtual void controlStatusChanged(bool controlGranted __unused) { }
    virtual void enableStatusChanged(bool enabled __unused) { }
    virtual void commandExecuted(uint32_t cmdCode __unused,
            uint32_t cmdSize __unused,
            void *pCmdData __unused,
            uint32_t replySize __unused,
            void *pReplyData __unused) { }
};

struct DeathRecipient : public IBinder::DeathRecipient {
    DeathRecipient() : mDied(false) { }
    virtual void binderDied(const wp<IBinder>& who __unused) { mDied = true; }
    bool died() const { return mDied; }
    bool mDied;
};

static bool isIEffectCommandSecure(IEffect *effect)
{
    // some magic constants here
    const int COMMAND_SIZE = 1024 + 12; // different than reply size to get different heap frag
    char cmdData[COMMAND_SIZE];
    memset(cmdData, 0xde, sizeof(cmdData));

    const int REPLY_DATA_SIZE = 256;
    char replyData[REPLY_DATA_SIZE];
    bool secure = true;
    for (int k = 0; k < 10; ++k) {
        Parcel data;
        data.writeInterfaceToken(effect->getInterfaceDescriptor());
        data.writeInt32(0);  // 0 is EFFECT_CMD_INIT
        data.writeInt32(sizeof(cmdData));
        data.write(cmdData, sizeof(cmdData));
        data.writeInt32(sizeof(replyData));

        Parcel reply;
        status_t status = effect->asBinder(effect)->transact(3, data, &reply);  // 3 is COMMAND
        ALOGV("transact status: %d", status);
        if (status != NO_ERROR) {
            ALOGW("invalid transaction status %d", status);
            continue;
        }

        ALOGV("reply data avail %zu", reply.dataAvail());
        status = reply.readInt32();
        ALOGV("reply status %d", status);
        if (status == NO_ERROR) {
            continue;
        }

        int size = reply.readInt32();
        ALOGV("reply size %d", size);
        if (size != sizeof(replyData)) { // we expect 0 or full reply data if command failed
            ALOGW_IF(size != 0, "invalid reply size: %d", size);
            continue;
        }

        // Note that if reply.read() returns success, it should completely fill replyData.
        status = reply.read(replyData, sizeof(replyData));
        if (status != NO_ERROR) {
            ALOGW("invalid reply read - ignoring");
            continue;
        }
        unsigned int *out = (unsigned int *)replyData;
        for (size_t index = 0; index < sizeof(replyData) / sizeof(*out); ++index) {
            if (out[index] != 0) {
                secure = false;
                ALOGI("leaked data = %#08x", out[index]);
            }
        }
    }
    ALOGI("secure: %s", secure ? "YES" : "NO");
    return secure;
}

static jboolean android_security_cts_AudioEffect_test_isCommandSecure()
{
    const sp<IAudioFlinger> &audioFlinger = AudioSystem::get_audio_flinger();
    if (audioFlinger.get() == NULL) {
        ALOGE("could not get audioflinger");
        return JNI_FALSE;
    }

    static const effect_uuid_t EFFECT_UIID_EQUALIZER =  // type
        { 0x0bed4300, 0xddd6, 0x11db, 0x8f34, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b }};
    sp<EffectClient> effectClient(new EffectClient());
    effect_descriptor_t descriptor;
    memset(&descriptor, 0, sizeof(descriptor));
    descriptor.type = EFFECT_UIID_EQUALIZER;
    descriptor.uuid = *EFFECT_UUID_NULL;
    const int32_t priority = 0;
    const int sessionId = AUDIO_SESSION_OUTPUT_MIX;
    const audio_io_handle_t io = AUDIO_IO_HANDLE_NONE;
    const String16 opPackageName("Exploitable");
    status_t status;
    int32_t id;
    int enabled;
    sp<IEffect> effect = audioFlinger->createEffect(&descriptor, effectClient,
            priority, io, sessionId, opPackageName, &status, &id, &enabled);
    if (effect.get() == NULL || status != NO_ERROR) {
        ALOGW("could not create effect");
        return JNI_TRUE;
    }

    sp<DeathRecipient> deathRecipient(new DeathRecipient());
    IInterface::asBinder(effect)->linkToDeath(deathRecipient);

    // check exploit
    if (!isIEffectCommandSecure(effect.get())) {
        ALOGE("not secure!");
        return JNI_FALSE;
    }

    sleep(1); // wait to check death
    if (deathRecipient->died()) {
        ALOGE("effect binder died");
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

int register_android_security_cts_AudioEffectBinderTest(JNIEnv *env)
{
    static JNINativeMethod methods[] = {
        { "native_test_isCommandSecure", "()Z",
                (void *) android_security_cts_AudioEffect_test_isCommandSecure },
    };

    jclass clazz = env->FindClass("android/security/cts/AudioEffectBinderTest");
    return env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0]));
}
