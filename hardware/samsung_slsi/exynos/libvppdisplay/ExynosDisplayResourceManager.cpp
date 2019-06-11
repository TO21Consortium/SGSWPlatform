#define LOG_TAG "DisplayResourceManager"
//#define LOG_NDEBUG 0

#include "ExynosDisplayResourceManager.h"
#include "ExynosMPPModule.h"
#include "ExynosPrimaryDisplay.h"
#include "ExynosExternalDisplay.h"
#if defined(USES_DUAL_DISPLAY)
#include "ExynosSecondaryDisplayModule.h"
#endif
#ifdef USES_VIRTUAL_DISPLAY
#include "ExynosVirtualDisplay.h"
#endif
#include "ExynosHWCDebug.h"

ExynosMPPVector::ExynosMPPVector() {
}

ExynosMPPVector::ExynosMPPVector(const ExynosMPPVector& rhs)
    : android::SortedVector<ExynosMPPModule* >(rhs) {
}

int ExynosMPPVector::do_compare(const void* lhs,
        const void* rhs) const
{
    if (lhs == NULL || rhs == NULL)
        return 0;

    const ExynosMPPModule* l = *((ExynosMPPModule**)(lhs));
    const ExynosMPPModule* r = *((ExynosMPPModule**)(rhs));
    uint8_t mppNum = sizeof(VPP_ASSIGN_ORDER)/sizeof(VPP_ASSIGN_ORDER[0]);

    if (l == NULL || r == NULL)
        return 0;

    if (l->mType != r->mType) {
        uint8_t lhsOrder = 0;
        uint8_t rhsOrder = 0;

        for (uint8_t i = 0; i < mppNum; i++) {
            if (l->mType == VPP_ASSIGN_ORDER[i]) {
                lhsOrder = i;
                break;
            }
        }
        for (uint8_t i = 0; i < mppNum; i++) {
            if (r->mType == VPP_ASSIGN_ORDER[i]) {
                rhsOrder = i;
                break;
            }
        }
        return lhsOrder - rhsOrder;
    }

    return l->mIndex - r->mIndex;
}

ExynosDisplayResourceManager::ExynosDisplayResourceManager(struct exynos5_hwc_composer_device_1_t *pdev)
    : mHwc(pdev),
      mNeedsReserveFbTargetPrimary(false)
{
#ifdef DISABLE_IDMA_SECURE
    mHwc->primaryDisplay->mUseSecureDMA = false;
#if defined(USES_DUAL_DISPLAY)
    mHwc->secondaryDisplay->mUseSecureDMA = false;
#endif
#else
#if defined(USES_DUAL_DISPLAY)
    mHwc->primaryDisplay->mUseSecureDMA = false;
    mHwc->secondaryDisplay->mUseSecureDMA = true;
#else
    mHwc->primaryDisplay->mUseSecureDMA = true;
#endif
#endif
    mHwc->externalDisplay->mUseSecureDMA = false;
#ifdef USES_VIRTUAL_DISPLAY
    mHwc->virtualDisplay->mUseSecureDMA = false;
#endif
    ExynosMPP::mainDisplayWidth = mHwc->primaryDisplay->mXres;
    if (ExynosMPP::mainDisplayWidth <= 0) {
        ExynosMPP::mainDisplayWidth = 1440;
    }
}

ExynosDisplayResourceManager::~ExynosDisplayResourceManager()
{
    if (!mInternalMPPs.isEmpty()) {
        for (size_t i = 0; i < mInternalMPPs.size(); i++) {
            delete mInternalMPPs[i];
        }
        mInternalMPPs.clear();
    }
    if (!mExternalMPPs.isEmpty()) {
        for (size_t i = 0; i < mExternalMPPs.size(); i++) {
            delete mExternalMPPs[i];
        }
        mExternalMPPs.clear();
    }
}

