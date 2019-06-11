#ifndef EXYNOS_VIRTUAL_DISPLAY_MODULE_H
#define EXYNOS_VIRTUAL_DISPLAY_MODULE_H

#include "ExynosVirtualDisplay.h"

class ExynosVirtualDisplayModule : public ExynosVirtualDisplay {
	public:
		ExynosVirtualDisplayModule(struct exynos5_hwc_composer_device_1_t *pdev);
		~ExynosVirtualDisplayModule();

		int postFrame(hwc_display_contents_1_t *contents);
		int postToMPP(hwc_layer_1_t & layer, hwc_layer_1_t *layerB,
						int index, hwc_display_contents_1_t *contents);
		void processGles(hwc_display_contents_1_t *contents);
		void processHwc(hwc_display_contents_1_t *contents);
		void processMixed(hwc_display_contents_1_t *contents);
		bool is2StepBlendingRequired(hwc_layer_1_t & layer, buffer_handle_t & outbuf);
		bool manageFences(hwc_display_contents_1_t *contents, int fence);

		virtual int clearDisplay();
		virtual int32_t getDisplayAttributes(const uint32_t attribute);
		virtual int prepare(hwc_display_contents_1_t *contents);
		virtual int set(hwc_display_contents_1_t *contents);
		virtual void determineYuvOverlay(hwc_display_contents_1_t *contents);
		virtual void determineSupportedOverlays(hwc_display_contents_1_t *contents);
		virtual bool isOverlaySupported(hwc_layer_1_t & layer,
				size_t index, bool useVPPOverlay,
				ExynosMPPModule **supportedInternalMPP,
				ExynosMPPModule **supportedExternalMPP);
		virtual void deInit();
};

#endif
