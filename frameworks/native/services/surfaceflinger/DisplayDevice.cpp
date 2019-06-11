/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <cutils/properties.h>

#include <utils/RefBase.h>
#include <utils/Log.h>

#include <ui/DisplayInfo.h>
#include <ui/PixelFormat.h>

#include <gui/Surface.h>

#include <hardware/gralloc.h>

#include "DisplayHardware/DisplaySurface.h"
#include "DisplayHardware/HWComposer.h"
#include "RenderEngine/RenderEngine.h"

#include "clz.h"
#include "DisplayDevice.h"
#include "SurfaceFlinger.h"
#include "Layer.h"

#ifdef USES_VDS_EXTERNAL_UI_TRANSFORM
#include "ExynosHWCService.h"
#endif
// ----------------------------------------------------------------------------
using namespace android;
// ----------------------------------------------------------------------------

#ifdef EGL_ANDROID_swap_rectangle
static constexpr bool kEGLAndroidSwapRectangle = true;
#else
static constexpr bool kEGLAndroidSwapRectangle = false;
#endif

#if !defined(EGL_EGLEXT_PROTOTYPES) || !defined(EGL_ANDROID_swap_rectangle)
// Dummy implementation in case it is missing.
inline void eglSetSwapRectangleANDROID (EGLDisplay, EGLSurface, EGLint, EGLint, EGLint, EGLint) {
}
#endif

/*
 * Initialize the display to the specified values.
 *
 */

DisplayDevice::DisplayDevice(
        const sp<SurfaceFlinger>& flinger,
        DisplayType type,
        int32_t hwcId,
        int format,
        bool isSecure,
        const wp<IBinder>& displayToken,
        const sp<DisplaySurface>& displaySurface,
        const sp<IGraphicBufferProducer>& producer,
        EGLConfig config)
    : lastCompositionHadVisibleLayers(false),
      mFlinger(flinger),
      mType(type), mHwcDisplayId(hwcId),
      mDisplayToken(displayToken),
      mDisplaySurface(displaySurface),
      mDisplay(EGL_NO_DISPLAY),
      mSurface(EGL_NO_SURFACE),
#ifdef USES_EGL_SURFACE_FOR_COMPOSITION_MIXED
      mNativeWindowMixed(NULL),
      mDisplaySurfaceMixed(NULL),
      mConfigMixed(EGL_NO_CONFIG),
      mDisplayMixed(EGL_NO_DISPLAY),
      mSurfaceMixed(EGL_NO_SURFACE),
      mCompositionType((int)DisplaySurface::COMPOSITION_UNKNOWN),
#endif
      mDisplayWidth(), mDisplayHeight(), mFormat(),
      mFlags(),
      mPageFlipCount(),
      mIsSecure(isSecure),
      mSecureLayerVisible(false),
      mLayerStack(NO_LAYER_STACK),
      mOrientation(),
      mPowerMode(HWC_POWER_MODE_OFF),
      mActiveConfig(0)
#ifdef USES_VDS_EXTERNAL_UI_TRANSFORM
      ,mHwcService(NULL)
