#ifndef __FM_HAL_APP_H__
#define __FM_HAL_APP_H__

#include <pthread.h>

enum fm_command {
	FM_CMD_ON = 1,		/* 1 */
	FM_CMD_OFF,
	FM_CMD_SEEK_UP,
	FM_CMD_SEEK_DOWN,
	FM_CMD_SEEK_CANCEL,

	FM_CMD_SET_CHANNEL,	/* 6 */
	FM_CMD_GET_CHANNEL,
	FM_CMD_SET_BAND,
	FM_CMD_SET_CHANNEL_SPACING,
	FM_CMD_SET_STEREO,

	FM_CMD_SET_MONO,	/* 11 */
	FM_CMD_MUTE_ON,
	FM_CMD_MUTE_OFF,
	FM_CMD_SOFT_MUTE,
	FM_CMD_SET_DECONSTANT,

	FM_CMD_ENABLE_RDS,	/* 16 */
	FM_CMD_DISABLE_RDS,
	FM_CMD_ENABLE_DNS,
	FM_CMD_DISABLE_DNS,
	FM_CMD_ENABLE_AF,

	FM_CMD_DISABLE_AF,	/* 21 */
	FM_CMD_CANCEL_AF_SWITCHING,
	FM_CMD_GET_CURRENT_RSSI,
	FM_CMD_GET_CURRENT_SNR,
	FM_CMD_SET_CURRENT_RSSI,

	FM_CMD_GET_SEEK_MODE,	/* 26 */
	FM_CMD_SET_SEEK_MODE,
	FM_CMD_START_RDS_DATA,
	FM_CMD_STOP_RDS_DATA,

	FM_CMD_SET_SOFT_STEREO_BLEND_COEFF = 51,
	FM_CMD_SET_SOFT_MUTE_COEFF,
};

/*******************************************************************************
 *
 * Decalarations for RDS, DNS, and AF
 *
 ******************************************************************************/

typedef struct
{
	unsigned short rdsa;
	unsigned short rdsb;
	unsigned short rdsc;
	unsigned short rdsd;
	unsigned char  curr_rssi;
	unsigned int curr_channel;
	unsigned char blera;
	unsigned char blerb;
	unsigned char blerc;
	unsigned char blerd;
}radio_data_t;

struct PIECC_data
{
	unsigned short PI;
	unsigned char ECC;
};

#define PS_MAXIMUM_SIZE 8
#define RT_MAXIMUM_SIZE 64

struct Final_RDS_data
{
	char StationName[PS_MAXIMUM_SIZE+1], RadioText[RT_MAXIMUM_SIZE+1];
	long Af_frequency;
};

#define RTPLUS_TAG_MAXIMUM_SIZE 2

struct RTPlusTagInfo
{
	int contentType;
	int startPos;
	int additionalLen;
};

struct RTPlus_data
{
	bool bToggle;
	bool bRunning;
	unsigned char bValidated;
	RTPlusTagInfo RTPlusTag[RTPLUS_TAG_MAXIMUM_SIZE];
};

#define MAX_AF_NUM 25

typedef struct
{
	unsigned char curr_rssi;
	unsigned char curr_rssi_th;
	unsigned char curr_snr;
}rssi_snr_t;

typedef struct
{
	unsigned char AF[MAX_AF_NUM];
	unsigned char RSSI[MAX_AF_NUM];
	unsigned char AFCount;
}AF_List;

typedef struct
{
	char Text[RT_MAXIMUM_SIZE+1];
	unsigned char bChangeFlag;
	unsigned char bValidated;
	unsigned char iBytesReceived;
	unsigned char iLenght;
}RadioText;

typedef struct
{
	char Text[PS_MAXIMUM_SIZE+1];
	unsigned char iBytesReceived;
	unsigned char bValidated;
	unsigned char iLenght;
}ServiceName;

/*******************************************************************************
 * Global variables for test
 ******************************************************************************/
#define TEST_DATA_SIZE 10

extern unsigned char test_read_data[TEST_DATA_SIZE][12];
extern radio_data_t test_radio_data[TEST_DATA_SIZE];
extern struct PIECC_data test_piecc[TEST_DATA_SIZE];
extern struct RTPlus_data test_rtplus[TEST_DATA_SIZE];
extern struct Final_RDS_data final_rds_data[10];

extern RadioText RT_Final;
extern RadioText RT_Buffered;
extern ServiceName PS_Final;
extern ServiceName PS_Buffered;

extern unsigned char rt_flag;
extern unsigned char ps_flag;

extern unsigned int test_data_index;
extern unsigned int test_data_index_rtplus;
extern unsigned int test_data_index_rds;

#endif /* __FM_HAL_APP_H__ */
