#include <sys/ioctl.h>
#include <sys/mman.h>

#include "libscaler-v4l2.h"
#include "libscalerblend-v4l2.h"

bool CScalerBlendV4L2::DevSetCtrl()
{

    if (!SetCtrl())
        return false;

    /* Blending related ctls */
    if (!TestFlag(m_fStatus, SCF_SRC_BLEND))
        return false;

    if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_2D_BLEND_OP,
                         m_SrcBlndCfg.blop) < 0) {
        SC_LOGERR("Failed S_CTRL V4L2_CID_2D_BLEND_OP");
        return false;
    }

    if (m_SrcBlndCfg.globalalpha.enable) {
        if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_GLOBAL_ALPHA, m_SrcBlndCfg.globalalpha.val) < 0) {
               SC_LOGERR("Failed S_CTRL V4L2_CID_GLOBAL_ALPHA");
               return false;
        }
    } else {
        if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_GLOBAL_ALPHA, 0xff) < 0) {
                SC_LOGERR("Failed S_CTRL V4L2_CID_GLOBAL_ALPHA 0xff");
                return false;
        }
    }

    if (m_SrcBlndCfg.cscspec.enable) {
        bool is_bt709 = (m_SrcBlndCfg.cscspec.space == COLORSPACE_REC709)? true : false;
        if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_CSC_EQ, is_bt709) < 0) {
            SC_LOGERR("Failed S_CTRL V4L2_CID_CSC_EQ - %d",
                                                   m_SrcBlndCfg.cscspec.space);
            return false;
        }
        if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_CSC_RANGE, m_SrcBlndCfg.cscspec.wide) < 0) {
            SC_LOGERR("Failed S_CTRL V4L2_CID_CSC_RANGE - %d",
                                                   m_SrcBlndCfg.cscspec.wide);
            return false;
        }
    }

    if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_2D_SRC_BLEND_SET_FMT,
                               m_SrcBlndCfg.srcblendfmt) < 0) {
        SC_LOGERR("Failed V4L2_CID_2D_SRC_BLEND_SET_FMT - %d",
                                                  m_SrcBlndCfg.srcblendfmt);
        return false;
    }

    if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_2D_SRC_BLEND_FMT_PREMULTI,
                           m_SrcBlndCfg.srcblendpremulti) < 0) {
        SC_LOGERR("Failed V4L2_CID_2D_BLEND_FMT_PREMULTI - %d",
                                                   m_SrcBlndCfg.srcblendpremulti);
        return false;
    }

    if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_2D_SRC_BLEND_SET_STRIDE,
                           m_SrcBlndCfg.srcblendstride) < 0) {
        SC_LOGERR("Failed V4L2_CID_2D_SRC_BLEND_SET_STRIDE - %d",
                                                   m_SrcBlndCfg.srcblendstride);
        return false;
    }

    if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_2D_SRC_BLEND_SET_H_POS,
                           m_SrcBlndCfg.srcblendhpos) < 0) {
        SC_LOGERR("Failed V4L2_CID_2D_SRC_BLEND_SET_H_POS with degree %d",
                                                   m_SrcBlndCfg.srcblendhpos);
        return false;
    }

    if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_2D_SRC_BLEND_SET_V_POS,
                           m_SrcBlndCfg.srcblendvpos) < 0) {
        SC_LOGERR("Failed V4L2_CID_2D_SRC_BLEND_SET_V_POS - %d",
                                                   m_SrcBlndCfg.srcblendvpos);
        return false;
    }

    if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_2D_SRC_BLEND_SET_WIDTH,
                           m_SrcBlndCfg.srcblendwidth) < 0) {
        SC_LOGERR("Failed V4L2_CID_2D_SRC_BLEND_SET_WIDTH with degree %d",
                                                   m_SrcBlndCfg.srcblendwidth);
        return false;
    }

    if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_2D_SRC_BLEND_SET_HEIGHT,
                           m_SrcBlndCfg.srcblendheight) < 0) {
        SC_LOGERR("Failed V4L2_CID_2D_SRC_BLEND_SET_HEIGHT - %d",
                                                   m_SrcBlndCfg.srcblendheight);
        return false;
    }

    ClearFlag(m_fStatus, SCF_SRC_BLEND);
    return true;
}

void CScalerBlendV4L2::GetCustomAlphaBlendFmt(int32_t &src_color_space,
                                                     unsigned int srcblendfmt) {

    if (src_color_space == V4L2_PIX_FMT_NV12M) {
        if ((srcblendfmt == V4L2_PIX_FMT_RGB32))
            src_color_space = V4L2_PIX_FMT_NV12M_RGB32;
        else if ((srcblendfmt == V4L2_PIX_FMT_BGR32))
            src_color_space = V4L2_PIX_FMT_NV12M_BGR32;
        else if ((srcblendfmt == V4L2_PIX_FMT_RGB565))
            src_color_space = V4L2_PIX_FMT_NV12M_RGB565;
        else if ((srcblendfmt == V4L2_PIX_FMT_RGB444))
            src_color_space = V4L2_PIX_FMT_NV12M_RGB444;
        else if ((srcblendfmt == V4L2_PIX_FMT_RGB555X))
            src_color_space = V4L2_PIX_FMT_NV12M_RGB555X;
    } else if (src_color_space == V4L2_PIX_FMT_NV12) {
            if ((srcblendfmt == V4L2_PIX_FMT_RGB32))
            src_color_space = V4L2_PIX_FMT_NV12_RGB32;
    } else if (src_color_space == V4L2_PIX_FMT_NV12N) {
            if ((srcblendfmt == V4L2_PIX_FMT_RGB32))
            src_color_space = V4L2_PIX_FMT_NV12N_RGB32;
    } else if (src_color_space == V4L2_PIX_FMT_NV12MT_16X16) {
            if ((srcblendfmt == V4L2_PIX_FMT_RGB32))
            src_color_space = V4L2_PIX_FMT_NV12MT_16X16_RGB32;
    } else if (src_color_space == V4L2_PIX_FMT_NV21M) {
        if ((srcblendfmt == V4L2_PIX_FMT_RGB32))
            src_color_space = V4L2_PIX_FMT_NV21M_RGB32;
        else if ((srcblendfmt == V4L2_PIX_FMT_BGR32))
            src_color_space = V4L2_PIX_FMT_NV21M_BGR32;
    } else if (src_color_space == V4L2_PIX_FMT_NV21) {
        if ((srcblendfmt == V4L2_PIX_FMT_RGB32))
            src_color_space = V4L2_PIX_FMT_NV21_RGB32;
    } else if (src_color_space == V4L2_PIX_FMT_YVU420) {
        if ((srcblendfmt == V4L2_PIX_FMT_RGB32))
            src_color_space = V4L2_PIX_FMT_YVU420_RGB32;
    } else {
        src_color_space = -1;
    }
}

CScalerBlendV4L2::CScalerBlendV4L2(int dev_num, int allow_drm) : CScalerV4L2(dev_num, allow_drm){

}

CScalerBlendV4L2::~CScalerBlendV4L2(){

}