#endif
{
#ifdef USES_EGL_SURFACE_FOR_COMPOSITION_MIXED
    mProducer = producer;
#endif
    Surface* surface;
    mNativeWindow = surface = new Surface(producer, false);
    ANativeWindow* const window = mNativeWindow.get();

    /*
     * Create our display's surface
     */

    EGLSurface eglSurface;
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (config == EGL_NO_CONFIG) {
        config = RenderEngine::chooseEglConfig(display, format);
    }
#ifdef USES_VIRTUAL_DISPLAY_DECON_EXT_WB
    EGLint lAttributes[] = {
        0x304f /* EGL_SEC_DISABLE_TE */,
        EGL_FALSE,
        EGL_NONE
    };
    if (mType == DisplayDevice::DISPLAY_VIRTUAL) {
        lAttributes[1] = EGL_TRUE;
    }
    eglSurface = eglCreateWindowSurface(display, config, window, lAttributes);
#else
    eglSurface = eglCreateWindowSurface(display, config, window, NULL);
#endif
    eglQuerySurface(display, eglSurface, EGL_WIDTH,  &mDisplayWidth);
    eglQuerySurface(display, eglSurface, EGL_HEIGHT, &mDisplayHeight);

    // Make sure that composition can never be stalled by a virtual display
    // consumer that isn't processing buffers fast enough. We have to do this
    // in two places:
    // * Here, in case the display is composed entirely by HWC.
    // * In makeCurrent(), using eglSwapInterval. Some EGL drivers set the
    //   window's swap interval in eglMakeCurrent, so they'll override the
    //   interval we set here.
    if (mType >= DisplayDevice::DISPLAY_VIRTUAL)
        window->setSwapInterval(window, 0);

    mConfig = config;
    mDisplay = display;
    mSurface = eglSurface;
    mFormat  = format;
    mPageFlipCount = 0;
    mViewport.makeInvalid();
    mFrame.makeInvalid();

    // virtual displays are always considered enabled
    mPowerMode = (mType >= DisplayDevice::DISPLAY_VIRTUAL) ?
                  HWC_POWER_MODE_NORMAL : HWC_POWER_MODE_OFF;

    // Name the display.  The name will be replaced shortly if the display
    // was created with createDisplay().
    switch (mType) {
        case DISPLAY_PRIMARY:
            mDisplayName = "Built-in Screen";
            break;
        case DISPLAY_EXTERNAL:
            mDisplayName = "HDMI Screen";
            break;
        default:
            mDisplayName = "Virtual Screen";    // e.g. Overlay #n
            break;
    }

    // initialize the display orientation transform.
    setProjection(DisplayState::eOrientationDefault, mViewport, mFrame);

#ifdef NUM_FRAMEBUFFER_SURFACE_BUFFERS
    surface->allocateBuffers();
#endif
}

DisplayDevice::~DisplayDevice() {
    if (mSurface != EGL_NO_SURFACE) {
        eglDestroySurface(mDisplay, mSurface);
        mSurface = EGL_NO_SURFACE;
    }
#ifdef USES_EGL_SURFACE_FOR_COMPOSITION_MIXED
    if (mSurfaceMixed != EGL_NO_SURFACE) {
        eglDestroySurface(mDisplayMixed, mSurfaceMixed);
        mSurfaceMixed = EGL_NO_SURFACE;
    }
#endif
}

void DisplayDevice::disconnect(HWComposer& hwc) {
    if (mHwcDisplayId >= 0) {
        hwc.disconnectDisplay(mHwcDisplayId);
        if (mHwcDisplayId >= DISPLAY_VIRTUAL)
            hwc.freeDisplayId(mHwcDisplayId);
        mHwcDisplayId = -1;
    }
}

bool DisplayDevice::isValid() const {
    return mFlinger != NULL;
}

int DisplayDevice::getWidth() const {
    return mDisplayWidth;
}

int DisplayDevice::getHeight() const {
    return mDisplayHeight;
}

PixelFormat DisplayDevice::getFormat() const {
    return mFormat;
}

EGLSurface DisplayDevice::getEGLSurface() const {
    return mSurface;
}

void DisplayDevice::setDisplayName(const String8& displayName) {
    if (!displayName.isEmpty()) {
        // never override the name with an empty name
        mDisplayName = displayName;
    }
}

uint32_t DisplayDevice::getPageFlipCount() const {
    return mPageFlipCount;
}

status_t DisplayDevice::compositionComplete() const {
    return mDisplaySurface->compositionComplete();
}