void ExynosDisplayResourceManager::removeUnAssignedIntMpp(ExynosMPPVector &internalMPPs)
{
    for (size_t i = internalMPPs.size(); i-- > 0;) {
        ExynosMPPModule* exynosMPP = (ExynosMPPModule *)internalMPPs[i];
        if (exynosMPP->mState == MPP_STATE_FREE || exynosMPP->mState == MPP_STATE_TRANSITION)
            internalMPPs.removeItemsAt(i);
    }
}

void ExynosDisplayResourceManager::addExternalMpp(hwc_display_contents_1_t** contents)
{
#if defined(USES_DUAL_DISPLAY)
    hwc_display_contents_1_t *fimd_contents = contents[HWC_DISPLAY_PRIMARY0];
    hwc_display_contents_1_t *fimd_contents1 = contents[HWC_DISPLAY_PRIMARY1];
#else
    hwc_display_contents_1_t *fimd_contents = contents[HWC_DISPLAY_PRIMARY];
#endif
    hwc_display_contents_1_t *hdmi_contents = contents[HWC_DISPLAY_EXTERNAL];
#ifdef USES_VIRTUAL_DISPLAY
    hwc_display_contents_1_t *virtual_contents = contents[HWC_DISPLAY_VIRTUAL];
#endif

    hwc_display_contents_1_t *secondary_contents = hdmi_contents;
    ExynosDisplay* secondary_display = mHwc->externalDisplay;

    if (mExternalMPPs.size() == 0)
        return;

    ExynosMPPModule* exynosMPP = (ExynosMPPModule *)mExternalMPPs[FIMD_EXT_MPP_IDX];
    if (fimd_contents &&
        ((mHwc->primaryDisplay->mHasDrmSurface) ||
         (mHwc->primaryDisplay->mYuvLayers > 0) ||
         ((fimd_contents->flags & HWC_GEOMETRY_CHANGED) == 0))) {
        mHwc->primaryDisplay->mExternalMPPs.add(exynosMPP);
    }

#if defined(USES_DUAL_DISPLAY)
    if (mHwc->hdmi_hpd == false) {
        secondary_contents = fimd_contents1;
        secondary_display = mHwc->secondaryDisplay;
    }
#endif

    if (secondary_contents) {
        exynosMPP = (ExynosMPPModule *)mExternalMPPs[HDMI_EXT_MPP_IDX];
        secondary_display->mExternalMPPs.add(exynosMPP);
    }

#ifdef USES_VIRTUAL_DISPLAY
    if (virtual_contents) {
        exynosMPP = (ExynosMPPModule *)mExternalMPPs[WFD_EXT_MPP_IDX];
        mHwc->virtualDisplay->mExternalMPPs.add(exynosMPP);
#ifdef USES_2MSC_FOR_WFD
        /* 1st is for blending and 2nd is for scaling */
        exynosMPP = (ExynosMPPModule *)mExternalMPPs[WFD_EXT_MPP_IDX + 1];
        mHwc->virtualDisplay->mExternalMPPs.add(exynosMPP);
#endif
#ifdef USES_3MSC_FOR_WFD
        /* To prevent lack of MSC, WFD use 3 external MPPs */
        exynosMPP = (ExynosMPPModule *)mExternalMPPs[WFD_EXT_MPP_IDX + 1];
        mHwc->virtualDisplay->mExternalMPPs.add(exynosMPP);

        exynosMPP = (ExynosMPPModule *)mExternalMPPs[WFD_EXT_MPP_IDX + 2];
        mHwc->virtualDisplay->mExternalMPPs.add(exynosMPP);
#endif
    }
#endif
}

void ExynosDisplayResourceManager::addUnAssignedIntMpp(ExynosDisplay *display)
{
    for (size_t i = 0; i < mInternalMPPs.size(); i++) {
        ExynosMPPModule* exynosMPP = (ExynosMPPModule *)mInternalMPPs[i];
        if ((exynosMPP->mState == MPP_STATE_FREE) && (exynosMPP->mCanBeUsed) && (exynosMPP->isAssignable(display)))
            display->mInternalMPPs.add(exynosMPP);
    }
}

