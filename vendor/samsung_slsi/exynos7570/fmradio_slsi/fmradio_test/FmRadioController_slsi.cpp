#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <utils/Log.h>
//#include <linux/videodev2.h>

#include "FmRadioController_slsi.h"

/*******************************************************************************
 *
 * Global variables
 *
 ******************************************************************************/

long channel_limit_low;
long channel_limit_high;

/*******************************************************************************
 *
 * Static functions to access device driver of kernel
 *
 ******************************************************************************/

static int fm_radio_check_capability(int fd)
{
    struct v4l2_capability cap;
    int ret;

    ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0) {
        ALOGE("FmRadioController: failed to check capability\n");
        return FM_FAILURE;
    }

    if (!(cap.capabilities & V4L2_CAP_RADIO)) {
        ALOGE("FmRadioController: this device does not support Radio function\n");
	return FM_FAILURE;
    }

    ALOGI("FmRadioController: driver      : %s\n", cap.driver);
    ALOGI("FmRadioController: card        : %s\n", cap.card);
    ALOGI("FmRadioController: bus_info    : %s\n", cap.bus_info);
    ALOGI("FmRadioController: capabilities: %x\n", cap.capabilities);

    return FM_SUCCESS;
}

static int fm_radio_get_tuner(int fd)
{
    struct v4l2_tuner tuner;
    int ret;

    tuner.index = 0;

    ret = ioctl(fd, VIDIOC_G_TUNER, &tuner);
    if (ret < 0) {
        ALOGE("FmRadioController: failed to get tuner\n");
	return FM_FAILURE;
    }

    return FM_SUCCESS;
}

static int fm_radio_set_tuner(int fd, unsigned int mode)
{
    struct v4l2_tuner tuner;
    int ret;

    tuner.index = 0;
    tuner.audmode = mode;

    ret = ioctl(fd, VIDIOC_S_TUNER, &tuner);
    if (ret < 0) {
        ALOGE("FmRadioController: failed to set tuner\n");
	return FM_FAILURE;
    }

    return FM_SUCCESS;
}

static int fm_radio_get_frequency(int fd, long *channel)
{
    struct v4l2_frequency freq;
    int ret;

    freq.tuner = 0;
    freq.type = V4L2_TUNER_RADIO;

    ret = ioctl(fd, VIDIOC_G_FREQUENCY, &freq);
    if (ret < 0) {
        ALOGE("FmRadioController: failed to get frequency\n");
	return FM_FAILURE;
    }

    *channel = (long)freq.frequency/16000;

    return FM_SUCCESS;
}

static int fm_radio_set_frequency(int fd, long channel)
{
    struct v4l2_frequency freq;
    int ret;

    freq.tuner = 0;
    freq.type = V4L2_TUNER_RADIO;
    freq.frequency = (unsigned int)channel*16000;

    ret = ioctl(fd, VIDIOC_S_FREQUENCY, &freq);
    if (ret < 0) {
        ALOGE("FmRadioController: failed to set frequency\n");
	return FM_FAILURE;
    }

    return FM_SUCCESS;
}

static int fm_radio_seek_frequency(int fd, unsigned int upward, unsigned int wrap_around, unsigned int spacing)
{
    struct v4l2_hw_freq_seek seek;
    int ret;

    seek.tuner = 0;
    seek.type = V4L2_TUNER_RADIO;
    seek.seek_upward = upward;
    seek.wrap_around = wrap_around;
    seek.spacing = spacing;

    ret = ioctl(fd, VIDIOC_S_HW_FREQ_SEEK, &seek);
    if (ret < 0) {
        ALOGE("FmRadioController: failed to seek frequency\n");
	return FM_FAILURE;
    }

    return FM_SUCCESS;
}

static int fm_radio_get_control(int fd, unsigned int id, long *val)
{
    struct v4l2_control ctrl;
    int ret;

    ALOGD("FmRadioController:fm_radio_get_control: id(%d)\n", id);

    ctrl.id = id;

    ret = ioctl(fd, VIDIOC_G_CTRL, &ctrl);
    if (ret < 0) {
        ALOGE("FmRadioController: failed to set control\n");
	return FM_FAILURE;
    }

    *val = (long)ctrl.value;

    return FM_SUCCESS;
}

static int fm_radio_set_control(int fd, unsigned int id, long val)
{
    struct v4l2_control ctrl;
    int ret;

    ALOGD("FmRadioController:fm_radio_set_control: id(%d) val(%d)\n", id, val);

    ctrl.id = id;
    if (val)
        ctrl.value = (unsigned int)val;
    else
        ctrl.value = 0;

    ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0) {
        ALOGE("FmRadioController: failed to set control\n");
	return FM_FAILURE;
    }

    return FM_SUCCESS;
}

/*
 ******************************************************************************
 */
static int fm_radio_check_state(int *state)
{
    if (*state != FM_RADIO_ON) {
        ALOGE("FmRadioController: state is not ON\n");
        return FM_FAILURE;
    }

    *state = FM_RADIO_WORKING;

    return FM_SUCCESS;
}

static int fm_radio_channel_searching(int fd, unsigned int upward, unsigned int wrap_around, unsigned int spacing, long *channel)
{
    int ret;

    ret = fm_radio_set_control(fd, V4L2_CID_S610_SEEK_MODE, FM_RADIO_AUTONOMOUS_SEARCH_MODE_SKIP);
    if (ret < 0)
        return ret;

    ret = fm_radio_seek_frequency(fd, upward, wrap_around, spacing);
    if (ret < 0)
	return ret;

    ret = fm_radio_get_tuner(fd);
    if (ret < 0)
	return ret;

    ret = fm_radio_get_frequency(fd, channel);
    if (ret < 0)
	return ret;

    return ret;
}

/*******************************************************************************
 *
 * Public functions
 *
 ******************************************************************************/

/*
 * FM Radio Controller: FmRadioController_slsi
 * Input : none
 * Output: none
 */
FmRadioController_slsi::FmRadioController_slsi()
{
    ALOGI("FmRadioController\n");

    radio_state = FM_RADIO_OFF;
    radio_fd = -1;

    is_mute_on = FM_RADIO_MUTE_OFF;
    tuner_mode = FM_RADIO_TUNER_MONO;
    channel_spacing = FM_RADIO_SPACING_100KHZ;
    current_band = 0;
    channel_limit_low = 87500;
    channel_limit_high = 108000;
    current_volume = -1;
    is_seeking = false;

    is_enabled_functions = 0;
    is_af_switching = false;
    radio_thread_termination = false;
    radio_rds_thread = (pthread_t)NULL;
    radio_af_thread = (pthread_t)NULL;

    current_pi = 0;
    curr_channel = 0;
    af_threshold = -80;
    afvalid_threshold = -75;
    cancel_af_switching = false;
}

/*
 * FM Radio Controller: ~FmRadioController_slsi
 * Input : none
 * Output: none
 */