void DisplayDevice::flip(const Region& dirty) const
{
    mFlinger->getRenderEngine().checkErrors();

#ifdef USES_EGL_SURFACE_FOR_COMPOSITION_MIXED
    EGLSurface surface = mSurface;
    if (mType >= DISPLAY_VIRTUAL && mCompositionType == (int)DisplaySurface::COMPOSITION_MIXED) {
        surface = mSurfaceMixed;
    } else {
        surface = mSurface;
    }
    if (mType >= DISPLAY_VIRTUAL)
        ALOGV("flip(), mSurfaceMixed %p, mSurface %p, surface %p", mSurfaceMixed, mSurface, surface);
#endif

    if (kEGLAndroidSwapRectangle) {
        if (mFlags & SWAP_RECTANGLE) {
            const Region newDirty(dirty.intersect(bounds()));
            const Rect b(newDirty.getBounds());
#ifdef USES_EGL_SURFACE_FOR_COMPOSITION_MIXED
            eglSetSwapRectangleANDROID(mDisplay, surface,
                    b.left, b.top, b.width(), b.height());
#else
            eglSetSwapRectangleANDROID(mDisplay, mSurface,
                    b.left, b.top, b.width(), b.height());
#endif
        }
    }

    mPageFlipCount++;
}

#ifdef USES_VDS_EXTERNAL_UI_TRANSFORM
status_t DisplayDevice::beginFrame(bool mustRecompose) {
    int halRotation, orientation;
    if (mType == DisplayDevice::DISPLAY_VIRTUAL) {
        if (mHwcService == NULL) {
            sp<IServiceManager> sm = defaultServiceManager();
            sp<IExynosHWCService> mHwcService =
                interface_cast<android::IExynosHWCService>(sm->getService(String16("Exynos.HWCService")));
        }
        if (mHwcService != NULL) {
                halRotation = mHwcService->getExternalUITransform();
                orientation = getTransformOrientation(halRotation);
                if (orientation != mOrientation)
                    setProjection(orientation, mViewport, mFrame);
        } else {
            ALOGE("Getting HWCService failed");
        }
    }
    return mDisplaySurface->beginFrame(mustRecompose);
}
#else
status_t DisplayDevice::beginFrame(bool mustRecompose) const {
    return mDisplaySurface->beginFrame(mustRecompose);
}
#endif

#ifdef USES_EGL_SURFACE_FOR_COMPOSITION_MIXED
void DisplayDevice::setEglConfigMixed(int format, EGLConfig config) {
    mNativeWindowMixed = new Surface(mProducer, false);
    ANativeWindow* const window = mNativeWindowMixed.get();
    EGLSurface surface;
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    if (config == EGL_NO_CONFIG) {
        config = RenderEngine::chooseEglConfig(display, format);
    }
#ifdef USES_VIRTUAL_DISPLAY_DECON_EXT_WB
    EGLint lAttributes[] = {
        0x304f /* EGL_SEC_DISABLE_TE */,
        EGL_FALSE,
        EGL_NONE
    };
    if (mType == DisplayDevice::DISPLAY_VIRTUAL) {
        lAttributes[1] = EGL_TRUE;
    }
    surface = eglCreateWindowSurface(display, config, window, lAttributes);
#else
    surface = eglCreateWindowSurface(display, config, window, NULL);
#endif

    // Make sure that composition can never be stalled by a virtual display
    // consumer that isn't processing buffers fast enough. We have to do this
    // in two places:
    // * Here, in case the display is composed entirely by HWC.
    // * In makeCurrent(), using eglSwapInterval. Some EGL drivers set the
    //   window's swap interval in eglMakeCurrent, so they'll override the
    //   interval we set here.
    if (mType >= DisplayDevice::DISPLAY_VIRTUAL)
        window->setSwapInterval(window, 0);

    mConfigMixed = config;
    mDisplayMixed = display;
    mSurfaceMixed = surface;
}

void DisplayDevice::setCompositionType(HWComposer& hwc) {
    DisplaySurface::CompositionType compositionType;
    bool haveGles = hwc.hasGlesComposition(mHwcDisplayId);
    bool haveHwc = hwc.hasHwcComposition(mHwcDisplayId);
    if (haveGles && haveHwc) {
        compositionType = DisplaySurface::COMPOSITION_MIXED;
    } else if (haveGles) {
        compositionType = DisplaySurface::COMPOSITION_GLES;
    } else if (haveHwc) {
        compositionType = DisplaySurface::COMPOSITION_HWC;
    } else {
        // Nothing to do -- when turning the screen off we get a frame like
        // this. Call it a HWC frame since we won't be doing any GLES work but
        // will do a prepare/set cycle.
        compositionType = DisplaySurface::COMPOSITION_HWC;
    }

    mCompositionType = compositionType;
}
#endif

