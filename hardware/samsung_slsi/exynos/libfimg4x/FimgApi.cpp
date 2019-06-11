/*
**
** Copyright 2009 Samsung Electronics Co, Ltd.
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

#define LOG_NDEBUG 0
#define LOG_TAG "SKIA"
#include <utils/Log.h>

#include "FimgApi.h"

pthread_mutex_t s_g2d_lock = PTHREAD_MUTEX_INITIALIZER;

struct blitinfo_table optbl[] = {
    { (int)BLIT_OP_SOLID_FILL, "FILL" },
    { (int)BLIT_OP_CLR, "CLR" },
    { (int)BLIT_OP_SRC, "SRC" },
    { (int)BLIT_OP_DST, "DST" },
    { (int)BLIT_OP_SRC_OVER, "SRC_OVER" },
    { (int)BLIT_OP_DST_OVER, "DST_OVER" },
    { (int)BLIT_OP_SRC_IN, "SRC_IN" },
    { (int)BLIT_OP_DST_IN, "DST_IN" },
    { (int)BLIT_OP_SRC_OUT, "SRC_OUT" },
    { (int)BLIT_OP_DST_OUT, "DST_OUT" },
    { (int)BLIT_OP_SRC_ATOP, "SRC_ATOP" },
    { (int)BLIT_OP_DST_ATOP, "DST_ATOP" },
    { (int)BLIT_OP_XOR, "XOR" },
    { (int)BLIT_OP_ADD, "ADD" },
    { (int)BLIT_OP_MULTIPLY, "MULTIPLY" },
    { (int)BLIT_OP_SCREEN, "SCREEN" },
    { (int)BLIT_OP_DARKEN, "DARKEN" },
    { (int)BLIT_OP_LIGHTEN, "LIGHTEN" },
    { (int)BLIT_OP_DISJ_SRC_OVER, "DISJ_SRC_OVER" },
    { (int)BLIT_OP_DISJ_DST_OVER, "DISJ_DST_OVER" },
    { (int)BLIT_OP_DISJ_SRC_IN, "DISJ_SRC_IN" },
    { (int)BLIT_OP_DISJ_DST_IN, "DISJ_DST_IN" },
    { (int)BLIT_OP_DISJ_SRC_OUT, "DISJ_SRC_OUT" },
    { (int)BLIT_OP_DISJ_DST_OUT, "DISJ_DST_OUT" },
    { (int)BLIT_OP_DISJ_SRC_ATOP, "DISJ_SRC_ATOP" },
    { (int)BLIT_OP_DISJ_DST_ATOP, "DISJ_DST_ATOP" },
    { (int)BLIT_OP_DISJ_XOR, "DISJ_XOR" },
    { (int)BLIT_OP_CONJ_SRC_OVER, "CONJ_SRC_OVER" },
    { (int)BLIT_OP_CONJ_DST_OVER, "CONJ_DST_OVER" },
    { (int)BLIT_OP_CONJ_SRC_IN, "CONJ_SRC_IN" },
    { (int)BLIT_OP_CONJ_DST_IN, "CONJ_DST_IN" },
    { (int)BLIT_OP_CONJ_SRC_OUT, "CONJ_SRC_OUT" },
    { (int)BLIT_OP_CONJ_DST_OUT, "CONJ_DST_OUT" },
    { (int)BLIT_OP_CONJ_SRC_ATOP, "CONJ_SRC_ATOP" },
    { (int)BLIT_OP_CONJ_DST_ATOP, "CONJ_DST_ATOP" },
    { (int)BLIT_OP_CONJ_XOR, "CONJ_XOR" },
    { (int)BLIT_OP_USER_COEFF, "USER_COEFF" },
    { (int)BLIT_OP_END, "" },
};

struct blitinfo_table repeat_tbl[] = {
   { (int)NO_REPEAT, "NON" },
   { (int)REPEAT_NORMAL, "DEFAULT" },
   { (int)REPEAT_PAD, "PAD" },
   { (int)REPEAT_REFLECT, "REFLECT, MIRROR" },
   { (int)REPEAT_CLAMP, "CLAMP" },
};

#ifndef REAL_DEBUG
    void VOID_FUNC(const char *format, ...)
    {}
#endif

FimgApi::FimgApi()
{
    m_flagCreate = false;
}

FimgApi::~FimgApi()
{
    if (m_flagCreate == true)
        PRINT("%s::this is not Destroyed fail\n", __func__);
}

bool FimgApi::Create(void)
{
    bool ret = false;

    if (t_Lock() == false) {
        PRINT("%s::t_Lock() fail\n", __func__);
        goto CREATE_DONE;
    }

    if (m_flagCreate == true) {
        PRINT("%s::Already Created fail\n", __func__);
        goto CREATE_DONE;
    }

    if (t_Create() == false) {
        PRINT("%s::t_Create() fail\n", __func__);
        goto CREATE_DONE;
    }

    m_flagCreate = true;

    ret = true;

CREATE_DONE :

    t_UnLock();

    return ret;
}

bool FimgApi::Destroy(void)
{
    bool ret = false;

    if (t_Lock() == false) {
        PRINT("%s::t_Lock() fail\n", __func__);
        goto DESTROY_DONE;
    }

    if (m_flagCreate == false) {
        PRINT("%s::Already Destroyed fail\n", __func__);
        goto DESTROY_DONE;
    }

    if (t_Destroy() == false) {
        PRINT("%s::t_Destroy() fail\n", __func__);
        goto DESTROY_DONE;
    }

    m_flagCreate = false;

    ret = true;

DESTROY_DONE :

    t_UnLock();

    return ret;
}

bool FimgApi::Stretch(struct fimg2d_blit *cmd)
{
    bool ret = false;

    if (t_Lock() == false) {
        PRINT("%s::t_Lock() fail\n", __func__);
        goto STRETCH_DONE;
    }

    if (m_flagCreate == false) {
        PRINT("%s::This is not Created fail\n", __func__);
        goto STRETCH_DONE;
    }

    if (t_Stretch(cmd) == false) {
        goto STRETCH_DONE;
    }

    ret = true;

STRETCH_DONE :

    t_UnLock();

    return ret;
}

bool FimgApi::Sync(void)
{
    bool ret = false;

    if (m_flagCreate == false) {
        PRINT("%s::This is not Created fail\n", __func__);
        goto SYNC_DONE;
    }

    if (t_Sync() == false)
        goto SYNC_DONE;

    ret = true;

SYNC_DONE :

    return ret;
}

bool FimgApi::t_Create(void)
{
    PRINT("%s::This is empty virtual function fail\n", __func__);
    return false;
}

bool FimgApi::t_Destroy(void)
{
    PRINT("%s::This is empty virtual function fail\n", __func__);
    return false;
}

bool FimgApi::t_Stretch(struct fimg2d_blit *cmd)
{
    PRINT("%s::This is empty virtual function fail\n", __func__);
    return false;
}

bool FimgApi::t_Sync(void)
{
    PRINT("%s::This is empty virtual function fail\n", __func__);
    return false;
}

bool FimgApi::t_Lock(void)
{
    PRINT("%s::This is empty virtual function fail\n", __func__);
    return false;
}

bool FimgApi::t_UnLock(void)
{
    PRINT("%s::This is empty virtual function fail\n", __func__);
    return false;
}

//---------------------------------------------------------------------------//
// extern function
//---------------------------------------------------------------------------//
extern "C" int stretchFimgApi(struct fimg2d_blit *cmd)
{
    pthread_mutex_lock(&s_g2d_lock);

    FimgApi * fimgApi = createFimgApi();

    if (fimgApi == NULL) {
        PRINT("%s::createFimgApi() fail\n", __func__);
        pthread_mutex_unlock(&s_g2d_lock);
        return -1;
    }

    if (fimgApi->Stretch(cmd) == false) {
        if (fimgApi != NULL)
            destroyFimgApi(fimgApi);

        pthread_mutex_unlock(&s_g2d_lock);
        return -1;
    }

    if (fimgApi != NULL)
        destroyFimgApi(fimgApi);

    pthread_mutex_unlock(&s_g2d_lock);
    return 0;
}

extern "C" int stretchFimgApi_fast(struct fimg2d_blit *cmd, unsigned long tmpbuf_addr, int tmpbuf_size)
{
    if (tmpbuf_addr == 0 || tmpbuf_size <= 0)
        return stretchFimgApi(cmd);

    /* scaling & rotation only */
    if (cmd->param.rotate == ORIGIN || cmd->param.scaling.mode == NO_SCALING)
        return stretchFimgApi(cmd);

    /* src & dst only */
    if (cmd->src == NULL || cmd->msk != NULL)
        return stretchFimgApi(cmd);

    /* a(x)rgb8888 src only */
    if (cmd->src->fmt >= CF_RGB_565)
        return stretchFimgApi(cmd);

    FimgApi * fimgApi = createFimgApi();

    if (fimgApi == NULL) {
        PRINT("%s::createFimgApi() fail\n", __func__);
        return -1;
    }

    bool is_scaledown, sr_w, sr_h;
    struct fimg2d_image tmpbuf;
    struct fimg2d_blit cmd1st, cmd2nd;
    struct fimg2d_param *p;

    /* check is_scaledown */
    p = &cmd->param;
    sr_w = p->scaling.src_w - p->scaling.dst_w;
    sr_h = p->scaling.src_h - p->scaling.dst_h;
    is_scaledown = (sr_w + sr_h > 0) ? true : false;

    if (is_scaledown) {
        /* set temp buffer */
        tmpbuf.width = cmd->dst->rect.y2 - cmd->dst->rect.y1;
        tmpbuf.height = cmd->dst->rect.x2 - cmd->dst->rect.x1;
        tmpbuf.stride = tmpbuf.width * 4;
        tmpbuf.order = cmd->src->order;
        tmpbuf.fmt = cmd->src->fmt;
        tmpbuf.addr.type = cmd->src->addr.type;
        tmpbuf.addr.start = tmpbuf_addr;
        tmpbuf.plane2.type = ADDR_NONE;
        tmpbuf.rect.x1 = 0;
        tmpbuf.rect.y1 = 0;
        tmpbuf.rect.x2 = tmpbuf.width;
        tmpbuf.rect.y2 = tmpbuf.height;
        tmpbuf.need_cacheopr = false;

        /* 1st step : copy with scaling down */
        p = &cmd1st.param;
        memcpy(p, &cmd->param, sizeof(cmd->param));
        p->rotate = ORIGIN;
        p->g_alpha = 0xff;
        p->dither = false;
        cmd1st.op = BLIT_OP_SRC;
        cmd1st.src = cmd->src;
        cmd1st.dst = &tmpbuf;
        cmd1st.msk = NULL;
        cmd1st.tmp = NULL;
        cmd1st.sync = BLIT_SYNC;
        cmd1st.seq_no = cmd->seq_no;

        /* 2nd step : op with rotation */
        p = &cmd2nd.param;
        memcpy(p, &cmd->param, sizeof(cmd->param));
        p->scaling.mode = NO_SCALING;
        cmd2nd.op = cmd->op;
        cmd2nd.src = &tmpbuf;
        cmd2nd.dst = cmd->dst;
        cmd2nd.msk = NULL;
        cmd2nd.tmp = NULL;
        cmd2nd.sync = BLIT_SYNC;
        cmd2nd.seq_no = cmd->seq_no;
    } else {
        /* set temp buffer */
        tmpbuf.width = cmd->src->rect.y2 - cmd->src->rect.y1;
        tmpbuf.height = cmd->src->rect.x2 - cmd->src->rect.x1;
        tmpbuf.stride = tmpbuf.width * 4;
        tmpbuf.order = cmd->src->order;
        tmpbuf.fmt = cmd->src->fmt;
        tmpbuf.addr.type = cmd->src->addr.type;
        tmpbuf.addr.start = tmpbuf_addr;
        tmpbuf.plane2.type = ADDR_NONE;
        tmpbuf.rect.x1 = 0;
        tmpbuf.rect.y1 = 0;
        tmpbuf.rect.x2 = tmpbuf.width;
        tmpbuf.rect.y2 = tmpbuf.height;
        tmpbuf.need_cacheopr = false;

        /* 1st step : copy with rotation */
        p = &cmd1st.param;
        memcpy(p, &cmd->param, sizeof(cmd->param));
        p->scaling.mode = NO_SCALING;
        p->g_alpha = 0xff;
        p->dither = false;
        cmd1st.op = BLIT_OP_SRC;
        cmd1st.src = cmd->src;
        cmd1st.dst = &tmpbuf;
        cmd1st.msk = NULL;
        cmd1st.tmp = NULL;
        cmd1st.sync = BLIT_SYNC;
        cmd1st.seq_no = cmd->seq_no;

        /* 2nd step : op with scaling up */
        p = &cmd2nd.param;
        memcpy(p, &cmd->param, sizeof(cmd->param));
        p->rotate = ORIGIN;
        cmd2nd.op = cmd->op;
        cmd2nd.src = &tmpbuf;
        cmd2nd.dst = cmd->dst;
        cmd2nd.msk = NULL;
        cmd2nd.tmp = NULL;
        cmd2nd.sync = BLIT_SYNC;
        cmd2nd.seq_no = cmd->seq_no;
    }

    /* 1st step blit */
    if (fimgApi->Stretch(&cmd1st) == false) {
        if (fimgApi != NULL)
            destroyFimgApi(fimgApi);
        return -1;
    }

    /* 2nd step blit */
    if (fimgApi->Stretch(&cmd2nd) == false) {
        if (fimgApi != NULL)
            destroyFimgApi(fimgApi);
        return -1;
    }

    if (fimgApi != NULL)
        destroyFimgApi(fimgApi);

    return 0;
}