void ExynosDisplayResourceManager::cleanupMPPs()
{
    for (size_t i = 0; i < mExternalMPPs.size(); i++)
    {
        mExternalMPPs[i]->preAssignDisplay(NULL);
        if (mExternalMPPs[i]->mState == MPP_STATE_FREE)
            mExternalMPPs[i]->cleanupM2M();
    }
    for (size_t i = 0; i < mInternalMPPs.size(); i++)
    {
        mInternalMPPs[i]->preAssignDisplay(NULL);
        if (mInternalMPPs[i]->mState == MPP_STATE_FREE)
            mInternalMPPs[i]->cleanupInternalMPP();
    }
}

void ExynosDisplayResourceManager::dumpMPPs(android::String8& result)
{
    result.appendFormat("ExynosDisplayResourceManager Internal MPPs number: %zu\n", mInternalMPPs.size());
    result.append(
            " mType | mIndex | mState | mCanBeUsed | mDisplay \n"
            "-------+--------+--------------------------------\n");
        //    5____ | 6_____ | 6_____ | 10________ | 8_______ \n
    for (size_t i = 0; i < mInternalMPPs.size(); i++)
    {
        ExynosMPPModule* internalMPP = mInternalMPPs[i];
        result.appendFormat(" %5d | %6d | %6d | %10d | %8d\n",
                internalMPP->mType, internalMPP->mIndex,
                internalMPP->mState, internalMPP->mCanBeUsed, (internalMPP->mDisplay == NULL)? -1: internalMPP->mDisplay->mType);
    }

    result.appendFormat("ExynosDisplayResourceManager External MPPs number: %zu\n", mExternalMPPs.size());
    result.append(
            " mType | mIndex | mState | mCanBeUsed \n"
            "-------+--------+----------------------\n");
        //    5____ | 6_____ | 6_____ | 10________\n

    for (size_t i = 0; i < mExternalMPPs.size(); i++)
    {
        ExynosMPPModule* externalMPP = mExternalMPPs[i];
        result.appendFormat(" %5d | %6d | %6d | %10d\n",
                externalMPP->mType, externalMPP->mIndex,
                externalMPP->mState, externalMPP->mCanBeUsed);
    }
}

void ExynosDisplayResourceManager::printDisplyInfos(size_t type)
{
    if (hwcCheckDebugMessages(eDebugResourceManager) == false)
        return;

    android::String8 result;

    dumpMPPs(result);
    HDEBUGLOGD(eDebugResourceManager, "%s", result.string());
    if (type == EXYNOS_PRIMARY_DISPLAY) {
        HDEBUGLOGD(eDebugResourceManager, "Primary display");
        result.clear();
        mHwc->primaryDisplay->dumpLayerInfo(result);
        HDEBUGLOGD(eDebugResourceManager, "%s", result.string());

        result.clear();
        mHwc->primaryDisplay->dumpMPPs(result);
        HDEBUGLOGD(eDebugResourceManager, "%s", result.string());
#if defined(USES_DUAL_DISPLAY)
    } else if (type == EXYNOS_SECONDARY_DISPLAY) {
        HDEBUGLOGD(eDebugResourceManager, "Secondary display");
        result.clear();
        mHwc->secondaryDisplay->dumpLayerInfo(result);
        HDEBUGLOGD(eDebugResourceManager, "%s", result.string());

        result.clear();
        mHwc->secondaryDisplay->dumpMPPs(result);
        HDEBUGLOGD(eDebugResourceManager, "%s", result.string());
#endif
    } else if (type == EXYNOS_EXTERNAL_DISPLAY) {
        HDEBUGLOGD(eDebugResourceManager, "External display");
        result.clear();
        mHwc->externalDisplay->dumpLayerInfo(result);
        HDEBUGLOGD(eDebugResourceManager, "%s", result.string());

        result.clear();
        mHwc->externalDisplay->dumpMPPs(result);
        HDEBUGLOGD(eDebugResourceManager, "%s", result.string());
    }
#ifdef USES_VIRTUAL_DISPLAY
    else if (type == EXYNOS_VIRTUAL_DISPLAY) {
        HDEBUGLOGD(eDebugResourceManager, "Virtual display");
        result.clear();
        mHwc->virtualDisplay->dumpLayerInfo(result);
        HDEBUGLOGD(eDebugResourceManager, "%s", result.string());

        result.clear();
        mHwc->virtualDisplay->dumpMPPs(result);
        HDEBUGLOGD(eDebugResourceManager, "%s", result.string());
    }
#endif
}

