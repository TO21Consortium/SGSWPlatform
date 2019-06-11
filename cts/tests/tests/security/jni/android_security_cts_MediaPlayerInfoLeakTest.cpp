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

#define LOG_TAG "MediaPlayerInfoLeakTest-JNI"

#include <jni.h>

#include <binder/Parcel.h>
#include <binder/IServiceManager.h>

#include <media/IMediaPlayer.h>
#include <media/IMediaPlayerService.h>
#include <media/IMediaPlayerClient.h>

#include <sys/stat.h>

using namespace android;

static status_t connectMediaPlayer(sp<IMediaPlayer>& iMP)
{
   sp<IServiceManager> sm = defaultServiceManager();
   sp<IBinder> mediaPlayerService = sm->checkService(String16("media.player"));

   sp<IMediaPlayerService> iMPService = IMediaPlayerService::asInterface(mediaPlayerService);
   sp<IMediaPlayerClient> client;
   Parcel data, reply;
   int dummyAudioSessionId = 1;
   data.writeInterfaceToken(iMPService->getInterfaceDescriptor());
   data.writeStrongBinder(IInterface::asBinder(client));
   data.writeInt32(dummyAudioSessionId);

   // Keep synchronized with IMediaPlayerService.cpp!
    enum {
        CREATE = IBinder::FIRST_CALL_TRANSACTION,
    };
   status_t err = IInterface::asBinder(iMPService)->transact(CREATE, data, &reply);
   if (err == NO_ERROR) {
       iMP = interface_cast<IMediaPlayer>(reply.readStrongBinder());
   }
   return err;
}

int testMediaPlayerInfoLeak(int command)
{
    sp<IMediaPlayer> iMP;
    if (NO_ERROR != connectMediaPlayer(iMP)) {
        return false;
    }


    Parcel data, reply;
    data.writeInterfaceToken(iMP->getInterfaceDescriptor());
    IInterface::asBinder(iMP)->transact(command, data, &reply);

    int leak = reply.readInt32();
    status_t err = reply.readInt32();
    return  leak;
}

jint android_security_cts_MediaPlayer_test_getCurrentPositionLeak(JNIEnv* env __unused,
                                                           jobject thiz __unused)
{
  // Keep synchronized with IMediaPlayer.cpp!
  enum {
      GET_CURRENT_POSITION = 16,
  };
  return testMediaPlayerInfoLeak(GET_CURRENT_POSITION);
}

jint android_security_cts_MediaPlayer_test_getDurationLeak(JNIEnv* env __unused,
                                                           jobject thiz __unused)
{
  // Keep synchronized with IMediaPlayer.cpp!
  enum {
      GET_DURATION = 17,
  };
  return testMediaPlayerInfoLeak(GET_DURATION);
}

static JNINativeMethod gMethods[] = {
    {  "native_test_getCurrentPositionLeak", "()I",
            (void *) android_security_cts_MediaPlayer_test_getCurrentPositionLeak },
    {  "native_test_getDurationLeak", "()I",
            (void *) android_security_cts_MediaPlayer_test_getDurationLeak },
};

int register_android_security_cts_MediaPlayerInfoLeakTest(JNIEnv* env)
{
    jclass clazz = env->FindClass("android/security/cts/MediaPlayerInfoLeakTest");
    return env->RegisterNatives(clazz, gMethods,
            sizeof(gMethods) / sizeof(JNINativeMethod));
}