FmRadioController_slsi::~FmRadioController_slsi()
{
    ALOGI("FmRadioController: power off FM Radio\n");

    radio_state = FM_RADIO_STOP;

    SeekCancel();
    DisableRDS();
    DisableDNS();
    DisableAF();

    close (radio_fd);
    radio_fd = -1;

    radio_state = FM_RADIO_OFF;
}

/*
 * FM Radio Controller: Initialise
 * Input : none
 * Output: (int)
 *         - FM_FAILURE (-1): if any error occurred
 *         - FM_SUCCESS (0) : success to complete
 */
int FmRadioController_slsi::Initialise()
{
    int ret;

    ALOGI("FmRadioController::Initialise\n");

    if (radio_state != FM_RADIO_OFF) {
	ALOGE("FmRadioController::Initialise: FmRadio is already working\n");
	return FM_FAILURE;
    }

    radio_fd = open(FM_RADIO_DEVICE, O_RDWR);
    if (radio_fd < 0) {
        ALOGE("FmRadioController::Initialise: cannot open radio dev\n");
        return FM_FAILURE;
    }

    ALOGI("FmRadioController::Initialise: radio dev[%d] ON\n", radio_fd);
    radio_state = FM_RADIO_ON;

    ret = fm_radio_check_capability(radio_fd);
    if (ret != FM_SUCCESS) {
	close(radio_fd);
	radio_fd = -1;
        radio_state = FM_RADIO_OFF;
	return ret;
    }

    return ret;
}

/*
 * FM Radio Controller: TuneChannel
 * Input : (long)
 *         - channel (KHz)
 * Output: none
 */