void ExynosDisplayResourceManager::doPreProcessing(hwc_display_contents_1_t* contents, ExynosDisplay* display,
        int *previous_drm_dma, ExynosMPPModule **previousDRMInternalMPP)
{
    if ((contents == NULL) || (display == NULL))
        return;

    android::Mutex::Autolock lock(display->mLayerInfoMutex);
    /*
     * Allocate LayerInfos of each DisplayDevice
     * because assignResources will set data of LayerInfos
     */
    display->allocateLayerInfos(contents);
    display->mInternalMPPs.clear();
    display->mExternalMPPs.clear();

    if (display->getPreviousDRMDMA(previous_drm_dma)) {
        if (getInternalMPPFromDMA((unsigned int)*previous_drm_dma, previousDRMInternalMPP) < 0) {
            *previous_drm_dma = -1;
        }
    }
}

void ExynosDisplayResourceManager::handleHighPriorityLayers(hwc_display_contents_1_t* contents, ExynosDisplay* display,
        int previous_drm_dma, ExynosMPPModule *previousDRMInternalMPP, bool reserveFbTarget)
{
    if ((contents == NULL) || (display == NULL))
        return;

    android::Mutex::Autolock lock(display->mLayerInfoMutex);

    /* Don't use the VPP that was used for DRM at the previous frame */
    if ((display->mPreProcessedInfo.mHasDrmSurface == false) &&
        (previous_drm_dma > 0) && (previousDRMInternalMPP != NULL)) {
        previousDRMInternalMPP->mCanBeUsed = false;
    }

    /*
     * Assign all of MPPs to the diplay for Video
     */
    addUnAssignedIntMpp(display);
    display->preAssignFbTarget(contents, reserveFbTarget);
    display->determineYuvOverlay(contents);

    // Remove all MPPs that were not assigned
    removeUnAssignedIntMpp(display->mInternalMPPs);

    /* Check whether VPP for DRM is changed */
    if ((previous_drm_dma > 0) &&
        (mHwc->primaryDisplay->mHasDrmSurface == true) && (display->mForceOverlayLayerIndex >= 0) &&
        (previousDRMInternalMPP != NULL)) {
        if (previousDRMInternalMPP != display->mLayerInfos[display->mForceOverlayLayerIndex]->mInternalMPP)
            previousDRMInternalMPP->mCanBeUsed = false;
    }

    HDEBUGLOGD(eDebugResourceManager, "Display(%d):: after determineYuvOverlay", display->mType);
    printDisplyInfos(display->mType);
}

void ExynosDisplayResourceManager::handleLowPriorityLayers(hwc_display_contents_1_t* contents, ExynosDisplay* display)
{
    if ((contents == NULL) || (display == NULL))
        return;

    android::Mutex::Autolock lock(display->mLayerInfoMutex);

    /*
     * Assign the rest MPPs to the display for UI
     */
    addUnAssignedIntMpp(display);
    display->determineSupportedOverlays(contents);
    HDEBUGLOGD(eDebugResourceManager, "Display(%d):: after determineSupportedOverlays", display->mType);
    printDisplyInfos(display->mType);
    display->mAllowedOverlays = display->mInternalMPPs.size() + display->mInternalDMAs.size();
    display->determineBandwidthSupport(contents);

    HDEBUGLOGD(eDebugResourceManager, "Display(%d):: after determineBandwidthSupport", display->mType);
    printDisplyInfos(display->mType);
    display->assignWindows(contents);

    // Remove all MPPs that were not assigned
    removeUnAssignedIntMpp(display->mInternalMPPs);

    HDEBUGLOGD(eDebugResourceManager, "Display(%d):: after assignWindows", display->mType);
    printDisplyInfos(display->mType);
}

