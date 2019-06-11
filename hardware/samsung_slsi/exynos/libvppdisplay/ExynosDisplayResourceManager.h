#ifndef EXYNOS_RESOURCE_MANAGER_H
#define EXYNOS_RESOURCE_MANAGER_H

#include <utils/SortedVector.h>
#include <hardware/hwcomposer.h>
#include <utils/String8.h>

class ExynosMPPModule;
class ExynosDisplay;

class ExynosMPPVector : public android::SortedVector< ExynosMPPModule* > {
    public:
        ExynosMPPVector();
        ExynosMPPVector(const ExynosMPPVector& rhs);
        virtual int do_compare(const void* lhs, const void* rhs) const;
};

class ExynosDisplayResourceManager {
    public:
        ExynosDisplayResourceManager(struct exynos5_hwc_composer_device_1_t *pdev);
        virtual ~ExynosDisplayResourceManager();
        ExynosMPPVector mInternalMPPs;
        android::Vector< ExynosMPPModule* > mExternalMPPs;
        struct exynos5_hwc_composer_device_1_t *mHwc;
        virtual int assignResources(size_t numDisplays, hwc_display_contents_1_t** displays);
        void cleanupMPPs();
        void dumpMPPs(android::String8& result);
    protected:
        bool mNeedsReserveFbTargetPrimary;

        void removeUnAssignedIntMpp(ExynosMPPVector &internalMPPs);
        void addUnAssignedIntMpp(ExynosDisplay *display);
        void addExternalMpp(hwc_display_contents_1_t** contents);
        virtual void preAssignResource();
        virtual bool preAssignIntMpp(ExynosDisplay *display, unsigned int mppType);
        void doPreProcessing(hwc_display_contents_1_t* contents, ExynosDisplay* display,
                int *previous_drm_dma, ExynosMPPModule **previousDRMInternalMPP);
        void handleHighPriorityLayers(hwc_display_contents_1_t* contents, ExynosDisplay* display,
                int previous_drm_dma, ExynosMPPModule *previousDRMInternalMPP, bool reserveFbTarget);
        void handleLowPriorityLayers(hwc_display_contents_1_t* contents, ExynosDisplay* display);
        void printDisplyInfos(size_t type);
        int  getInternalMPPFromDMA(unsigned int dma, ExynosMPPModule** internalMPP);
};

#endif
