/*
 * Customer HW 4 dependant file
 *
 * Copyright (C) 1999-2015, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: dhd_sec_feature.h$
 */


/*
 * ** Desciption ***
 * 1. Module vs COB
 *    If your model's WIFI HW chip is COB type, you must add below feature
 *    - #undef USE_CID_CHECK
 *    - #define READ_MACADDR
 *    Because COB type chip have not CID and Mac address.
 *    So, you must add below feature to defconfig file.
 *    - CONFIG_WIFI_BROADCOM_COB
 *
 * 2. PROJECTS
 *    If you want add some feature only own Project, you can add it in 'PROJECTS' part.
 *
 * 3. Region code
 *    If you want add some feature only own region model, you can use below code.
 *    - 100 : EUR OPEN
 *    - 101 : EUR ORG
 *    - 200 : KOR OPEN
 *    - 201 : KOR SKT
 *    - 202 : KOR KTT
 *    - 203 : KOR LGT
 *    - 300 : CHN OPEN
 *    - 400 : USA OPEN
 *    - 401 : USA ATT
 *    - 402 : USA TMO
 *    - 403 : USA VZW
 *    - 404 : USA SPR
 *    - 405 : USA USC
 *    You can refer how to using it below this file.
 *    And, you can add more region code, too.
 */

#ifndef _dhd_sec_feature_h_
#define _dhd_sec_feature_h_

#include <linuxver.h>

#if defined(CONFIG_MACH_UNIVERSAL5433) || defined(CONFIG_MACH_UNIVERSAL7420)
#undef CUSTOM_SET_CPUCORE
#define PRIMARY_CPUCORE 0
#define DPC_CPUCORE 4
#define RXF_CPUCORE 5
#define TASKLET_CPUCORE 5
#define ARGOS_CPU_SCHEDULER
#define ARGOS_RPS_CPU_CTL
#endif /* CONFIG_MACH_UNIVERSAL5433 || CONFIG_MACH_UNIVERSAL7420 */



#endif /* _dhd_sec_feature_h_ */
