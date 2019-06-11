#ifndef MPP_FACTORY_H_
#define MPP_FACTORY_H_

#include "LibMpp.h"

class MppFactory {
public:
    MppFactory() {
        ALOGD("%s\n", __func__);
    }
    virtual ~MppFactory() {
        ALOGD("%s\n", __func__);
    }
    virtual LibMpp *CreateMpp(int id, int mode, int outputMode, int drm);
    virtual LibMpp *CreateBlendMpp(int id, int mode, int outputMode, int drm);
};
#endif
