#ifndef __SAMSUNG_WIFI_H__
#define __SAMSUNG_WIFI_H__

extern int samsung_wlan_init(struct platform_device *pdev);
extern struct wifi_platform_data espresso_wifi_control;
extern int irq_oob;
//extern void somc_wifi_mmc_host_register(struct mmc_host *host);

#endif /* __SAMSUNG_WIFI_H__ */

