/*
 *    Copyright (C) 2013 SAMSUNG S.LSI
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 *   Author: Woonki Lee <woonki84.lee@samsung.com>
 *   Version: 2.0
 *
 */

#include <hardware/nfc.h>

#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "osi.h"
#include "hal.h"
#include "hal_msg.h"
#include "util.h"

#include <cutils/properties.h>
#ifdef FWU_APK
#include "sverifysignature.h"
#endif

int hal_vs_send(uint8_t *data, size_t size)
{
    int ret;

    ret = __send_to_device(data, size);
    if (ret != (int)size)
        OSI_loge("VS message send failed");

    return ret;
}

int hal_vs_nci_send(uint8_t oid, uint8_t *data, size_t size)
{
    tNFC_NCI_PKT nci_pkt;
    nci_pkt.oct0 = NCI_MT_CMD | NCI_PBF_LAST | NCI_GID_PROP;
    nci_pkt.oid = oid;
    nci_pkt.len = size;

    if (data)
        memcpy(nci_pkt.payload, data, size);

    return hal_nci_send(&nci_pkt);
}

int hal_vs_get_prop(int n, tNFC_NCI_PKT *pkt)
{
    char prop_field[15];

    pkt->oct0 = NCI_MT_CMD | NCI_GID_PROP | NCI_PBF_LAST;
    pkt->oid = get_config_propnci_get_oid(n);
    snprintf(prop_field, sizeof(prop_field), "NCI_PROP0x%02X", NCI_OID(pkt));
    pkt->len = get_config_byteArry(prop_field, pkt->payload, NCI_MAX_PAYLOAD);

    return (size_t)(pkt->len + NCI_HDR_SIZE);
}

size_t nfc_hal_vs_get_rfreg(int id, tNFC_NCI_PKT *pkt)
{
    char field_name[50];
    uint8_t *buffer = (uint8_t *)pkt;
    size_t size;

    snprintf(field_name, sizeof(field_name), "%s%d", cfg_name_table[CFG_RF_REG], id);
    size = get_config_byteArry(field_name, pkt->payload+1, NCI_MAX_PAYLOAD-1);
    if (size > 0 )
    {
        pkt->oct0 = NCI_MT_CMD | NCI_PBF_LAST | NCI_GID_PROP;
        pkt->oid = NCI_PROP_SET_RFREG;
        pkt->len = size + 1;
        pkt->payload[0] = id;
        size += 4;
    }
    return size;
}

bool hal_vs_is_rfreg_file(tNFC_HAL_VS_INFO *vs)
{
// Start [S15083101] APIs for RF Tuning
    size_t ret = 0;
// End [S15083101] APIs for RF Tuning

    char system_rfreg_file[256];
    uint8_t system_rfreg_time[RFREG_VERSION_SIZE];

    memset(system_rfreg_time, 0, RFREG_VERSION_SIZE);


    if(get_config_string(cfg_name_table[CFG_RFREG_FILE], system_rfreg_file, sizeof(system_rfreg_file)))
    {
        if(nfc_rf_getver_from_image(system_rfreg_time, system_rfreg_file))
        {
            memcpy(vs->rfreg_file, system_rfreg_file, 256);
            memcpy(vs->rfreg_img_version, system_rfreg_time, RFREG_VERSION_SIZE);

            return true;
        }
        else
            OSI_loge("Not found System RF Register File!");
    }
    else
        OSI_loge("Not found System RF Register Suffix.");

// Start [S15083101] APIs for RF Tuning
            ret = 0;
            get_config_int(cfg_name_table[CFG_UPDATE_MODE], (int *)&ret);
            if(ret == 0x03)
            {
	        get_config_string("RFREG_FILE_RFTUNE", vs->rfreg_file, sizeof(vs->rfreg_file));

	        return true;
            }
// End [S15083101] APIs for RF Tuning

    return false;
}

