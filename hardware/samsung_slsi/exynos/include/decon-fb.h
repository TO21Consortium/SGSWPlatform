/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef ___SAMSUNG_DECON_H__
#define ___SAMSUNG_DECON_H__
#define S3C_FB_MAX_WIN (7)
#define MAX_DECON_WIN (7)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define DECON_WIN_UPDATE_IDX MAX_DECON_WIN
#define MAX_BUF_PLANE_CNT (3)
typedef unsigned int u32;
#if defined(USES_ARCH_ARM64) || defined(USES_DECON_64BIT_ADDRESS)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
typedef uint64_t dma_addr_t;
#else
typedef uint32_t dma_addr_t;
#endif
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct decon_win_rect {
  int x;
  int y;
  u32 w;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  u32 h;
};
struct decon_rect {
  int left;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int top;
  int right;
  int bottom;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct s3c_fb_user_window {
  int x;
  int y;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct s3c_fb_user_plane_alpha {
  int channel;
  unsigned char red;
  unsigned char green;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned char blue;
};
struct s3c_fb_user_chroma {
  int enabled;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned char red;
  unsigned char green;
  unsigned char blue;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct s3c_fb_user_ion_client {
  int fd[MAX_BUF_PLANE_CNT];
  int offset;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum decon_pixel_format {
  DECON_PIXEL_FORMAT_ARGB_8888 = 0,
  DECON_PIXEL_FORMAT_ABGR_8888,
  DECON_PIXEL_FORMAT_RGBA_8888,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  DECON_PIXEL_FORMAT_BGRA_8888,
  DECON_PIXEL_FORMAT_XRGB_8888,
  DECON_PIXEL_FORMAT_XBGR_8888,
  DECON_PIXEL_FORMAT_RGBX_8888,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  DECON_PIXEL_FORMAT_BGRX_8888,
  DECON_PIXEL_FORMAT_RGBA_5551,
  DECON_PIXEL_FORMAT_RGB_565,
  DECON_PIXEL_FORMAT_NV16,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  DECON_PIXEL_FORMAT_NV61,
  DECON_PIXEL_FORMAT_YVU422_3P,
  DECON_PIXEL_FORMAT_NV12,
  DECON_PIXEL_FORMAT_NV21,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  DECON_PIXEL_FORMAT_NV12M,
  DECON_PIXEL_FORMAT_NV21M,
  DECON_PIXEL_FORMAT_YUV420,
  DECON_PIXEL_FORMAT_YVU420,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  DECON_PIXEL_FORMAT_YUV420M,
  DECON_PIXEL_FORMAT_YVU420M,
  DECON_PIXEL_FORMAT_NV12N,
  DECON_PIXEL_FORMAT_NV12N_10B,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  DECON_PIXEL_FORMAT_MAX,
};
enum decon_blending {
  DECON_BLENDING_NONE = 0,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  DECON_BLENDING_PREMULT = 1,
  DECON_BLENDING_COVERAGE = 2,
  DECON_BLENDING_MAX = 3,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum otf_status {
  S3C_FB_DMA,
  S3C_FB_LOCAL,
  S3C_FB_STOP_DMA,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  S3C_FB_READY_TO_LOCAL,
};
enum vpp_rotate {
  VPP_ROT_NORMAL = 0x0,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  VPP_ROT_XFLIP,
  VPP_ROT_YFLIP,
  VPP_ROT_180,
  VPP_ROT_90,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  VPP_ROT_90_XFLIP,
  VPP_ROT_90_YFLIP,
  VPP_ROT_270,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum vpp_csc_eq {
  BT_601_NARROW = 0x0,
  BT_601_WIDE,
  BT_709_NARROW,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  BT_709_WIDE,
};
enum decon_idma_type {
  IDMA_G0 = 0x0,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  IDMA_G1,
  IDMA_VG0,
  IDMA_VG1,
  IDMA_VGR0,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  IDMA_VGR1,
  IDMA_G2,
  IDMA_G3,
  MAX_DECON_DMA_TYPE
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct vpp_params {
  dma_addr_t addr[MAX_BUF_PLANE_CNT];
  enum vpp_rotate rot;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  enum vpp_csc_eq eq_mode;
};
struct decon_frame {
  int x;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int y;
  u32 w;
  u32 h;
  u32 f_w;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  u32 f_h;
};
struct decon_win_config {
  enum {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    DECON_WIN_STATE_DISABLED = 0,
    DECON_WIN_STATE_COLOR,
    DECON_WIN_STATE_BUFFER,
    DECON_WIN_STATE_UPDATE,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  } state;
  union {
    __u32 color;
    struct {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
      int fd_idma[3];
      int fence_fd;
      int plane_alpha;
      enum decon_blending blending;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
      enum decon_idma_type idma_type;
      enum decon_pixel_format format;
      struct vpp_params vpp_parm;
      struct decon_win_rect block_area;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
      struct decon_win_rect transparent_area;
      struct decon_win_rect covered_opaque_area;
      struct decon_frame src;
    };
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  };
  struct decon_frame dst;
  bool protection;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct decon_win_config_data {
  int fence;
  int fd_odma;
  struct decon_win_config config[MAX_DECON_WIN + 1];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct decon_dual_display_blank_data {
  enum {
    DECON_PRIMARY_DISPLAY = 0,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    DECON_SECONDARY_DISPLAY,
  } display_type;
  int blank;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum disp_pwr_mode {
  DECON_POWER_MODE_OFF = 0,
  DECON_POWER_MODE_DOZE,
  DECON_POWER_MODE_NORMAL,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  DECON_POWER_MODE_DOZE_SUSPEND,
};
#define S3CFB_WIN_POSITION _IOW('F', 203, struct s3c_fb_user_window)
#define S3CFB_WIN_SET_PLANE_ALPHA _IOW('F', 204, struct s3c_fb_user_plane_alpha)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define S3CFB_WIN_SET_CHROMA _IOW('F', 205, struct s3c_fb_user_chroma)
#define S3CFB_SET_VSYNC_INT _IOW('F', 206, __u32)
#define S3CFB_GET_ION_USER_HANDLE _IOWR('F', 208, struct s3c_fb_user_ion_client)
#define S3CFB_WIN_CONFIG _IOW('F', 209, struct decon_win_config_data)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define S3CFB_DUAL_DISPLAY_BLANK _IOW('F', 300, struct decon_dual_display_blank_data)
#define S3CFB_WIN_PSR_EXIT _IOW('F', 210, int)
#define EXYNOS_GET_HDMI_CONFIG _IOW('F', 220, struct exynos_hdmi_data)
#define EXYNOS_SET_HDMI_CONFIG _IOW('F', 221, struct exynos_hdmi_data)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define S3CFB_POWER_MODE _IOW('F', 223, __u32)
#endif