int ExynosDisplayResourceManager::assignResources(size_t numDisplays, hwc_display_contents_1_t** displays)
{
    mNeedsReserveFbTargetPrimary = false;
    if (!numDisplays || !displays)
        return 0;

#if defined(USES_DUAL_DISPLAY)
    hwc_display_contents_1_t *fimd_contents = displays[HWC_DISPLAY_PRIMARY0];
    hwc_display_contents_1_t *fimd_contents1 = displays[HWC_DISPLAY_PRIMARY1];
#else
    hwc_display_contents_1_t *fimd_contents = displays[HWC_DISPLAY_PRIMARY];
#endif
    hwc_display_contents_1_t *hdmi_contents = displays[HWC_DISPLAY_EXTERNAL];
#ifdef USES_VIRTUAL_DISPLAY
    hwc_display_contents_1_t *virtual_contents = displays[HWC_DISPLAY_VIRTUAL];
#endif

    hwc_display_contents_1_t *secondary_contents = hdmi_contents;
    ExynosDisplay* secondary_display = mHwc->externalDisplay;

    int primary_previous_drm_dma = -1;
    int secondary_previous_drm_dma = -1;
    int virtual_previous_drm_dma = -1;
    ExynosMPPModule *previousDRMInternalMPPPrimary = NULL;
    ExynosMPPModule *previousDRMInternalMPPSecondary = NULL;
    ExynosMPPModule *previousDRMInternalMPPVirtual = NULL;

#if defined(USES_DUAL_DISPLAY)
    if (mHwc->hdmi_hpd == false) {
        secondary_contents = fimd_contents1;
        secondary_display = mHwc->secondaryDisplay;
    }
#endif

    if ((mHwc->hdmi_hpd == false) && (mHwc->externalDisplay->isIONBufferAllocated())) {
        bool noExtVideoBuffer = true;
        for (size_t i = 0; i < mExternalMPPs.size(); i++) {
            if (!mExternalMPPs[i]->checkNoExtVideoBuffer())
                noExtVideoBuffer = false;
        }

        if (noExtVideoBuffer)
            mHwc->externalDisplay->freeExtVideoBuffers();
    }

    /* Clear assigned flag */
    for (size_t i = 0; i < mInternalMPPs.size(); i++) {
        if (mInternalMPPs[i]->mState != MPP_STATE_TRANSITION)
            mInternalMPPs[i]->mState = MPP_STATE_FREE;
        mInternalMPPs[i]->mCanBeUsed = true;
    }
    for (size_t i = 0; i < mExternalMPPs.size(); i++) {
        if (mExternalMPPs[i]->mState != MPP_STATE_TRANSITION)
            mExternalMPPs[i]->mState = MPP_STATE_FREE;
    }

    if (fimd_contents) {
        doPreProcessing(fimd_contents, mHwc->primaryDisplay, &primary_previous_drm_dma, &previousDRMInternalMPPPrimary);
    }
    if (secondary_contents) {
        doPreProcessing(secondary_contents, secondary_display, &secondary_previous_drm_dma, &previousDRMInternalMPPSecondary);
    }
#ifdef USES_VIRTUAL_DISPLAY
    if (virtual_contents) {
        doPreProcessing(virtual_contents, mHwc->virtualDisplay, &virtual_previous_drm_dma, &previousDRMInternalMPPVirtual);
    }
#endif

    preAssignResource();
    addExternalMpp(displays);

    if (fimd_contents) {
        handleHighPriorityLayers(fimd_contents, mHwc->primaryDisplay,
                primary_previous_drm_dma, previousDRMInternalMPPPrimary, mNeedsReserveFbTargetPrimary);
    }

    if (secondary_contents) {
        handleHighPriorityLayers(secondary_contents, secondary_display,
                secondary_previous_drm_dma, previousDRMInternalMPPSecondary, false);
    }

#ifdef USES_VIRTUAL_DISPLAY
    if (virtual_contents) {
        handleHighPriorityLayers(virtual_contents, mHwc->virtualDisplay,
                virtual_previous_drm_dma, previousDRMInternalMPPVirtual, false);
    }
#endif

    if (fimd_contents) {
        handleLowPriorityLayers(fimd_contents, mHwc->primaryDisplay);
    }

    if (secondary_contents) {
        /* It's mirror mode */
        if ((mHwc->hdmi_hpd == true) && fimd_contents && (fimd_contents->numHwLayers > 0) &&
            (fimd_contents->numHwLayers == secondary_contents->numHwLayers) &&
            (fimd_contents->hwLayers[0].handle == secondary_contents->hwLayers[0].handle))
            secondary_display->mForceFb = true;

        handleLowPriorityLayers(secondary_contents, secondary_display);
    }

#ifdef USES_VIRTUAL_DISPLAY
    if (virtual_contents) {
#ifndef USES_OVERLAY_FOR_WFD_UI_MIRROR
        /* It's mirror mode */
        if (fimd_contents && (fimd_contents->numHwLayers > 0) &&
            (fimd_contents->numHwLayers == virtual_contents->numHwLayers) &&
            (fimd_contents->hwLayers[0].handle == virtual_contents->hwLayers[0].handle))
            mHwc->virtualDisplay->mForceFb = true;
#endif
        handleLowPriorityLayers(virtual_contents, mHwc->virtualDisplay);
    }
#endif

    return 0;
}

