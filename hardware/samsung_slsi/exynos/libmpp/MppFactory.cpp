#include "MppFactory.h"
#include "LibMpp.h"

LibMpp *MppFactory::CreateMpp(int id, int mode, int outputMode, int drm)
{
    ALOGD("%s dev(%d) mode(%d) drm(%d), \n", __func__, id, mode, drm);
#ifdef USES_GSCALER
    return reinterpret_cast<LibMpp *>(exynos_gsc_create_exclusive(id,
					    mode, outputMode, drm));
#else
    return reinterpret_cast<LibMpp *>(exynos_fimc_create_exclusive(id,
					    mode, outputMode, drm));
#endif
}

LibMpp *MppFactory::CreateBlendMpp(int id, int mode, int outputMode, int drm)
{
    ALOGD("%s(%d)\n", __func__, __LINE__);

#ifdef USES_GSCALER
    return reinterpret_cast<LibMpp *>(exynos_gsc_create_blend_exclusive(id,
					    mode, outputMode, drm));
#endif
    return NULL;
}
