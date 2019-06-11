/*
**
** Copyright 2009 Samsung Electronics Co, Ltd.
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
**
*/

#ifndef FIMG_API_H
#define FIMG_API_H

#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>

#if defined(FIMGAPI_V5X)
#include "sec_g2d_5x.h"
#else
#include "sec_g2d_4x.h"
#endif

#undef FIMGAPI_HAL_DEBUG
#undef REAL_DEBUG
#undef ANDROID_LOG

#if defined(REAL_DEBUG)
#ifdef ANDROID_LOG
#define PRINT  SLOGE
#define PRINTD SLOGD
#else
#define PRINT  printf
#define PRINTD printf
#endif
#else
void VOID_FUNC(const char *format, ...);

#define PRINT  VOID_FUNC
#define PRINTD VOID_FUNC
#endif

struct Fimg {
    int srcX;
    int srcY;
    int srcW;
    int srcH;
    int srcFWStride; // this is not w, just stride (w * bpp)
    int srcFH;
    int srcBPP;
    int srcColorFormat;
    int srcAlphaType;
    unsigned char* srcAddr;

    int dstX;
    int dstY;
    int dstW;
    int dstH;
    int dstFWStride; // this is not w, just stride (w * bpp)
    int dstFH;
    int dstBPP;
    int dstColorFormat;
    int dstAlphaType;
    unsigned char* dstAddr;

    int clipT;
    int clipB;
    int clipL;
    int clipR;
    int level;

    unsigned int fillcolor;
    int tileModeX;
    int tileModeY;
    int rotate;
    unsigned int alpha;
    int xfermode;
    int isDither;
    int isFilter;
    int matrixType;
    float matrixSw;
    float matrixSh;

    int called; /* 0 : drawRect 1 : drawBitmap */
};

#ifdef __cplusplus

struct blitinfo_table {
    int op;
    const char *str;
};

struct compromise_param {
    int clipW;
    int clipH;
    int src_fmt;
    int dst_fmt;
    int isScaling;
    int isFilter;
    int isSrcOver;
};

extern struct blitinfo_table optbl[];

class FimgApi
{
public:
#endif

#ifdef __cplusplus
private :
    bool    m_flagCreate;

protected :
    FimgApi();
    FimgApi(const FimgApi& rhs) {}
    virtual ~FimgApi();

public:
    bool        Create(void);
    bool        Destroy(void);
    inline bool FlagCreate(void) { return m_flagCreate; }
    bool        Stretch(struct fimg2d_blit *cmd);
#ifdef FIMG2D_USE_M2M1SHOT2
    bool        Stretch_v5(struct m2m1shot2 *cmd);
#endif
    bool        Sync(void);

protected:
    virtual bool t_Create(void);
    virtual bool t_Destroy(void);
    virtual bool t_Stretch(struct fimg2d_blit *cmd);
#ifdef FIMG2D_USE_M2M1SHOT2
    virtual bool t_Stretch_v5(struct m2m1shot2 *cmd);
#endif
    virtual bool t_Sync(void);
    virtual bool t_Lock(void);
    virtual bool t_UnLock(void);

};
#endif

#ifdef __cplusplus
extern "C"
#endif
struct FimgApi *createFimgApi();

#ifdef __cplusplus
extern "C"
#endif
void destroyFimgApi(FimgApi *ptrFimgApi);

#ifdef __cplusplus
extern "C"
#endif
int stretchFimgApi(struct fimg2d_blit *cmd);

#ifdef FIMG2D_USE_M2M1SHOT2
#ifdef __cplusplus
extern "C"
#endif
int stretchFimgApi_v5(struct m2m1shot2 *cmd);
#endif

#ifdef __cplusplus
extern "C"
#endif
int stretchFimgApi_fast(struct fimg2d_blit *cmd, unsigned long temp_addr, int temp_size);

#ifdef __cplusplus
extern "C"
#endif
bool checkScaleFimgApi(Fimg *fimg);


#ifdef __cplusplus
extern "C"
#endif
int SyncFimgApi(void);

int requestFimgApi_v5(struct Fimg *fimg);
void printDataBlit(char *title, const char *called, struct fimg2d_blit *cmd);
void printDataBlitRotate(int rotate);
void printDataBlitImage(const char *title, struct fimg2d_image *image);
void printDataBlitRect(const char *title, struct fimg2d_rect *rect);
void printDataBlitRect(const char *title, struct fimg2d_clip *clipping);
void printDataBlitScale(struct fimg2d_scale *scaling);
#endif //FIMG_API_H