extern "C" int SyncFimgApi(void)
{
    pthread_mutex_lock(&s_g2d_lock);
    FimgApi * fimgApi = createFimgApi();
    if (fimgApi == NULL) {
        PRINT("%s::createFimgApi() fail\n", __func__);
        pthread_mutex_unlock(&s_g2d_lock);
        return -1;
    }

    if (fimgApi->Sync() == false) {
        if (fimgApi != NULL)
            destroyFimgApi(fimgApi);

        pthread_mutex_unlock(&s_g2d_lock);
        return -1;
    }

    if (fimgApi != NULL)
        destroyFimgApi(fimgApi);

    pthread_mutex_unlock(&s_g2d_lock);
    return 0;
}

void printDataBlit(char *title, const char *called, struct fimg2d_blit *cmd)
{
    SLOGI("%s (From %s)\n", title, called);
    SLOGI("    sequence_no. = %u\n", cmd->seq_no);
    SLOGI("    blit_op      = %d(%s)\n", cmd->op, optbl[cmd->op].str);
    SLOGI("    fill_color   = %X\n", cmd->param.solid_color);
    SLOGI("    global_alpha = %u\n", (unsigned int)cmd->param.g_alpha);
    SLOGI("    PREMULT      = %s\n", cmd->param.premult == PREMULTIPLIED ? "PREMULTIPLIED" : "NON-PREMULTIPLIED");
    SLOGI("    do_dither    = %s\n", cmd->param.dither == true ? "dither" : "no-dither");
    SLOGI("    repeat       = %d(%s)\n", cmd->param.repeat.mode,
              repeat_tbl[cmd->param.repeat.mode].str);

    printDataBlitRotate(cmd->param.rotate);

    printDataBlitScale(&cmd->param.scaling);

    printDataBlitImage("SRC", cmd->src);
    printDataBlitImage("DST", cmd->dst);

    if (cmd->src != NULL)
        printDataBlitRect("SRC", &cmd->src->rect);
    if (cmd->dst != NULL)
        printDataBlitRect("DST", &cmd->dst->rect);
}

