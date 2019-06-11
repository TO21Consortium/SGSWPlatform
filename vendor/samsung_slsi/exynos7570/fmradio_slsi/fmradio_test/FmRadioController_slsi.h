#ifndef __FM_RADIO_CONTROLLER_SLSI_H__
#define __FM_RADIO_CONTROLLER_SLSI_H__

#define FM_RADIO_TEST_MODE

#include <pthread.h>

#if defined(FM_RADIO_TEST_MODE)
#include "FmRadioController_test.h"
#else
#include "FmRadioRDSParser.h"
#endif
#include <linux/videodev2.h>

#define FM_RADIO_DEVICE "/dev/radio0"

#define FM_FAILURE -1
#define FM_SUCCESS 0

enum fm_radio_state
{
    FM_RADIO_OFF = 0,
    FM_RADIO_ON,
    FM_RADIO_WORKING,
    FM_RADIO_STOP,
};

/*******************************************************************************
 *
 * Definitions for radio basic functions
 *
 ******************************************************************************/

enum fm_radio_seek_dir
{
    FM_RADIO_SEEK_DOWN = 0,
    FM_RADIO_SEEK_UP,
};

enum fm_radio_band
{
    FM_RADIO_BAND_EUR = 0, /* European/U.S. (87.5 - 108)MHz */
    FM_RADIO_BAND_JAP,     /* Japanese      (76   -  90)MHz */
};

enum fm_radio_mute
{
    FM_RADIO_MUTE_ON = 0,
    FM_RADIO_MUTE_OFF,
};

enum fm_radio_tuner_mode
{
    FM_RADIO_TUNER_MONO = 0,
    FM_RADIO_TUNER_STEREO,
};

#define FM_RADIO_SPACING_50KHZ  5
#define FM_RADIO_SPACING_100KHZ 10
#define FM_RADIO_SPACING_200KHZ 20

enum fm_radio_deconstant_val
{
    FM_RADIO_DE_TIME_CONSTANT_0 = 0,
    FM_RADIO_DE_TIME_CONSTANT_75,
};

enum fm_radio_search_mode
{
    FM_RADIO_STOP_SEARCH_MODE = 0,
    FM_RADIO_PRESET_MODE,
    FM_RADIO_AUTONOMOUS_SEARCH_MODE,
    FM_RADIO_AF_JUMP_MODE,
    FM_RADIO_AUTONOMOUS_SEARCH_MODE_SKIP,
};

#define V4L2_CID_USER_S610_BASE (V4L2_CID_USER_BASE + 0x1070)

enum s610_ctrl_id {
    V4L2_CID_S610_CH_SPACING              = (V4L2_CID_USER_S610_BASE + 0x01),
    V4L2_CID_S610_CH_BAND                 = (V4L2_CID_USER_S610_BASE + 0x02),
    V4L2_CID_S610_SOFT_STEREO_BLEND       = (V4L2_CID_USER_S610_BASE + 0x03),
    V4L2_CID_S610_SOFT_STEREO_BLEND_COEFF = (V4L2_CID_USER_S610_BASE + 0x04),
    V4L2_CID_S610_SOFT_MUTE_COEFF         = (V4L2_CID_USER_S610_BASE + 0x05),
    V4L2_CID_S610_RSSI_CURR               = (V4L2_CID_USER_S610_BASE + 0x06),
    V4L2_CID_S610_SNR_CURR                = (V4L2_CID_USER_S610_BASE + 0x07),
    V4L2_CID_S610_SEEK_CANCEL             = (V4L2_CID_USER_S610_BASE + 0x08),
    V4L2_CID_S610_SEEK_MODE               = (V4L2_CID_USER_S610_BASE + 0x09),
    V4L2_CID_S610_RDS_ON                  = (V4L2_CID_USER_S610_BASE + 0x0A),
    V4L2_CID_S610_IF_COUNT1               = (V4L2_CID_USER_S610_BASE + 0x0B),
    V4L2_CID_S610_IF_COUNT2               = (V4L2_CID_USER_S610_BASE + 0x0C),
    V4L2_CID_S610_RSSI_TH                 = (V4L2_CID_USER_S610_BASE + 0x0D),
};

/*******************************************************************************
 *
 * Decalarations for RDS
 *
 ******************************************************************************/