status_t DisplayDevice::prepareFrame(const HWComposer& hwc) const {

    DisplaySurface::CompositionType compositionType;
    bool haveGles = hwc.hasGlesComposition(mHwcDisplayId);
    bool haveHwc = hwc.hasHwcComposition(mHwcDisplayId);
    if (haveGles && haveHwc) {
        compositionType = DisplaySurface::COMPOSITION_MIXED;
    } else if (haveGles) {
        compositionType = DisplaySurface::COMPOSITION_GLES;
    } else if (haveHwc) {
        compositionType = DisplaySurface::COMPOSITION_HWC;
    } else {
        // Nothing to do -- when turning the screen off we get a frame like
        // this. Call it a HWC frame since we won't be doing any GLES work but
        // will do a prepare/set cycle.
        compositionType = DisplaySurface::COMPOSITION_HWC;
    }
    return mDisplaySurface->prepareFrame(compositionType);
}

void DisplayDevice::swapBuffers(HWComposer& hwc) const {
    // We need to call eglSwapBuffers() if:
    //  (1) we don't have a hardware composer, or
    //  (2) we did GLES composition this frame, and either
    //    (a) we have framebuffer target support (not present on legacy
    //        devices, where HWComposer::commit() handles things); or
    //    (b) this is a virtual display
    if (hwc.initCheck() != NO_ERROR ||
            (hwc.hasGlesComposition(mHwcDisplayId) &&
             (hwc.supportsFramebufferTarget() || mType >= DISPLAY_VIRTUAL))) {
#ifdef USES_EGL_SURFACE_FOR_COMPOSITION_MIXED
        EGLBoolean success;
        EGLDisplay tempDisplay;
        EGLSurface tempSurface;
        if (mType >= DISPLAY_VIRTUAL && mCompositionType == (int)DisplaySurface::COMPOSITION_MIXED) {
            tempDisplay = mDisplayMixed;
            tempSurface = mSurfaceMixed;
        } else {
            tempDisplay = mDisplay;
            tempSurface = mSurface;
        }
        if (mType >= DISPLAY_VIRTUAL)
            ALOGV("swapBuffers(), mSurfaceMixed %p, mSurface %p, tempSurface %p",
                mSurfaceMixed, mSurface, tempSurface);
        success = eglSwapBuffers(tempDisplay, tempSurface);
        if (!success) {
            EGLint error = eglGetError();
            if (error == EGL_CONTEXT_LOST ||
                    mType == DisplayDevice::DISPLAY_PRIMARY) {
                LOG_ALWAYS_FATAL("eglSwapBuffers(%p, %p) failed with 0x%08x",
                        tempDisplay, tempSurface, error);
            } else {
                ALOGE("eglSwapBuffers(%p, %p) failed with 0x%08x",
                        tempDisplay, tempSurface, error);
            }
        }
#else
        EGLBoolean success = eglSwapBuffers(mDisplay, mSurface);
        if (!success) {
            EGLint error = eglGetError();
            if (error == EGL_CONTEXT_LOST ||
                    mType == DisplayDevice::DISPLAY_PRIMARY) {
                LOG_ALWAYS_FATAL("eglSwapBuffers(%p, %p) failed with 0x%08x",
                        mDisplay, mSurface, error);
            } else {
                ALOGE("eglSwapBuffers(%p, %p) failed with 0x%08x",
                        mDisplay, mSurface, error);
            }
        }
#endif
    }

    status_t result = mDisplaySurface->advanceFrame();
    if (result != NO_ERROR) {
        ALOGE("[%s] failed pushing new frame to HWC: %d",
                mDisplayName.string(), result);
    }
}