void printDataBlitImage(const char *title, struct fimg2d_image *image)
{
    if (NULL != image) {
        SLOGI("    Image_%s\n", title);
        SLOGI("        addr = %X\n", image->addr.start);
        SLOGI("        format = %d\n", image->fmt);
        SLOGI("        size = (%d, %d)\n", image->width, image->height);
    } else {
        SLOGI("    Image_%s : NULL\n", title);
    }
}

void printDataBlitRect(const char *title, struct fimg2d_rect *rect)
{
    if (NULL != rect) {
        SLOGI("    RECT_%s\n", title);
        SLOGI("        (x1, y1) = (%d, %d)\n", rect->x1, rect->y1);
        SLOGI("        (x2, y2) = (%d, %d)\n", rect->x2, rect->y2);
        SLOGI("        (width, height) = (%d, %d)\n", rect->x2 - rect->x1, rect->y2 - rect->y1);
    } else
        SLOGI("    RECT_%s : NULL\n", title);
}

void printDataBlitRotate(int rotate)
{
    SLOGI("    ROTATE : %d\n", rotate);
}

void printDataBlitScale(struct fimg2d_scale *scaling)
{
    SLOGI("    SCALING\n");
    if (scaling->mode != 0) {
        SLOGI("        scale_mode : %s\n", (scaling->mode == 1 ? "SCALING_NEAREST" : "SCALING_BILINEAR"));
        SLOGI("        src : (src_w, src_h) = (%d, %d)\n", scaling->src_w, scaling->src_h);
        SLOGI("        dst : (dst_w, dst_h) = (%d, %d)\n", scaling->dst_w, scaling->dst_h);
        SLOGI("        scaling_factor : (scale_w, scale_y) = (%3.2f, %3.2f)\n",
				(double)scaling->dst_w / scaling->src_w, (double)scaling->dst_h / scaling->src_h);
    } else {
        SLOGI("        scale_mode : NO_SCALING\n");
    }
}
