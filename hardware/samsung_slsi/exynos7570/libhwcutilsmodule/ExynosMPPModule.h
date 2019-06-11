#ifndef EXYNOS_GSC_MODULE_H
#define EXYNOS_GSC_MODULE_H

#include "ExynosMPPv2.h"

class ExynosDisplay;

class ExynosMPPModule : public ExynosMPP {
    public:
        ExynosMPPModule();
        ExynosMPPModule(ExynosDisplay *display, int gscIndex);
        ExynosMPPModule(ExynosDisplay *display, unsigned int mppType, unsigned int mppIndex);
        virtual bool isFormatSupportedByMPP(int format);
    protected:
        virtual int getBufferUsage(private_handle_t *srcHandle);
};

#endif