#define FM_RADIO_ENABLED_RDS (1 << 0)
#define FM_RADIO_ENABLED_DNS (1 << 1)
#define FM_RADIO_ENABLED_AF  (1 << 2)
#define FM_RADIO_ENABLED_ALL (FM_RADIO_ENABLED_RDS | FM_RADIO_ENABLED_DNS | FM_RADIO_ENABLED_AF)

#define FM_RADIO_RDS_DATA_MAX 48

#define FM_RADIO_RDS_GRP_NUM 3
#define FM_RADIO_RDS_BLK_NUM 4
#define FM_RADIO_RDS_SET_NUM (FM_RADIO_RDS_GRP_NUM * FM_RADIO_RDS_BLK_NUM)

#define FM_RADIO_RDS_BLER_MASK (0x3 << 3)

enum fm_radio_rds_index
{
    FM_RADIO_RDS_LSB = 0,
    FM_RADIO_RDS_MSB,
    FM_RADIO_RDS_BLER,
};

#if !defined(FM_RADIO_TEST_MODE)
extern void RDSDataReceived(Final_RDS_data rdsData);
extern void RTPlusDataReceived(RTPlus_data rtplusData);
extern void PIECCDataReceived(PIECC_data PIECCData);
extern void AFDataReceived(long AF);
extern void CallbackAFStarted();
#endif

/*******************************************************************************
 *
 * FmRadioController_slsi Class
 *
 ******************************************************************************/

class FmRadioController_slsi {
private:
    int radio_state;
    int radio_fd;

    int is_mute_on;
    int tuner_mode;
    int channel_spacing;
    int current_band;
    long current_volume;
    bool is_seeking;

    /* RDS/DNS/AF */
#if !defined(FM_RADIO_TEST_MODE)
    FmRadioRDSParser RDSParser;
    AF_List AFList;
#endif
    bool radio_thread_termination;
    pthread_t radio_rds_thread;
    pthread_t radio_af_thread;
    unsigned char is_enabled_functions;
    unsigned char is_af_switching;
    unsigned short current_pi;
    long curr_channel;
    int af_threshold;
    int afvalid_threshold;
    bool cancel_af_switching;

    /* Sub functions for RDS/DNS/AF */
    int startThreadforFmRadio(unsigned char func);
    int stopThreadforFmRadio(unsigned char func);
    static void* radio_thread_handler(void *arg);
    void radio_data_proc(unsigned char *buf, int count);
#if !defined(FM_RADIO_TEST_MODE)
    void radio_af_switching();
    static void* radio_af_thread_handler(void *arg);
    int af_signal_check(AF_List *af_list);
#endif

public:
    FmRadioController_slsi();
    ~FmRadioController_slsi();

    int Initialise();

    void TuneChannel(long channel);
    long GetChannel();

    long SeekUp();
    long SeekDown();
    long SearchUp();
    long SearchDown();
    long SearchAll();
    void SeekCancel();

    /* Audio */
    void SetVolume(long volume);
    long GetVolume();
    long GetMaxVolume();
    void SetSpeakerOn(bool on);
    void SetRecordMode(int on);

    void SetBand(int band);
    void SetChannelSpacing(int spacing);
    void SetStereo();
    void SetMono();
    void MuteOn();
    void MuteOff();
    void setSoftmute(bool setSoftmute);
    void SetDeConstant(long constant);

    long GetCurrentRSSI();
    long GetCurrentSNR();
    void SetCurrentRSSI(long rssi);
    long GetSeekMode();
    void SetSeekMode(long seek_mode);
    int GetChannelSpacing();
    void SetSoftStereoBlendCoeff(long value);
    void SetSoftMuteCoeff(long value);

    void EnableRDS();
    void DisableRDS();
    void EnableDNS();
    void DisableDNS();
    void EnableAF();
    void DisableAF();
    void CancelAfSwitchingProcess();
    void SetAF_th(int freq);
    int GetAF_th();
    void SetAFValid_th(int freq);
    int GetAFValid_th();

    /* Not Support */
    void SetSeekRSSI(long freq);
    void SetSeekSNR(long freq);
    void SetRSSI_th(int threshold);
    void SetSNR_th(int threshold);
    void SetCnt_th(int threshold);
    int GetRSSI_th();
    int GetSNR_th();
    int GetCnt_th();
    void setScanning(bool value);
};

#endif /* __FM_RADIO_CONTROLLER_SLSI_H__ */