void DisplayDevice::onSwapBuffersCompleted(HWComposer& hwc) const {
    if (hwc.initCheck() == NO_ERROR) {
        mDisplaySurface->onFrameCommitted();
    }
}

uint32_t DisplayDevice::getFlags() const
{
    return mFlags;
}

EGLBoolean DisplayDevice::makeCurrent(EGLDisplay dpy, EGLContext ctx) const {
    EGLBoolean result = EGL_TRUE;
#ifdef USES_EGL_SURFACE_FOR_COMPOSITION_MIXED
    EGLSurface sur = eglGetCurrentSurface(EGL_DRAW);
    EGLSurface tempSurface;
    if (mType >= DISPLAY_VIRTUAL && mCompositionType == (int)DisplaySurface::COMPOSITION_MIXED) {
        tempSurface = mSurfaceMixed;
    } else {
        tempSurface = mSurface;
    }
    if (mType >= DISPLAY_VIRTUAL)
        ALOGV("makeCurrent(), mSurfaceMixed %p, mSurface %p, tempSurface %p", mSurfaceMixed, mSurface, tempSurface);

    if (sur != tempSurface) {
        result = eglMakeCurrent(dpy, tempSurface, tempSurface, ctx);
        if (result == EGL_TRUE) {
            if (mType >= DisplayDevice::DISPLAY_VIRTUAL)
                eglSwapInterval(dpy, 0);
        }
    }
#else
    EGLSurface sur = eglGetCurrentSurface(EGL_DRAW);
    if (sur != mSurface) {
        result = eglMakeCurrent(dpy, mSurface, mSurface, ctx);
        if (result == EGL_TRUE) {
            if (mType >= DisplayDevice::DISPLAY_VIRTUAL)
                eglSwapInterval(dpy, 0);
        }
    }
#endif
    setViewportAndProjection();
    return result;
}

void DisplayDevice::setViewportAndProjection() const {
    size_t w = mDisplayWidth;
    size_t h = mDisplayHeight;
    Rect sourceCrop(0, 0, w, h);
    mFlinger->getRenderEngine().setViewportAndProjection(w, h, sourceCrop, h,
        false, Transform::ROT_0);
}

// ----------------------------------------------------------------------------

void DisplayDevice::setVisibleLayersSortedByZ(const Vector< sp<Layer> >& layers) {
    mVisibleLayersSortedByZ = layers;
    mSecureLayerVisible = false;
    size_t count = layers.size();
    for (size_t i=0 ; i<count ; i++) {
        const sp<Layer>& layer(layers[i]);
        if (layer->isSecure()) {
            mSecureLayerVisible = true;
        }
    }
}

const Vector< sp<Layer> >& DisplayDevice::getVisibleLayersSortedByZ() const {
    return mVisibleLayersSortedByZ;
}

bool DisplayDevice::getSecureLayerVisible() const {
    return mSecureLayerVisible;
}

Region DisplayDevice::getDirtyRegion(bool repaintEverything) const {
    Region dirty;
    if (repaintEverything) {
        dirty.set(getBounds());
    } else {
        const Transform& planeTransform(mGlobalTransform);
        dirty = planeTransform.transform(this->dirtyRegion);
        dirty.andSelf(getBounds());
    }
    return dirty;
}

// ----------------------------------------------------------------------------
void DisplayDevice::setPowerMode(int mode) {
    mPowerMode = mode;
}

int DisplayDevice::getPowerMode()  const {
    return mPowerMode;
}

bool DisplayDevice::isDisplayOn() const {
    return (mPowerMode != HWC_POWER_MODE_OFF);
}

// ----------------------------------------------------------------------------
void DisplayDevice::setActiveConfig(int mode) {
    mActiveConfig = mode;
}

int DisplayDevice::getActiveConfig()  const {
    return mActiveConfig;
}

// ----------------------------------------------------------------------------

void DisplayDevice::setLayerStack(uint32_t stack) {
    mLayerStack = stack;
    dirtyRegion.set(bounds());
}

// ----------------------------------------------------------------------------