void FmRadioController_slsi::TuneChannel(long channel)
{
    int ret;

    ALOGI("FmRadioController::TuneChannel: %d\n", channel);

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    if (channel < channel_limit_low || channel > channel_limit_high) {
        ALOGE("FmRadioController::TuneChannel: out of bound [%.1f - %.1f]Mhz\n",
                (float)channel_limit_low, (float)channel_limit_high);
        goto fail;
    }

    CancelAfSwitchingProcess();

    ret = fm_radio_get_tuner(radio_fd);
    if (ret < 0)
        goto fail;

    ret = fm_radio_set_frequency(radio_fd, channel);
    if (ret < 0)
        goto fail;

#if !defined(FM_RADIO_TEST_MODE)
    RDSParser.ResetData();
#endif

    ALOGI("FmRadioController::TuneChannel: set channel: %.1fMHz\n", (float)channel/1000);

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*
 * FM Radio Controller: GetChannel
 * Input : none
 * Output: (long)
 *         - channel (KHz)
 *         - FM_FAILURE (-1): if any error occurred
 */
long FmRadioController_slsi::GetChannel()
{
    long channel;
    int ret;

    ALOGI("FmRadioController::GetChannel\n");

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return ret;

    ret = fm_radio_get_frequency(radio_fd, &channel);
    if (ret < 0) {
        channel = (long)ret;
	goto done;
    }

    ALOGI("FmRadioController::GetChannel: current Channel: %.1fMHz\n", (float)channel/1000);

done:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;

    return channel;
}

/*
 * FM Radio Controller: Seek(Up/Down), Search(Up/Down/All)
 * Input : none
 * Output: (long)
 *         - channel (KHz)
 *         - FM_FAILURE (-1): if any error occurred
 */
long FmRadioController_slsi::SeekUp()
{
    long channel;
    unsigned int upward, wrap_around, spacing;
    int ret;

    ALOGI("FmRadioController::SeekUp\n");

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return ret;

    is_seeking = true;

    CancelAfSwitchingProcess();

    upward = FM_RADIO_SEEK_UP;
    wrap_around = true;
    spacing = (unsigned int)channel_spacing*10000;

    ret = fm_radio_channel_searching(radio_fd, upward, wrap_around, spacing, &channel);
    if (ret < 0) {
        channel = (long)ret;
	goto done;
    }

#if !defined(FM_RADIO_TEST_MODE)
    RDSParser.ResetData();
#endif

    ALOGI("FmRadioController::SeekUp: current Channel: %1.fMHz\n", (float)channel/1000);

done:
    is_seeking = false;

    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;

    return channel;
}

long FmRadioController_slsi::SeekDown()
{
    long channel;
    unsigned int upward, wrap_around, spacing;
    int ret;

    ALOGI("FmRadioController::SeekDown\n");

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return ret;

    is_seeking = true;

    CancelAfSwitchingProcess();

    upward = FM_RADIO_SEEK_DOWN;
    wrap_around = true;
    spacing = (unsigned int)channel_spacing*10000;

    ret = fm_radio_channel_searching(radio_fd, upward, wrap_around, spacing, &channel);
    if (ret < 0) {
        channel = (long)ret;
	goto done;
    }

#if !defined(FM_RADIO_TEST_MODE)
    RDSParser.ResetData();
#endif

    ALOGI("FmRadioController::SeekDown: current Channel: %1.fMHz\n", (float)channel/1000);

done:
    is_seeking = false;

    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;

    return channel;
}

long FmRadioController_slsi::SearchUp()
{
    ALOGI("FmRadioController::SearchUp\n");
    return SeekUp();
}

long FmRadioController_slsi::SearchDown()
{
    ALOGI("FmRadioController::SearchDown\n");
    return SeekDown();
}

long FmRadioController_slsi::SearchAll()
{
    long channel;
    unsigned int upward, wrap_around, spacing;
    int ret;

    ALOGI("FmRadioController::SearchAll\n");

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return ret;

    is_seeking = true;

    CancelAfSwitchingProcess();

    upward = FM_RADIO_SEEK_UP;
    wrap_around = false;
    spacing = (unsigned int)channel_spacing*10000;

    ret = fm_radio_channel_searching(radio_fd, upward, wrap_around, spacing, &channel);
    if (ret < 0) {
        channel = (long)ret;
	goto done;
    }

#if !defined(FM_RADIO_TEST_MODE)
    RDSParser.ResetData();
#endif

    ALOGI("FmRadioController::SearchAll: current Channel: %1.fMHz\n", (float)channel/1000);

done:
    is_seeking = false;

    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;

    return channel;
}

/*
 * FM Radio Controller: SeekCancel
 * Input : none
 * Output: none
 */
void FmRadioController_slsi::SeekCancel()
{
    int ret;

    ALOGI("FmRadioController::SeekCancel\n");

    if (radio_state == FM_RADIO_OFF || (radio_state == FM_RADIO_WORKING && !is_seeking)) {
        ALOGE("FmRadioController: radio state fail\n");
        return;
    }

    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_WORKING;

    ret = fm_radio_set_control(radio_fd, V4L2_CID_S610_SEEK_CANCEL, 1);
    if (ret < 0)
        goto fail;

    ALOGI("FmRadioController::SeekCancel done\n");

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*******************************************************************************
 *
 * FM Radio functions about AUDIO
 * FM Radio HAL does not support these functions.
 *
 * SetVolume, GetVolume, GetMaxVolume, SetSpeakerOn, SetRecordMode
 *
 */
void FmRadioController_slsi::SetVolume(long volume)
{
    int ret;

    ALOGI("FmRadioController::SetVolume: %d\n", volume);
    current_volume = volume;
}

long FmRadioController_slsi::GetVolume()
{
    ALOGI("FmRadioController::GetVolume: %d\n", current_volume);
    return current_volume;
}

long FmRadioController_slsi::GetMaxVolume()
{
    ALOGI("FmRadioController::GetMaxVolume\n");
    return -1;
}

void FmRadioController_slsi::SetSpeakerOn(bool on)
{
    ALOGI("FmRadioController::SetSpeakerOn: %d\n", on);
}

void FmRadioController_slsi::SetRecordMode(int on)
{
    ALOGI("FmRadioController::SetRecordMode: %d\n", on);
}

/*******************************************************************************
 * FM Radio Controller: SetBand
 * Input : (int) band
 *         - 1 : 87.5 - 108 MHz
 *         - 2 : 76.0 - 108 MHz
 *         - 3 : 76.0 -  90 MHz (Not supported)
 * Output: none
 */
void FmRadioController_slsi::SetBand(int input_band)
{
    int band;
    int ret;

    ALOGI("FmRadioController::SetBand: %d\n", input_band);

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    if (input_band == 1) {
        band = FM_RADIO_BAND_EUR;
    } else if (input_band == 3) {
        band = FM_RADIO_BAND_JAP;
    } else { /* input_band:2 76000_108000 */
        ALOGE("FmRadioController::SetBand: This band is not supported\n");
	goto fail;
    }

    ret = fm_radio_set_control(radio_fd, V4L2_CID_S610_CH_BAND, (long)band);
    if (ret < 0)
        goto fail;

    current_band = band;
    if (band) {
        channel_limit_low  = 76000;
        channel_limit_high = 90000;
    } else {
        channel_limit_low  = 87500;
        channel_limit_high = 108000;
    }
    ALOGI("FmRadioController::SetBand: current Band: %d\n", band);
    ALOGI("FmRadioController::Band [0] : European/U.S. (87.5 - 108 MHz)\n");
    ALOGI("FmRadioController::Band [1] : Japanese      (76   - 90  MHz)\n");

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*
 * FM Radio Controller: SetChannelSpacing
 * Input : (int)
 *         - spacing (unit:10KHz)
 * Output: none
 */
void FmRadioController_slsi::SetChannelSpacing(int spacing)
{
    long index;
    int ret;

    ALOGI("FmRadioController::SetChannelSpacing: %d\n", spacing);

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    switch (spacing) {
        case FM_RADIO_SPACING_50KHZ:
            index = 0;
	    break;
        case FM_RADIO_SPACING_100KHZ:
            index = 1;
	    break;
        case FM_RADIO_SPACING_200KHZ:
        default:
            index = 2;
	    break;
    };

    ret = fm_radio_set_control(radio_fd, V4L2_CID_S610_CH_SPACING, index);
    if (ret < 0)
        goto fail;

    channel_spacing = spacing;
    ALOGI("FmRadioController::SetChannelSpacing: %dKHz\n", spacing*10);

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*
 * FM Radio Controller: SetStereo
 * Input : none
 * Output: none
 */
void FmRadioController_slsi::SetStereo()
{
    int ret;

    ALOGI("FmRadioController::SetStereo\n");

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    ret = fm_radio_set_tuner(radio_fd, V4L2_TUNER_MODE_STEREO);
    if (ret < 0)
        goto fail;

    tuner_mode = FM_RADIO_TUNER_STEREO;
    ALOGI("FmRadioController::SetStereo: Success\n");

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*
 * FM Radio Controller: SetMono
 * Input : none
 * Output: none
 */
void FmRadioController_slsi::SetMono()
{
    int ret;

    ALOGI("FmRadioController::SetMono\n");

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    ret = fm_radio_set_tuner(radio_fd, V4L2_TUNER_MODE_MONO);
    if (ret < 0)
        goto fail;

    tuner_mode = FM_RADIO_TUNER_MONO;
    ALOGI("FmRadioController::SetMono: Success\n");

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*
 * FM Radio Controller: MuteOn
 * Input : none
 * Output: none
 */
void FmRadioController_slsi::MuteOn()
{
    int ret;

    ALOGI("FmRadioController::MuteOn\n");

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    ret = fm_radio_set_control(radio_fd, V4L2_CID_AUDIO_MUTE, FM_RADIO_MUTE_ON);
    if (ret < 0)
        goto fail;

    /* FM_RADIO_MUTE_ON: 0 */
    is_mute_on = FM_RADIO_MUTE_ON;
    ALOGI("FmRadioController::MuteOn: Success\n");

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*
 * FM Radio Controller: MuteOff
 * Input : none
 * Output: none
 */
void FmRadioController_slsi::MuteOff()
{
    int ret;

    ALOGI("FmRadioController::MuteOff\n");

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    ret = fm_radio_set_control(radio_fd, V4L2_CID_AUDIO_MUTE, FM_RADIO_MUTE_OFF);
    if (ret < 0)
        goto fail;

    /* FM_RADIO_MUTE_OFF: 1 */
    is_mute_on = FM_RADIO_MUTE_OFF;
    ALOGI("FmRadioController::MuteOff: Success\n");

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*
 * FM Radio Controller: setSoftmute
 * Input : none
 * Output: none
 */
void FmRadioController_slsi::setSoftmute(bool setSoftmute)
{
    int ret;

    ALOGI("FmRadioController::setSoftmute: %d\n", setSoftmute);

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    ret = fm_radio_set_control(radio_fd, V4L2_CID_S610_SOFT_STEREO_BLEND, setSoftmute);
    if (ret < 0)
        goto fail;

    ALOGI("FmRadioController::setSoftmute: set[%d] success\n", setSoftmute);

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*
 * FM Radio Controller: SetDeConstant
 * Input : none
 * Output: none
 */
void FmRadioController_slsi::SetDeConstant(long DeConstant)
{
    long val_deconstant;
    int ret;

    ALOGI("FmRadioController::SetDeConstant: %d\n", DeConstant);

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    switch (DeConstant) {
        case 0:
            val_deconstant = FM_RADIO_DE_TIME_CONSTANT_75;
	    break;
        case 2:
	    val_deconstant = FM_RADIO_DE_TIME_CONSTANT_0;
	    break;
        default:
            ALOGE("FmRadioController::SetDeConstant: [%d] is not supported\n", DeConstant);
            goto fail;
    }

    ret = fm_radio_set_control(radio_fd, V4L2_CID_TUNE_DEEMPHASIS, val_deconstant);
    if (ret < 0)
        goto fail;

    ALOGI("FmRadioController::setDeConstant: set[%d] success\n", val_deconstant);

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*******************************************************************************
 * FM Radio Controller: GetCurrentRSSI
 * Input : none
 * Output: (long)
 *         - Current RSSI (-128 ~ 127)
 */
long FmRadioController_slsi::GetCurrentRSSI()
{
    long value;
    int ret;

    ALOGI("FmRadioController::GetCurrentRSSI\n");

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return ret;

    ret = fm_radio_get_control(radio_fd, V4L2_CID_S610_RSSI_CURR, &value);
    if (ret < 0) {
        value = (long)ret;
        goto done;
    }

    if (value > 127)
        value |= 0xFFFFFF00;

    ALOGI("FmRadioController::GetCurrentRSSI: %d\n", value);

done:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;

    return value;
}

/*
 * FM Radio Controller: GetCurrentSNR
 * Input : none
 * Output: (long)
 *         - Current SNR
 */
long FmRadioController_slsi::GetCurrentSNR()
{
    long value;
    int ret;

    ALOGI("FmRadioController::GetCurrentSNR\n");

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return ret;

    ret = fm_radio_get_control(radio_fd, V4L2_CID_S610_SNR_CURR, &value);
    if (ret < 0) {
        value = (long)ret;
        goto done;
    }

    ALOGI("FmRadioController::GetCurrentSNR: %d\n", value);

done:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;

    return value;
}

/*
 * FM Radio Controller: SetCurrentRSSI
 * Input : (long) Current RSSI (-128 ~ 127)
 * Output: none
 */
void FmRadioController_slsi::SetCurrentRSSI(long rssi)
{
    unsigned char crssi;
    int ret;

    ALOGI("FmRadioController::SetCurrentRSSI: %d\n", rssi);

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    crssi = (unsigned char)(rssi & 0xFF);
    rssi  = (long)crssi;

    ret = fm_radio_set_control(radio_fd, V4L2_CID_S610_RSSI_CURR, rssi);
    if (ret < 0)
        goto fail;

    ALOGI("FmRadioController::SetCurrentRSSI: set[%d] success\n", rssi);

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*
 * FM Radio Controller: GetSeekMode
 * input : none
 * output: (long) Seek Mode
 *         - 0 : STOP SEARCH MODE
 *         - 1 : PRESET MODE
 *         - 2 : AUTONOMOUS SEARCH MODE
 *         - 3 : AF JUMP MODE
 *         - 4 : AUTONOMOUS SEARCH MODE SKIP
 */
long FmRadioController_slsi::GetSeekMode()
{
    long seek_mode;
    int ret;

    ALOGI("FmRadioController::GetSeekMode\n");

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return ret;

    ret = fm_radio_get_control(radio_fd, V4L2_CID_S610_SEEK_MODE, &seek_mode);
    if (ret < 0) {
        seek_mode = (long)ret;
        goto done;
    }

    ALOGI("FmRadioController::GetSeekMode: %d\n", seek_mode);

done:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;

    return seek_mode;
}

/*
 * FM Radio Controller: SetSeekMode
 * input : (long) Seek Mode
 * output: none
 */
void FmRadioController_slsi::SetSeekMode(long seek_mode)
{
    int ret;

    ALOGI("FmRadioController::SetSeekMode %d\n", seek_mode);

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    ret = fm_radio_set_control(radio_fd, V4L2_CID_S610_SEEK_MODE, seek_mode);
    if (ret < 0)
        goto fail;

    ALOGI("FmRadioController::SetSeekMode: set[%d] success\n", seek_mode);

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*
 * FM Radio Controller: GetChannelSpacing
 * Input : none
 * Output: (int) spacing (unit:10KHz)
 */
int FmRadioController_slsi::GetChannelSpacing()
{
    long index;
    int ret;

    ALOGI("FmRadioController::GetChannelSpacing\n");

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return ret;

    ret = fm_radio_get_control(radio_fd, V4L2_CID_S610_CH_SPACING, &index);
    if (ret < 0)
        goto fail;

    switch (index) {
        case 0:
            ret = FM_RADIO_SPACING_50KHZ;
            break;
        case 1:
            ret = FM_RADIO_SPACING_100KHZ;
            break;
        case 2:
            ret = FM_RADIO_SPACING_200KHZ;
            break;
        default:
            ALOGE("FmRadioController: ChannelSpacing is wrong\n");
	    ret = FM_FAILURE;
            goto fail;
    }

    ALOGI("FmRadioController: ChannelSpacing: current[%d], get[%d] KHz\n", channel_spacing*10, ret*10);

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;

    return ret;
}

void FmRadioController_slsi::SetSoftStereoBlendCoeff(long value)
{
    int ret;

    ALOGI("FmRadioController::SetSoftStereoBlendCoeff: %ld\n", value);

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    ret = fm_radio_set_control(radio_fd, V4L2_CID_S610_SOFT_STEREO_BLEND_COEFF, value);
    if (ret < 0)
        goto fail;

    ALOGI("FmRadioController::SetSoftStereoBlendCoeff: set[%ld] success\n", value);

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

void FmRadioController_slsi::SetSoftMuteCoeff(long value)
{
    int ret;

    ALOGI("FmRadioController::SetSoftMuteCoeff: %ld\n", value);

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    ret = fm_radio_set_control(radio_fd, V4L2_CID_S610_SOFT_MUTE_COEFF, value);
    if (ret < 0)
        goto fail;

    ALOGI("FmRadioController::SetSoftMuteCoeff: set[%ld] success\n", value);

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*******************************************************************************
 *
 * Functions about RDS, AF, and DNS
 *
 ******************************************************************************/

static int fm_radio_poll(int fd, struct pollfd *poll_fd)
{
    int ret;

    poll_fd->fd = fd;
    poll_fd->events = POLLIN;
    poll_fd->revents = 0;

    ret = poll(poll_fd, 1, 360);
    if (ret > 0) {
        if (poll_fd->revents & POLLIN) {
            ALOGI("FmRadioController: ready to read\n");
	    return FM_SUCCESS;
	}

	ALOGI("FmRadioController: cannot read yet\n");
	return FM_FAILURE;
    }

    if (!ret) {
        ALOGE("FmRadioController: polling timeout\n");
	return FM_FAILURE;
    }

    ALOGE("FmRadioController: pollig fail: %d\n", ret);
    return FM_FAILURE - 1;
}

static int fm_radio_read(int fd, unsigned char *buf)
{
    int ret;

    ret = read(fd, buf, FM_RADIO_RDS_DATA_MAX);
    if (ret < 0) {
        ALOGE("FmRadioController: failed to read\n");
	return FM_FAILURE;
    }

    return ret;
}

#if defined(FM_RADIO_TEST_MODE)
static void fm_radio_data_parsing(unsigned char *buf, radio_data_t *r_data);
#else
static void fm_radio_tune(int fd, int *state, long channel)
{
    int ret;

    ALOGI("FmRadioController:fm_radio_tune: %ld\n", channel);

    ret = fm_radio_check_state(state);
    if (ret < 0)
        return;

    if (channel < channel_limit_low || channel > channel_limit_high) {
        ALOGE("FmRadioController:fm_radio_tune: out of bound [%.1f - %.1f]Mhz\n",
                (float)channel_limit_low/1000, (float)channel_limit_high/1000);
	goto fail;
    }

    ret = fm_radio_get_tuner(fd);
    if (ret < 0)
        goto fail;

    ret = fm_radio_set_frequency(fd, channel);
    if (ret < 0)
        goto fail;

    ALOGI("FmRadioController:fm_radio_tune: %.1fMHz\n", (float)channel/1000);

fail:
    if (*state != FM_RADIO_STOP)
        *state = FM_RADIO_ON;
}

int FmRadioController_slsi::af_signal_check(AF_List *af_list)
{
    long new_freq = 0;
    int ret = 0;
    int i, j;
    unsigned char u8_af_threshold = (unsigned char)(af_threshold & 0xFF);
    unsigned char temp;

    for (i = 0; i < af_list->AFCount; i++) {
        if (cancel_af_switching) {
            ALOGD("FmRadioController: Cancel AF signal check\n");
            return 0;
        }

        new_freq = AF_CODE_TO_FM_FREQ(af_list->AF[i]);
        fm_radio_tune(radio_fd, &radio_state, new_freq);
        usleep(20000);
        af_list->RSSI[i] = GetCurrentRSSI();

        ALOGD("FmRadioController_slsi: af_signal_check: channel:%dKHz, rssi:%d, af_threshold:%d(%d)\n",
                        new_freq, af_list->RSSI[i], af_threshold, u8_af_threshold);

        if (af_list->RSSI[i] >= u8_af_threshold)
            ret = 1;
    }

    if (!ret) {
        ALOGD("FmRadioController_slsi: af_signal_check: all rssi is lower than af_threshold\n");
        return ret;
    }

    for (i = 0; i < (af_list->AFCount-1); i++) {
        for (j = i+1; j < af_list->AFCount; j++) {
            if (af_list->RSSI[j] > af_list->RSSI[i]) {
                temp = af_list->RSSI[i];
                af_list->RSSI[i] = af_list->RSSI[j];
                af_list->RSSI[j] = temp;
            }
        }
    }

    ALOGD("FmRadioController_slsi: af_signal_check: the highest rssi:%d\n", af_list->RSSI[0]);

    return ret;
}

void* FmRadioController_slsi::radio_af_thread_handler(void *arg)
{
    FmRadioController_slsi *radio_obj = static_cast<FmRadioController_slsi*>(arg);
    long new_freq = 0;
    int af_found = 0;
    int pi = 0;
    int i;

    ALOGD("FmRadioController: radio_af_thread_handler\n");

    if (!radio_obj->af_signal_check(&radio_obj->AFList))
        goto exit;

    if (radio_obj->cancel_af_switching)
        goto exit;
    else
        CallbackAFStarted();

    ALOGD("FmRadioController: start AF handle thread: afvalid_threshold:%d\n", radio_obj->afvalid_threshold);

    for (i = 0; i < radio_obj->AFList.AFCount; i++) {
        if (radio_obj->cancel_af_switching) {
            ALOGD("FmRadioController: Cancel AF Switching\n");
            goto exit;
        }

        new_freq = AF_CODE_TO_FM_FREQ(radio_obj->AFList.AF[i]);
        fm_radio_tune(radio_obj->radio_fd, &radio_obj->radio_state, new_freq);
        usleep(20000);

        if (radio_obj->afvalid_threshold < radio_obj->GetCurrentRSSI()) {
            pi = radio_obj->RDSParser.GetPI();
            ALOGI("FmRadioController: AF handle thread: PI(%d:%d)\n", pi, radio_obj->current_pi);
            if (pi == radio_obj->current_pi) {
                AFDataReceived(new_freq);
                ALOGI("FmRadioController: AF handle thread: found new freq:%ld\n", new_freq);
                goto found;
            }
        }
    }

    ALOGI("FmRadioController: AF handle thread: could not found valid freq\n");

exit:
    new_freq = radio_obj->curr_channel;
    fm_radio_tune(radio_obj->radio_fd, &radio_obj->radio_state, new_freq);

found:
    radio_obj->is_af_switching = false;
    radio_obj->cancel_af_switching = false;

    ALOGI("FmRadioController: complete AF handle thread\n");

    pthread_exit(NULL);
    return NULL;
}

void FmRadioController_slsi::radio_af_switching()
{
    long c_rssi;
    int ret;

    ALOGI("FmRadioController: start AF Switching\n");

    if (is_af_switching) {
        ALOGE("FmRadioController: AF Switching is processing\n");
        return;
    }

    is_af_switching = true;

    c_rssi = GetCurrentRSSI();
    if (c_rssi < af_threshold) {
        current_pi = RDSParser.GetPI();
        curr_channel = GetChannel();

        ALOGD("FmRadioController: AF Switching: pi:%d, channel:%d\n", current_pi, curr_channel);

        AFList = RDSParser.GetAFList(curr_channel);

        if (AFList.AFCount > 0) {
            if (radio_af_thread) {
                ALOGI("FmRadioController: AF Switching: wrong state\n");
                cancel_af_switching = true;
                pthread_join(radio_af_thread, NULL);
                radio_af_thread = (pthread_t)NULL;
                cancel_af_switching = false;
                is_af_switching = false;
                return;
            }

            ret = pthread_create(&radio_af_thread, NULL, radio_af_thread_handler, this);
            if (ret < 0) {
                ALOGE("FmRadioController: failed to create af thread\n");
            } else {
                ALOGI("FmRadioController: AF Switching: start af thread\n");
                return;
            }
        }
    }

    is_af_switching = false;
}
#endif

void FmRadioController_slsi::radio_data_proc(unsigned char *buf, int count)
{
    radio_data_t r_data;
    struct PIECC_data piecc_data;
    struct Final_RDS_data final_rds_data;
    struct RTPlus_data rtplus_data;
    unsigned short checkFlag = 0;
    int i;

    memset(&r_data, 0, sizeof(radio_data_t));

    for (i = 0; i < count; i += FM_RADIO_RDS_SET_NUM) {
        r_data.rdsa  = ((buf[i+ 1] << 8) | buf[i]);
        r_data.rdsb  = ((buf[i+ 4] << 8) | buf[i+3]);
        r_data.rdsc  = ((buf[i+ 7] << 8) | buf[i+6]);
        r_data.rdsd  = ((buf[i+10] << 8) | buf[i+9]);

        r_data.blera = ((buf[i+ 2] & FM_RADIO_RDS_BLER_MASK) >> 3);
        r_data.blerb = ((buf[i+ 5] & FM_RADIO_RDS_BLER_MASK) >> 3);
        r_data.blerc = ((buf[i+ 8] & FM_RADIO_RDS_BLER_MASK) >> 3);
        r_data.blerd = ((buf[i+11] & FM_RADIO_RDS_BLER_MASK) >> 3);

	memset(&piecc_data, 0, sizeof(PIECC_data));
	memset(&final_rds_data, 0, sizeof(Final_RDS_data));
	memset(&rtplus_data, 0, sizeof(RTPlus_data));

#if defined(FM_RADIO_TEST_MODE)
	fm_radio_data_parsing(&buf[i], &r_data);
#else
        checkFlag = RDSParser.ParseData(&r_data);

        if (is_enabled_functions & FM_RADIO_ENABLED_RDS) {
            if ((checkFlag & RT_FLAG) || (checkFlag & PS_FLAG)) {
                if (RDSParser.GetFinalRDSData(&final_rds_data)) {
                    RDSDataReceived(final_rds_data);
		}
	    }
            if (checkFlag & RTPLUS_FLAG) {
                if (RDSParser.GetFinalRTPlusData(&rtplus_data)) {
                    RTPlusDataReceived(rtplus_data);
                }
            }
        }

        if (is_enabled_functions & FM_RADIO_ENABLED_DNS) {
            if ((checkFlag & PI_FLAG) || (checkFlag & ECC_FLAG)) {
                if (RDSParser.GetPIECCData(&piecc_data)) {
                    PIECCDataReceived(piecc_data);
                }
            }
        }

        if (is_enabled_functions & FM_RADIO_ENABLED_AF) {
            checkFlag = RDSParser.ParseAFList(&r_data);
            radio_af_switching();
        }
#endif
    }
}

void* FmRadioController_slsi::radio_thread_handler(void *arg)
{
    FmRadioController_slsi *radio_obj = static_cast<FmRadioController_slsi*>(arg);
    struct pollfd radio_poll;
    unsigned char read_buf[FM_RADIO_RDS_DATA_MAX];
    int ret;

    ALOGI("FmRadioController: start radio_thread_handler\n");

#if defined(FM_RADIO_TEST_MODE)
    test_data_index = 0;
#endif
    while (!radio_obj->radio_thread_termination) {
        ret = fm_radio_poll(radio_obj->radio_fd, &radio_poll);
        if (ret < 0) {
            if (ret < FM_FAILURE)
                break;
            else
                continue;
        }

        ret = fm_radio_read(radio_obj->radio_fd, read_buf);
        if (ret < 0)
            break;

        radio_obj->radio_data_proc(read_buf, ret);
    }

    pthread_exit(NULL);

    return NULL;
}

int FmRadioController_slsi::startThreadforFmRadio(unsigned char func)
{
    int ret;

    if (radio_rds_thread) {
        if (!is_enabled_functions) {
            ALOGE("FmRadioController: radio_rds_thread state is abnormal\n");

            radio_thread_termination = true;
            pthread_join(radio_rds_thread, NULL);
            radio_rds_thread = (pthread_t)NULL;
        } else {
            ALOGI("FmRadioController: FM Radio thread is already working\n");

            is_enabled_functions |= func;
            return FM_SUCCESS;
        }
    }

    ret = fm_radio_set_control(radio_fd, V4L2_CID_S610_RDS_ON, FM_RADIO_ON);
    if (ret < 0)
        return FM_FAILURE;

    ALOGI("FmRadioController: enable RDS\n");

    is_enabled_functions |= func;

    radio_thread_termination = false;
    ret = pthread_create(&radio_rds_thread, NULL, radio_thread_handler, this);
    if (ret < 0) {
        ALOGE("FmRadioController: failed to create radio thread\n");
        radio_rds_thread = (pthread_t)NULL;
        return FM_FAILURE;
    }

    ALOGI("FmRadioController: thread creation success\n");

    is_enabled_functions |= func;

    return FM_SUCCESS;
}

int FmRadioController_slsi::stopThreadforFmRadio(unsigned char func)
{
    int ret;

    if (!radio_rds_thread) {
        ALOGE("FmRadioController: radio_rds_thread state is abnormal\n");
        goto rds_off;
    }

    if (is_enabled_functions & (~func)) {
        ALOGI("FmRadioController: FM Radio thread is still used\n");
	goto done;
    }

    radio_thread_termination = true;
    pthread_join(radio_rds_thread, NULL);
    radio_rds_thread = (pthread_t)NULL;

    ALOGI("FmRadioController: thread completion success\n");

rds_off:
    is_enabled_functions = 0;

    ret = fm_radio_set_control(radio_fd, V4L2_CID_S610_RDS_ON, FM_RADIO_OFF);
    if (ret < 0)
        return FM_FAILURE;

    ALOGI("FmRadioController: disable RDS\n");

    return FM_SUCCESS;

done:
    is_enabled_functions &= ~func;
    return FM_SUCCESS;
}

/******************************************************************************
 * FM Radio Controller: EnableRDS()
 * Input : none
 * Output: none
 */
void FmRadioController_slsi::EnableRDS()
{
    unsigned char func_mask = FM_RADIO_ENABLED_RDS;
    int ret;

    ALOGI("FmRadioController::EnableRDS\n");

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    if (is_enabled_functions & func_mask) {
        ALOGE("FmRadioController: RDS is already enabled\n");
        goto fail;
    }

    ret = startThreadforFmRadio(func_mask);
    if (ret < 0)
        goto fail;

    ALOGI("FmRadioController::EnableRDS: success\n");

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*
 * FM Radio Controller: DisableRDS()
 */
void FmRadioController_slsi::DisableRDS()
{
    unsigned char func_mask = FM_RADIO_ENABLED_RDS;
    int ret;

    ALOGI("FmRadioController::DisableRDS\n");

    if (radio_state != FM_RADIO_STOP && radio_state != FM_RADIO_ON) {
        ALOGE("FmRadioController::DisableRDS: state is not vaild!\n");
        return;
    }

    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_WORKING;

    if (!(is_enabled_functions & func_mask)) {
        ALOGE("FmRadioController: RDS is already disabled\n");
        goto fail;
    }

    ret = stopThreadforFmRadio(func_mask);
    if (ret < 0)
        goto fail;

    ALOGI("FmRadioController::DisableRDS: success\n");

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*
 * FM Radio Controller: EnableDNS()
 */
void FmRadioController_slsi::EnableDNS()
{
    unsigned char func_mask = FM_RADIO_ENABLED_DNS;
    int ret;

    ALOGI("FmRadioController::EnableDNS\n");

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    if (is_enabled_functions & func_mask) {
        ALOGE("FmRadioController: DNS is already enabled\n");
        goto fail;
    }

    ret = startThreadforFmRadio(func_mask);
    if (ret < 0)
        goto fail;

    ALOGI("FmRadioController::EnableDNS: success\n");

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*
 * FM Radio Controller: DisableDNS()
 */
void FmRadioController_slsi::DisableDNS()
{
    unsigned char func_mask = FM_RADIO_ENABLED_DNS;
    int ret;

    ALOGI("FmRadioController::DisableDNS\n");

    if (radio_state != FM_RADIO_STOP && radio_state != FM_RADIO_ON) {
        ALOGE("FmRadioController::DisableDNS: state is not vaild!\n");
        return;
    }

    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_WORKING;

    if (!(is_enabled_functions & func_mask)) {
        ALOGE("FmRadioController: DNS is already disabled\n");
        goto fail;
    }

    ret = stopThreadforFmRadio(func_mask);
    if (ret < 0)
        goto fail;

    ALOGI("FmRadioController::DisableDNS: success\n");

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*
 * FM Radio Controller: EnableAF()
 */
void FmRadioController_slsi::EnableAF()
{
    unsigned char func_mask = FM_RADIO_ENABLED_AF;
    int ret;

    ALOGI("FmRadioController::EnableAF\n");

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    if (is_enabled_functions & func_mask) {
        ALOGE("FmRadioController: AF is already enabled\n");
        goto fail;
    }

    ret = startThreadforFmRadio(func_mask);
    if (ret < 0)
        goto fail;

    ALOGI("FmRadioController::EnableAF: success\n");

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*
 * FM Radio Controller: DisableAF()
 */
void FmRadioController_slsi::DisableAF()
{
    unsigned char func_mask = FM_RADIO_ENABLED_AF;
    int ret;

    ALOGI("FmRadioController::DisableAF\n");

    if (radio_state != FM_RADIO_STOP && radio_state != FM_RADIO_ON) {
        ALOGE("FmRadioController::DisableAF: state is not vaild!\n");
        return;
    }

    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_WORKING;

    if (!(is_enabled_functions & func_mask)) {
        ALOGE("FmRadioController: AF is already disabled\n");
        goto fail;
    }

    ret = stopThreadforFmRadio(func_mask);
    if (ret < 0)
        goto fail;

    ALOGI("FmRadioController::DisableAF: success\n");

fail:
    CancelAfSwitchingProcess();

    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

/*
 * FM Radio Controller: CancelAfSwitchingProcess()
 */
void FmRadioController_slsi::CancelAfSwitchingProcess()
{
    ALOGI("FmRadioController::CancelAfSwitchingProcess\n");

    if (is_af_switching) {
        cancel_af_switching = true;

        if (radio_af_thread) {
            pthread_join(radio_af_thread, NULL);
            radio_af_thread = (pthread_t)NULL;
            ALOGI("FmRadioController::DisableAF: af thread was terminated\n");
        }
    }
}

void FmRadioController_slsi::SetRSSI_th(int threshold)
{
    int ret;

    ALOGI("FmRadioController::SetRSSI_th: threshold:%d\n", threshold);

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    ret = fm_radio_set_control(radio_fd, V4L2_CID_S610_RSSI_TH, (long)threshold);
    if (ret < 0)
        goto fail;

    ALOGI("FmRadioController::SetRSSI_th: set[%d] success\n", threshold);

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

int FmRadioController_slsi::GetRSSI_th()
{
    long value;
    int ret;

    ALOGI("FmRadioController::GetRSSI_th\n");

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return ret;

    ret = fm_radio_get_control(radio_fd, V4L2_CID_S610_RSSI_TH, &value);
    if (ret < 0)
        goto done;

    if (value > 127)
        value |= 0xFFFFFF00;

    ret = (int)value;

    ALOGI("FmRadioController::GetRSSI_th: %d\n", ret);

done:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;

    return ret;
}

void FmRadioController_slsi::SetSNR_th(int threshold)
{
    int ret;

    ALOGI("FmRadioController::SetSNR_th: threshold:%d\n", threshold);

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    ret = fm_radio_set_control(radio_fd, V4L2_CID_S610_IF_COUNT1, (long)threshold);
    if (ret < 0)
        goto fail;

    ALOGI("FmRadioController::SetSNR_th: set[%d] success\n", threshold);

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

int FmRadioController_slsi::GetSNR_th()
{
    long value;
    int ret;

    ALOGI("FmRadioController::GetSNR_th\n");

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return ret;

    ret = fm_radio_get_control(radio_fd, V4L2_CID_S610_IF_COUNT1, &value);
    if (ret < 0)
        goto done;

    ret = (int)value;

    ALOGI("FmRadioController::GetSNR_th: %d\n", ret);

done:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;

    return ret;
}

void FmRadioController_slsi::SetCnt_th(int threshold)
{
    int ret;

    ALOGI("FmRadioController::SetCnt_th: threshold:%d\n", threshold);

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return;

    ret = fm_radio_set_control(radio_fd, V4L2_CID_S610_IF_COUNT2, (long)threshold);
    if (ret < 0)
        goto fail;

    ALOGI("FmRadioController::SetCnt_th: set[%d] success\n", threshold);

fail:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;
}

int FmRadioController_slsi::GetCnt_th()
{
    long value;
    int ret;

    ALOGI("FmRadioController::GetCnt_th\n");

    ret = fm_radio_check_state(&radio_state);
    if (ret < 0)
        return ret;

    ret = fm_radio_get_control(radio_fd, V4L2_CID_S610_IF_COUNT2, &value);
    if (ret < 0)
        goto done;

    ret = (int)value;

    ALOGI("FmRadioController::GetCnt_th: %d\n", ret);

done:
    if (radio_state != FM_RADIO_STOP)
        radio_state = FM_RADIO_ON;

    return ret;
}

void FmRadioController_slsi::SetAF_th(int freq)
{
    ALOGI("FmRadioController::SetAF_th: freq:%d\n", freq);
    af_threshold = freq;
}

int FmRadioController_slsi::GetAF_th()
{
    ALOGI("FmRadioController::GetAF_th\n");
    return af_threshold;
}

void FmRadioController_slsi::SetAFValid_th(int freq)
{
    ALOGI("FmRadioController::SetAFValid_th: freq:%d\n", freq);
    afvalid_threshold = freq;
}

int FmRadioController_slsi::GetAFValid_th()
{
    ALOGI("FmRadioController::GetAFValid_th\n");
    return afvalid_threshold;
}

/*******************************************************************************
 *
 * Not supported functions
 *
 ******************************************************************************/
void FmRadioController_slsi::SetSeekRSSI(long freq)
{
    ALOGI("FmRadioController::SetSeekRSSI: freq:%d\n", freq);
}

void FmRadioController_slsi::SetSeekSNR(long freq)
{
    ALOGI("FmRadioController::SetSeekSNR: freq:%d\n", freq);
}

void FmRadioController_slsi::setScanning(bool value)
{
    ALOGI("FmRadioController::setScanning: %d\n", value);
}

/*******************************************************************************
 *
 * Functions for test
 *
 ******************************************************************************/

#if defined(FM_RADIO_TEST_MODE)
static int fm_radio_rds_valid_check(char* string, char len)
{
    int length = strlen(string);
    if (length < len)
        return 0;
    return 1;
}

static void fm_radio_data_parsing(unsigned char *buf, radio_data_t *r_data)
{
    unsigned char rds_group = (r_data->rdsb >> 11);
    unsigned char running;
    unsigned char content[2];
    unsigned char rds_data[4];
    unsigned char len = 0, index = 0;
    unsigned char rds_flag = 0;
    int i;

    memcpy(&test_read_data[test_data_index][0], buf, FM_RADIO_RDS_SET_NUM);
    memcpy(&test_radio_data[test_data_index], r_data, sizeof(radio_data_t));

    /* PI & ECC */
    test_piecc[test_data_index].PI  = r_data->rdsa;
    test_piecc[test_data_index].ECC = 0;
    if (rds_group == 2) {
        if (!((r_data->rdsc >> 12) & 0x7))
            test_piecc[test_data_index].ECC = (r_data->rdsc & 0xFF);
    }

    /* RTPLUS */
    test_rtplus[test_data_index_rtplus].bToggle  = 0;
    test_rtplus[test_data_index_rtplus].bRunning = 0;
    test_rtplus[test_data_index_rtplus].bValidated = 0;
    memset(&test_rtplus[test_data_index_rtplus].RTPlusTag[0], 0, sizeof(struct RTPlusTagInfo) * 2);

    if (rds_group == 10 || rds_group == 12 || rds_group == 14 ||
        rds_group == 16 || rds_group == 18 || rds_group == 22 ||
        rds_group == 24 || rds_group == 26 || rds_group == 30) {
        running = ((r_data->rdsb & 0x0008) >> 3);

	if (running) {
            test_rtplus[test_data_index_rtplus].bToggle  = ((r_data->rdsb & 0x0010) >> 4);
            test_rtplus[test_data_index_rtplus].bRunning = running;

            content[0] = ((r_data->rdsb & 0x0007) << 3) | ((r_data->rdsc & 0xE000) >> 13);
            if (content[0]) {
                test_rtplus[test_data_index_rtplus].RTPlusTag[0].contentType = (int)content[0];
                test_rtplus[test_data_index_rtplus].RTPlusTag[0].startPos = (int)((r_data->rdsc & 0x1F80) >> 7);
                test_rtplus[test_data_index_rtplus].RTPlusTag[0].additionalLen = (int)((r_data->rdsc &0x007E) >> 1);
            }

            content[1] = ((r_data->rdsc & 0x0001) << 5) | ((r_data->rdsd & 0xF800) >> 11);
            if (content[1]) {
                test_rtplus[test_data_index_rtplus].RTPlusTag[1].contentType = (int)content[1];
                test_rtplus[test_data_index_rtplus].RTPlusTag[1].startPos = (int)((r_data->rdsd &0x07E0) >> 5);
                test_rtplus[test_data_index_rtplus].RTPlusTag[1].additionalLen = (int)(r_data->rdsd &0x001F);
            }

            if (content[0] != 0 || content[1] != 0) {
                if (++test_data_index_rtplus == TEST_DATA_SIZE)
                    test_data_index_rtplus = 0;
            }
        }
    }

    /* RT and PS */
    memset(&rds_data[0], 0, sizeof(unsigned char) * 4);
    if (rds_group == 4 || rds_group == 5)
    {
        if (RT_Buffered.bChangeFlag != ((r_data->rdsb & 0x0010) >> 4))
        {
            memset(&RT_Buffered, 0, sizeof(RadioText));
	    RT_Buffered.bChangeFlag = (r_data->rdsb & 0x0010) >> 4;
	}

        if (rds_group == 4) {
            rds_data[0] = r_data->rdsc >> 8;
	    rds_data[1] = r_data->rdsc & 0xFF;
	    rds_data[2] = r_data->rdsd >> 8;
	    rds_data[3] = r_data->rdsd & 0xFF;
	    index = (r_data->rdsb & 0xF) * 4;
	    len = 4;
        } else {
            rds_data[0] = r_data->rdsd >> 8;
	    rds_data[1] = r_data->rdsd & 0xFF;
	    index = (r_data->rdsb & 0xF) * 2;
	    len = 2;
        }

        for (i = 0; i < len; i++) {
            if (rds_data[i] == 0x0d) {
                RT_Buffered.Text[i+index] = 0;
		RT_Buffered.iLenght = index + i;
		break;
            }
            RT_Buffered.iBytesReceived++;
	    RT_Buffered.Text[i+index] = rds_data[i];
        }

        if (strncmp(RT_Buffered.Text, "    ", 4) == 0)
        {
            RT_Buffered.Text[4] = 0;
            RT_Buffered.iLenght = 4;
        }

        if (RT_Buffered.iLenght <= index)
            RT_Buffered.iLenght = index+len;

        RT_Buffered.bValidated = fm_radio_rds_valid_check(RT_Buffered.Text, RT_Buffered.iLenght);

        if (RT_Buffered.bValidated && (RT_Buffered.iBytesReceived > RT_Buffered.iLenght ||
            RT_Buffered.iLenght == RT_MAXIMUM_SIZE)) {
	    if (strcmp(RT_Final.Text, RT_Buffered.Text) != 0) {
                RT_Final = RT_Buffered;
            } else {
                RT_Final.bValidated++;
            }
            RT_Buffered.iBytesReceived = 0;
            if ((RT_Final.bValidated == 1) || (RT_Final.bValidated % 10) == 0) {
                strcpy(final_rds_data[test_data_index_rds].RadioText, RT_Final.Text);
                rds_flag = 1;
            }
        }
    }

    memset(&rds_data[0], 0, sizeof(unsigned char) * 4);
    if (rds_group == 0 || rds_group == 1) {
        rds_data[0] = r_data->rdsd >> 8;
	rds_data[1] = r_data->rdsd & 0xFF;
	index = (r_data->rdsb & 0x3) * 2;
	len = 2;

	for (i = 0; i < len; i++) {
            if (PS_Buffered.Text[i+index] != rds_data[i])
                PS_Buffered.iBytesReceived ++;
            PS_Buffered.Text[i+index] = rds_data[i];
        }

        if (PS_Buffered.iLenght <= index)
            PS_Buffered.iLenght = index+len;

        PS_Buffered.bValidated = fm_radio_rds_valid_check(PS_Buffered.Text, PS_MAXIMUM_SIZE);
        if (PS_Buffered.bValidated && PS_Buffered.iBytesReceived >= PS_MAXIMUM_SIZE) {
            if (strcmp(PS_Final.Text, PS_Buffered.Text) != 0)
                PS_Final = PS_Buffered;
            else
                PS_Final.bValidated++;
            memset(&PS_Buffered, 0, sizeof(ServiceName));
            if ((PS_Final.bValidated == 2) || (PS_Final.bValidated % 10) == 0) {
                strcpy(final_rds_data[test_data_index_rds].StationName, PS_Final.Text);
                rds_flag = 1;
            }
        }
    }

    if (rds_flag) {
        if (++test_data_index_rds == 10)
            test_data_index_rds = 0;
    }

    if (++test_data_index == TEST_DATA_SIZE)
        test_data_index = 0;
}
#endif