void ExynosDisplayResourceManager::preAssignResource()
{
    return;
}

bool ExynosDisplayResourceManager::preAssignIntMpp(ExynosDisplay *display, unsigned int mppType)
{
    int lastFreeIntMppIndex = -1;
    int lastReservedIntMppIndex = -1;
    int lastPreemptableIntMppIndex = -1;
    for (size_t i = 0; i < mInternalMPPs.size(); i++) {
        ExynosMPPModule* exynosMPP = (ExynosMPPModule *)mInternalMPPs[i];
        if ((exynosMPP->mType == mppType) && (exynosMPP->mState == MPP_STATE_FREE)) {
            if (exynosMPP->mDisplay == display)
                lastReservedIntMppIndex = i;
            else if (exynosMPP->mDisplay == NULL)
                lastFreeIntMppIndex = i;
            else
                lastPreemptableIntMppIndex = i;
        }
    }
    if (lastReservedIntMppIndex != -1) {
        ((ExynosMPPModule *)mInternalMPPs[lastReservedIntMppIndex])->preAssignDisplay(display);
        return true;
    } else if (lastFreeIntMppIndex != -1) {
        ((ExynosMPPModule *)mInternalMPPs[lastFreeIntMppIndex])->preAssignDisplay(display);
        return true;
    } else if (lastPreemptableIntMppIndex != -1) {
        ((ExynosMPPModule *)mInternalMPPs[lastPreemptableIntMppIndex])->preAssignDisplay(display);
         return true;
    }
    return false;
}

int ExynosDisplayResourceManager::getInternalMPPFromDMA(unsigned int dma, ExynosMPPModule** internalMPP)
{
    unsigned int mpp_type;
    unsigned int mpp_index;
    *internalMPP = NULL;

    switch (dma) {
    case IDMA_G0:
    case IDMA_G1:
    case IDMA_G2:
    case IDMA_G3:
        /* VPP is always used in DRM playback case */
        return -1;
    case IDMA_VG0:
        mpp_type = MPP_VG;
        mpp_index = 0;
        break;
    case IDMA_VG1:
        mpp_type = MPP_VG;
        mpp_index = 1;
        break;
    case IDMA_VGR0:
        mpp_type = MPP_VGR;
        mpp_index = 0;
        break;
    case IDMA_VGR1:
        mpp_type = MPP_VGR;
        mpp_index = 1;
        break;
    default:
        return -1;
    }

    for (size_t i = 0; i < mInternalMPPs.size(); i++)
    {
        ExynosMPPModule* exynosMPP = (ExynosMPPModule *)mInternalMPPs[i];
        if ((exynosMPP->mType == mpp_type) && (exynosMPP->mIndex == mpp_index)) {
            *internalMPP = exynosMPP;
            return 1;
        }
    }
    return -1;
}