uint32_t DisplayDevice::getOrientationTransform() const {
    uint32_t transform = 0;
    switch (mOrientation) {
        case DisplayState::eOrientationDefault:
            transform = Transform::ROT_0;
            break;
        case DisplayState::eOrientation90:
            transform = Transform::ROT_90;
            break;
        case DisplayState::eOrientation180:
            transform = Transform::ROT_180;
            break;
        case DisplayState::eOrientation270:
            transform = Transform::ROT_270;
            break;
    }
    return transform;
}

#ifdef USES_VDS_EXTERNAL_UI_TRANSFORM
uint32_t DisplayDevice::getTransformOrientation(uint32_t transform) const {
    uint32_t orientation = 0;
    switch (transform) {
        case Transform::ROT_0:
            orientation = DisplayState::eOrientationDefault;
            break;
        case Transform::ROT_90:
            orientation = DisplayState::eOrientation90;
            break;
        case Transform::ROT_180:
            orientation = DisplayState::eOrientation180;
            break;
        case Transform::ROT_270:
            orientation = DisplayState::eOrientation270;
            break;
    }
    return orientation;
}
#endif

status_t DisplayDevice::orientationToTransfrom(
        int orientation, int w, int h, Transform* tr)
{
    uint32_t flags = 0;
    switch (orientation) {
    case DisplayState::eOrientationDefault:
        flags = Transform::ROT_0;
        break;
    case DisplayState::eOrientation90:
        flags = Transform::ROT_90;
        break;
    case DisplayState::eOrientation180:
        flags = Transform::ROT_180;
        break;
    case DisplayState::eOrientation270:
        flags = Transform::ROT_270;
        break;
    default:
        return BAD_VALUE;
    }
    tr->set(flags, w, h);
    return NO_ERROR;
}

void DisplayDevice::setDisplaySize(const int newWidth, const int newHeight) {
    dirtyRegion.set(getBounds());

    if (mSurface != EGL_NO_SURFACE) {
        eglDestroySurface(mDisplay, mSurface);
        mSurface = EGL_NO_SURFACE;
    }

    mDisplaySurface->resizeBuffers(newWidth, newHeight);

    ANativeWindow* const window = mNativeWindow.get();
    mSurface = eglCreateWindowSurface(mDisplay, mConfig, window, NULL);
    eglQuerySurface(mDisplay, mSurface, EGL_WIDTH,  &mDisplayWidth);
    eglQuerySurface(mDisplay, mSurface, EGL_HEIGHT, &mDisplayHeight);

    LOG_FATAL_IF(mDisplayWidth != newWidth,
                "Unable to set new width to %d", newWidth);
    LOG_FATAL_IF(mDisplayHeight != newHeight,
                "Unable to set new height to %d", newHeight);
}

#ifdef USES_VDS_EXTERNAL_UI_TRANSFORM
void DisplayDevice::setProjection(int orientation) {
    setProjection(orientation, mViewport, mFrame);
}
#endif