bool hal_vs_check_rfreg_update(tNFC_HAL_VS_INFO *vs, tNFC_NCI_PKT *pkt)
{
    tNFC_HAL_FW_BL_INFO *bl = &nfc_hal_info.fw_info.bl_info;
    char rfreg_version_buffer[30], rfreg_img_version_buffer[30];
    char rfreg_number_version[10], rfreg_img_number_version[10];
    struct stat attrib;
    int i;
    if (stat(vs->rfreg_file, &attrib) != 0)
    {
        OSI_loge("hal_vs_check_rfreg_update return false");
        return false;
    }

    // set rf register version to structure
    memset(rfreg_version_buffer, 0, 30);
    memset(rfreg_img_version_buffer, 0, 30);

    if(SNFC_N8 <= bl->version[0])
    {
        vs->rfreg_number_version[0] = NCI_PAYLOAD(pkt)[0]>>4;              // main version
        vs->rfreg_number_version[1] = NCI_PAYLOAD(pkt)[0]&(~(0xf<<4));     // sub version
        vs->rfreg_number_version[2] = NCI_PAYLOAD(pkt)[1];                 // patch version
        vs->rfreg_number_version[3] = NCI_PAYLOAD(pkt)[11];                // minor version (number version)

        sprintf(rfreg_number_version, "%d.%d.%d.%d",
            vs->rfreg_number_version[0], vs->rfreg_number_version[1],
            vs->rfreg_number_version[2], vs->rfreg_number_version[3]);
        property_set("nfc.fw.rfreg_display_ver", rfreg_number_version);
        OSI_logd("RF Register Display Version : %s", rfreg_number_version);

        vs->rfreg_version[0] = (NCI_PAYLOAD(pkt)[5]>>4)+14;        // year
        vs->rfreg_version[1] = NCI_PAYLOAD(pkt)[5]&(~(0xf<<4));    // month
        vs->rfreg_version[2] = NCI_PAYLOAD(pkt)[6];                // day
        vs->rfreg_version[3] = NCI_PAYLOAD(pkt)[7];                // hour
        vs->rfreg_version[4] = NCI_PAYLOAD(pkt)[8];                // min
        vs->rfreg_version[5] = NCI_PAYLOAD(pkt)[9];                // sec
    }
    else
    {
    for (i = 0; i < 8; i++)
            vs->rfreg_version[i] = NCI_PAYLOAD(pkt)[i+1];
    }

    sprintf(rfreg_version_buffer, "%d/%d/%d/%d.%d.%d",
        vs->rfreg_version[0], vs->rfreg_version[1], vs->rfreg_version[2],
        vs->rfreg_version[3], vs->rfreg_version[4], vs->rfreg_version[5] );
    OSI_logd("RF Version (F/W): %s", rfreg_version_buffer);

    sprintf(rfreg_img_version_buffer, "%d/%d/%d/%d.%d.%d",
        vs->rfreg_img_version[0], vs->rfreg_img_version[1], vs->rfreg_img_version[2],
        vs->rfreg_img_version[3], vs->rfreg_img_version[4], vs->rfreg_img_version[5] );
    OSI_logd("RF Version (BIN): %s", rfreg_img_version_buffer);


    if (memcmp(vs->rfreg_img_version, vs->rfreg_version, RFREG_VERSION_SIZE) == 0)
    {
        OSI_logd("nfc.fw.rfreg_ver is %s!!!", rfreg_version_buffer);
        property_set("nfc.fw.rfreg_ver", rfreg_version_buffer);
        return false;
    }
    memcpy(vs->rfreg_version, vs->rfreg_img_version, 8);
    OSI_logd("nfc.fw.rfreg_ver is %s!!!!", rfreg_img_version_buffer);
    property_set("nfc.fw.rfreg_ver", rfreg_img_version_buffer);
    if(SNFC_N8 <= bl->version[0])
    {
        sprintf(rfreg_img_number_version, "%d.%d.%d.%d",
            vs->rfreg_img_number_version[0], vs->rfreg_img_number_version[1],
            vs->rfreg_img_number_version[2], vs->rfreg_img_number_version[3]);
        OSI_logd("RF Register Display Version : %s!!!!", rfreg_img_number_version);
        property_set("nfc.fw.rfreg_display_ver", rfreg_img_number_version);
    }
    return true;
}

#define RFREG_SECTION_SIZE    252
bool hal_vs_rfreg_update(tNFC_HAL_VS_INFO *vs)
{
    FILE *file;
    uint8_t data[RFREG_SECTION_SIZE+1];
    uint32_t *check_sum;
    size_t size;
    int total, next = vs->rfregid * RFREG_SECTION_SIZE;
    int i;

    if ((file = fopen(vs->rfreg_file, "rb")) == NULL)
    {
        OSI_loge("Failed to open RF REG file (file: %s : ret: %d)", vs->rfreg_file, errno);
        OSI_loge("next rf reg id: %d", vs->rfregid);
        memset(vs->rfreg_img_version, 0xFF, sizeof(uint8_t) * 8);
        return false;
    }

    fseek(file, 0, SEEK_END);

    total = (int)ftell(file);

    if (total <= next)
    {
        fclose(file);
        return false;
    }

    fseek(file, next, SEEK_SET);

    size = fread(data+1, 1, RFREG_SECTION_SIZE, file);

    fclose(file);

    // checksum
    check_sum = (uint32_t *)(data+1);
    for (i = 1; i <= (int)size; i+=4)
        vs->check_sum += *check_sum++;

    data[0] = vs->rfregid;
    OSI_logd("Sent RF register #id: 0x%02X", vs->rfregid);
    vs->rfregid++;
    hal_vs_nci_send(NCI_PROP_SET_RFREG, data, size+1);

    return true;
}

bool nfc_rf_getver_from_image(uint8_t *rfreg_ver, char *file_name)
{
    tNFC_HAL_FW_BL_INFO *bl = &nfc_hal_info.fw_info.bl_info;
    FILE *file;
    uint8_t value[16];
    unsigned int tmp;

    if ((file = fopen(file_name, "rb")) == NULL)
    {
        OSI_loge("Failed to open RF Register file (file: %s", file_name);
        return false;
    }
    // File size check
    fseek(file, 0, SEEK_END);
    tmp = (unsigned int)ftell(file);
    if (tmp < 24)   // TODO: FILE header size
    {
        OSI_loge("Invalid file format!!");
        fclose(file);
        return false;
    }
    // Get RF Register Version
    fseek(file, -16, SEEK_END);
    fread(value, 16, 1, file);

    rfreg_ver[0] = (value[5]>>4)+14;        // year
    rfreg_ver[1] = value[5]&(~(0xf<<4));    // month
    rfreg_ver[2] = value[6];                // day
    rfreg_ver[3] = value[7];                // hour
    rfreg_ver[4] = value[8];                // min
    rfreg_ver[5] = value[9];                // sec

    if(SNFC_N8 <= bl->version[0])
    {
        nfc_hal_info.vs_info.rfreg_img_number_version[0] = value[0]>>4;              // main version
        nfc_hal_info.vs_info.rfreg_img_number_version[1] = value[0]&(~(0xf<<4));     // sub version
        nfc_hal_info.vs_info.rfreg_img_number_version[2] = value[1];                 // patch version
        nfc_hal_info.vs_info.rfreg_img_number_version[3] = value[11];                // minor version (number version)
    }
    fclose(file);

    return true;
}
