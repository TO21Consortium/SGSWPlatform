/*
*
* Copyright 2012 Samsung Electronics S.LSI Co. LTD
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>


#include "seiren_hw.h"

#define LOG_NDEBUG 1
#define LOG_TAG "libseirenhw"
#include <utils/Log.h>

#define MAX_INSTANCE 10

struct instance_info {
    unsigned int handle;
    void *ibuf_addr;
    void *obuf_addr;
};
static struct instance_info inst_info[MAX_INSTANCE];
static struct audio_mem_info_t ibuf_info;
static struct audio_mem_info_t obuf_info;
static int created = 0;
static int seiren_dev = -1;

int ADec_getIdx(unsigned int handle)
{
    int i;
    for (i = 0; i<MAX_INSTANCE; i++)
        if (handle == inst_info[i].handle)
            return i;
    ALOGE("%s: err", __func__);
    return -1;
}

int ADec_allocIdx(void)
{
    int i;
    for (i = 0; i<MAX_INSTANCE; i++)
        if ((int) inst_info[i].handle == -1)
            return i;
    ALOGE("%s: err", __func__);
    return -1;
}

int ADec_Create(
    u32 ulPlayerID __unused,
    SEIREN_IPTYPE ipType,
    u32* pulHandle __unused)
{
    int ret;
    int index;
    int i;

    if (!created) {
        for (i = 0; i<MAX_INSTANCE; i++)
            inst_info[i].handle = -1;
        created = 1;
        ALOGD("%s: initialized", __func__);
    }

    index = ADec_allocIdx();
    if (index == -1) {
        ALOGE("Index is full.");
        return -1;
    }

    seiren_dev = open(ADEC_DEV_NAME, O_RDWR);
    ALOGD("%s: called. handle:%d", __func__, seiren_dev);
    if (seiren_dev >= 0) {
        ret = ioctl(seiren_dev, SEIREN_IOCTL_CH_CREATE, ipType);
        if (ret != 0) {
            ALOGE("%s: ch_create ret: %d", __func__, ret);
            goto EXIT_ERROR;
        }
        inst_info[index].handle = seiren_dev;
        ALOGD("%s: successed to open AudH, index:%d", __func__,index);
    }
    else {
        ALOGE("%s: failed to open AudH", __func__);
    }

    goto EXIT;
EXIT_ERROR:
    close(seiren_dev);
    seiren_dev = -1;
EXIT:
    return seiren_dev;
}

int ADec_Destroy(u32 ulHandle)
{
    int index = ADec_getIdx(ulHandle);
    int ret;

    if (index == -1) {
        ALOGE("Can't find index.");
        return -1;
    }

    ALOGD("%s: index:%d,ibuf_addr:%p,obuf_addr:%p", __func__,
           index, inst_info[index].ibuf_addr, inst_info[index].obuf_addr);
    inst_info[index].handle = -1;
    free(inst_info[index].ibuf_addr);
    free(inst_info[index].obuf_addr);

    ret = ioctl(ulHandle, SEIREN_IOCTL_CH_DESTROY, ulHandle);
    if (ret != 0)
        ALOGE("%s: ch_destroy ret: %d", __func__, ret);

    ALOGD("%s: called, handle:%d", __func__, ulHandle);

    if (close(ulHandle) != 0) {
        ALOGE("%s: failed to close", __func__);
        return -1;
    }
    else {
        ALOGD("%s: successed to close", __func__);
    }

    return 0;
}

int ADec_SendStream(u32 ulHandle, audio_mem_info_t* pInputInfo, int* consumedSize)
{
    int index = ADec_getIdx(ulHandle);

    ALOGV("%s: handle:%d, buf_addr[%p], buf_size[%d]", __func__,
               ulHandle, pInputInfo->virt_addr, pInputInfo->data_size);
    *consumedSize = write(ulHandle, pInputInfo->virt_addr, pInputInfo->data_size);

    ALOGV("%s: consumedSize: %d", __func__, *consumedSize);

    return (*consumedSize < 0 ? *consumedSize : 0);
}

int ADec_DoEQ(u32 ulHandle, audio_mem_info_t* pMemInfo)
{
    int index = ADec_getIdx(ulHandle);

    ALOGD("%s: handle:%d, buf_addr[%p], buf_size[%d]", __func__,
               ulHandle, pMemInfo->virt_addr, pMemInfo->mem_size);
    ioctl(ulHandle, SEIREN_IOCTL_CH_EXE, pMemInfo);

    return 0;
}

int ADec_RecvPCM(u32 ulHandle, audio_mem_info_t* pOutputInfo)
{
    int index = ADec_getIdx(ulHandle);
    int pcm_size;

    ALOGV("%s: handle:%d, buf_addr[%p], buf_size[%d]", __func__,
               ulHandle, pOutputInfo->virt_addr, pOutputInfo->mem_size);
    pcm_size = read(ulHandle, pOutputInfo->virt_addr, pOutputInfo->mem_size);

    pOutputInfo->data_size = pcm_size;

    ALOGV("%s: pcm_size : %d", __func__, pOutputInfo->data_size);

    return 0;
}

int ADec_SetParams(u32 ulHandle, SEIREN_PARAMCMD paramCmd, unsigned long pulValues)
{
    u32 cmd = paramCmd << 16;
    cmd |= SEIREN_IOCTL_CH_SET_PARAMS;

    ALOGD("%s: called. handle:%d", __func__, ulHandle);
    ioctl(ulHandle, cmd, pulValues);

    return 0;
}

int ADec_GetParams(u32 ulHandle, SEIREN_PARAMCMD paramCmd, unsigned long *pulValues)
{
    u32 cmd = paramCmd << 16;

    cmd |= SEIREN_IOCTL_CH_GET_PARAMS;
    ioctl(ulHandle, cmd, pulValues);
    ALOGD("%s: val:%lu. handle:%d", __func__, *pulValues, ulHandle);

    return 0;
}

int ADec_SendEOS(u32 ulHandle)
{
    u32 cmd = ADEC_PARAM_SET_EOS << 16;
    cmd |= SEIREN_IOCTL_CH_SET_PARAMS;

    ALOGD("%s: called. handle:%d", __func__, ulHandle);
    ioctl(ulHandle, cmd);

    return 0;
}

int ADec_Flush(u32 ulHandle, SEIREN_PORTTYPE portType)
{
    ALOGD("%s: called. handle:%d", __func__, ulHandle);
    ioctl(ulHandle, SEIREN_IOCTL_CH_FLUSH, portType);

    return 0;
}

int ADec_ConfigSignal(u32 ulHandle)
{
    ALOGD("%s: called. handle:%d", __func__, ulHandle);
    ioctl(ulHandle, SEIREN_IOCTL_CH_CONFIG);

    return 0;
}

int ADec_GetPCMParams(u32 ulHandle, u32* pulValues)
{
    u32 cmd = PCM_CONFIG_INFO << 16;
    cmd |= SEIREN_IOCTL_CH_GET_PARAMS;

    ALOGD("%s: called. handle:%d", __func__, ulHandle);
    ioctl(ulHandle, cmd, &pulValues);

    return 0;
}

int ADec_GetIMemPoolInfo(u32 ulHandle, audio_mem_info_t* pIMemPoolInfo)
{
    u32 cmd = GET_IBUF_POOL_INFO << 16;
    cmd |= SEIREN_IOCTL_CH_GET_PARAMS;
    int index = ADec_getIdx(ulHandle);

    if (index == -1) {
        ALOGE("Can't find index.");
        return -1;
    }

    ALOGD("%s: called. handle:%d", __func__, ulHandle);
    ioctl(ulHandle, cmd, &ibuf_info);

    ibuf_info.virt_addr = malloc(ibuf_info.mem_size);
    inst_info[index].ibuf_addr = ibuf_info.virt_addr;

    pIMemPoolInfo->virt_addr = ibuf_info.virt_addr;
    pIMemPoolInfo->mem_size = ibuf_info.mem_size;
    pIMemPoolInfo->block_count = ibuf_info.block_count;

    ALOGD("%s: I_vaddr[%p], I_paddr[%p], I_size[%d], I_cnt[%d]",
            __func__,
            ibuf_info.virt_addr,
            ibuf_info.phy_addr,
            ibuf_info.mem_size,
            ibuf_info.block_count);

    return 0;
}

int ADec_GetOMemPoolInfo(u32 ulHandle, audio_mem_info_t* pOMemPoolInfo)
{
    u32 cmd = GET_OBUF_POOL_INFO << 16;
    cmd |= SEIREN_IOCTL_CH_GET_PARAMS;
    int index = ADec_getIdx(ulHandle);

    if (index == -1) {
        ALOGE("Can't find index.");
        return -1;
    }

    ALOGD("%s: called. handle:%d", __func__, ulHandle);
    ioctl(ulHandle, cmd, &obuf_info);

    obuf_info.virt_addr = malloc(obuf_info.mem_size);
    inst_info[index].obuf_addr = obuf_info.virt_addr;

    pOMemPoolInfo->virt_addr = obuf_info.virt_addr;
    pOMemPoolInfo->mem_size = obuf_info.mem_size;
    pOMemPoolInfo->block_count = obuf_info.block_count;

    ALOGD("%s: O_vaddr[%p], O_paddr[%p], O_size[%d], O_cnt[%d]",
            __func__,
            obuf_info.virt_addr,
            obuf_info.phy_addr,
            obuf_info.mem_size,
            obuf_info.block_count);

    return 0;
}
