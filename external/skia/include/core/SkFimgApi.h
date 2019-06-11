/*
 * Copyright 2012, Samsung Electronics Co. LTD
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
#ifndef SkFimgApi_DEFINED
#define SkFimgApi_DEFINED

#include "SkColorPriv.h"
#include "SkBitmap.h"
#include "SkMallocPixelRef.h"
#include "SkFlattenable.h"
#include "SkUtils.h"
#include "SkXfermode.h"
#include "SkMatrix.h"
#include "SkBitmap.h"
#include "SkMask.h"

#include "FimgApi.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>

#ifdef FIMG2D_BOOSTUP
#include "fimg2d_board.h"
#endif

//---------------------------------------------------------------------------//

#define FIMGAPI_COMPROMISE_USE true

#if defined(FIMGAPI_V5X)
#define FIMGAPI_MAXSIZE 16383
#else
#define FIMGAPI_MAXSIZE 8000
#endif
#define FIMGAPI_FINISHED       (0x1<<0)
#undef FIMGAPI_DEBUG_MESSAGE

bool FimgApiCheckPossible(Fimg *fimg);
bool FimgApiIsDstMode(Fimg *fimg);
bool FimgApiClipping(Fimg *fimg);
bool FimgApiCompromise(Fimg *fimg);
bool FimgSupportNegativeCoordinate(Fimg *fimg);
int FimgApiStretch(Fimg *fimg, const char *func_name);
int FimgARGB32_Rect(uint32_t *device,  int x,  int y,  int width,  int height,
                    size_t rowbyte,  uint32_t color);
int FimgRGB16_Rect(uint32_t *device,  int x,  int y,  int width,  int height,
                    size_t rowbyte,  uint32_t color);
uint32_t toARGB32(uint32_t color);
#endif //SkFimgApi_DEFINED