void DisplayDevice::setProjection(int orientation,
        const Rect& newViewport, const Rect& newFrame) {
    Rect viewport(newViewport);
    Rect frame(newFrame);

    const int w = mDisplayWidth;
    const int h = mDisplayHeight;

    Transform R;
    DisplayDevice::orientationToTransfrom(orientation, w, h, &R);

    if (!frame.isValid()) {
        // the destination frame can be invalid if it has never been set,
        // in that case we assume the whole display frame.
        frame = Rect(w, h);
    }

    if (viewport.isEmpty()) {
        // viewport can be invalid if it has never been set, in that case
        // we assume the whole display size.
        // it's also invalid to have an empty viewport, so we handle that
        // case in the same way.
        viewport = Rect(w, h);
        if (R.getOrientation() & Transform::ROT_90) {
            // viewport is always specified in the logical orientation
            // of the display (ie: post-rotation).
            swap(viewport.right, viewport.bottom);
        }
    }

    dirtyRegion.set(getBounds());

    Transform TL, TP, S;
    float src_width  = viewport.width();
    float src_height = viewport.height();
    float dst_width  = frame.width();
    float dst_height = frame.height();
#ifdef USES_VDS_EXTERNAL_UI_TRANSFORM
    if (mType == DisplayDevice::DISPLAY_VIRTUAL) {
        if (orientation == DisplayState::eOrientation90 || orientation == DisplayState::eOrientation270) {
            dst_width  = mDisplayHeight;
            dst_height = mDisplayWidth;
        } else {
            dst_width  = mDisplayWidth;
            dst_height = mDisplayHeight;
        }
        /* adjust src width/height ratio to dst */
        if (src_width / src_height > dst_width / dst_height)
            dst_height = (src_height * dst_width) / src_width;
        else
            dst_width = (src_width * dst_height) / src_height;
    }
#endif

    if (src_width != dst_width || src_height != dst_height) {
        float sx = dst_width  / src_width;
        float sy = dst_height / src_height;
        S.set(sx, 0, 0, sy);
    }

    float src_x = viewport.left;
    float src_y = viewport.top;
    float dst_x = frame.left;
    float dst_y = frame.top;
#ifdef USES_VDS_EXTERNAL_UI_TRANSFORM
    if (mType == DisplayDevice::DISPLAY_VIRTUAL) {
        if (orientation == DisplayState::eOrientation90 || orientation == DisplayState::eOrientation270) {
            dst_x = (mDisplayHeight - dst_width) / 2;
            dst_y = (mDisplayWidth - dst_height) / 2;
        } else {
            dst_x = (mDisplayWidth - dst_width) / 2;
            dst_y = (mDisplayHeight - dst_height) / 2;
        }
    }
#endif

    TL.set(-src_x, -src_y);
    TP.set(dst_x, dst_y);

    // The viewport and frame are both in the logical orientation.
    // Apply the logical translation, scale to physical size, apply the
    // physical translation and finally rotate to the physical orientation.
    mGlobalTransform = R * TP * S * TL;

    const uint8_t type = mGlobalTransform.getType();
    mNeedsFiltering = (!mGlobalTransform.preserveRects() ||
            (type >= Transform::SCALE));

    mScissor = mGlobalTransform.transform(viewport);
    if (mScissor.isEmpty()) {
        mScissor = getBounds();
    }

    mOrientation = orientation;
    mViewport = viewport;
    mFrame = frame;
}

void DisplayDevice::dump(String8& result) const {
    const Transform& tr(mGlobalTransform);
    result.appendFormat(
        "+ DisplayDevice: %s\n"
        "   type=%x, hwcId=%d, layerStack=%u, (%4dx%4d), ANativeWindow=%p, orient=%2d (type=%08x), "
        "flips=%u, isSecure=%d, secureVis=%d, powerMode=%d, activeConfig=%d, numLayers=%zu\n"
        "   v:[%d,%d,%d,%d], f:[%d,%d,%d,%d], s:[%d,%d,%d,%d],"
        "transform:[[%0.3f,%0.3f,%0.3f][%0.3f,%0.3f,%0.3f][%0.3f,%0.3f,%0.3f]]\n",
        mDisplayName.string(), mType, mHwcDisplayId,
        mLayerStack, mDisplayWidth, mDisplayHeight, mNativeWindow.get(),
        mOrientation, tr.getType(), getPageFlipCount(),
        mIsSecure, mSecureLayerVisible, mPowerMode, mActiveConfig,
        mVisibleLayersSortedByZ.size(),
        mViewport.left, mViewport.top, mViewport.right, mViewport.bottom,
        mFrame.left, mFrame.top, mFrame.right, mFrame.bottom,
        mScissor.left, mScissor.top, mScissor.right, mScissor.bottom,
        tr[0][0], tr[1][0], tr[2][0],
        tr[0][1], tr[1][1], tr[2][1],
        tr[0][2], tr[1][2], tr[2][2]);

    String8 surfaceDump;
    mDisplaySurface->dumpAsString(surfaceDump);
    result.append(surfaceDump);
}
