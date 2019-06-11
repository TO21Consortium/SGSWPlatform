#include <cstring>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/types.h>

#include "FmRadioController_slsi.h"

/*******************************************************************************
 * Global Variables
 ******************************************************************************/
static FmRadioController_slsi * pFMRadio = NULL;
static long current_channel;

unsigned char test_read_data[TEST_DATA_SIZE][12];
radio_data_t test_radio_data[TEST_DATA_SIZE];
struct PIECC_data test_piecc[TEST_DATA_SIZE];
struct RTPlus_data test_rtplus[TEST_DATA_SIZE];
struct Final_RDS_data final_rds_data[10];

RadioText RT_Final;
RadioText RT_Buffered;
ServiceName PS_Final;
ServiceName PS_Buffered;

unsigned char rt_flag;
unsigned char ps_flag;

unsigned int test_data_index;
unsigned int test_data_index_rtplus;
unsigned int test_data_index_rds;

pthread_t rds_print_thread;
bool thread_stop;
int print_mode;

/*******************************************************************************
 * Functions executed by fm_command
 ******************************************************************************/
void Initialise()
{
	int ret;

	printf("\n%s +++++++\n\n", __func__);

	if (pFMRadio->Initialise() != 0)
	{
		printf("%s failed to initialise FmRadio\n", __func__);
		return;
	}

	current_channel = 0;

	printf("%s -------\n\n", __func__);
}

void remove_FM()
{
	printf("\n%s +++++++\n\n", __func__);

	if (!pFMRadio) {
		printf("%s FmRadio is already removed\n", __func__);
		return;
	}

	delete pFMRadio;
	pFMRadio = NULL;

	printf("%s -------\n\n", __func__);
}

void SeekUp()
{
	long channel;

	printf("\n%s +++++++\n\n", __func__);

	channel = pFMRadio->SeekUp();
	if (channel == FM_FAILURE) {
		printf("%s failed to seek up\n", __func__);
		return;
	}

	current_channel = channel;

	printf("%s -------: current Channel: %1.fMHz\n\n", __func__, (float)channel/1000);
}

void SeekDown()
{
	long channel;

	printf("\n%s +++++++\n\n", __func__);

	channel = pFMRadio->SeekDown();
	if (channel == FM_FAILURE) {
		printf("%s failed to seek Down\n", __func__);
		return;
	}

	current_channel = channel;

	printf("%s -------: current Channel: %1.fMHz\n\n", __func__, (float)channel/1000);
}

void SeekCancel() {
	printf("\n%s +++++++\n\n", __func__);

	pFMRadio->SeekCancel();

	printf("%s -------\n\n", __func__);
}

void SetChannel()
{
	int channel;

	printf("\n%s: Input channel...\n", __func__);

	scanf(" %d", &channel);

	current_channel = (long)channel;

	pFMRadio->TuneChannel(current_channel);

	printf("\n%s -------: set channel: %.1fMHz\n\n", __func__, (float)current_channel/1000);
}

void GetChannel()
{
	long channel;

	printf("\n%s +++++++\n\n", __func__);

	channel = pFMRadio->GetChannel();
	if (channel == FM_FAILURE) {
		printf("%s failed to get channel\n", __func__);
		return;
	}

	printf("%s -------: current:%.1fMHz, get:%.1fMHz\n\n", __func__, (float)current_channel/1000, (float)channel/1000);

	current_channel = channel;
}

void SetBand()
{
	int band;

	printf("\n");
	printf("%s: Band [1] : European/U.S. (87.5 - 108)MHz\n", __func__);
	printf("%s: Band [2] : Not supported (76   - 108)MHz\n", __func__);
	printf("%s: Band [3] : Japanese      (76   -  90)MHz\n", __func__);
	printf("%s: Input band...\n", __func__);
	scanf(" %d", &band);

	pFMRadio->SetBand(band);

	printf("\n%s -------\n\n", __func__);
}

void SetChannelSpacing()
{
	int spacing;

	printf("%s: input spacing [5/10/20] (unit:10KHz)\n", __func__);
	scanf(" %d", &spacing);

	pFMRadio->SetChannelSpacing(spacing);

	printf("\n%s -------: Channel Spacing: %dKHz\n", __func__, spacing);
}

void setStereo()
{
	printf("\n%s +++++++\n\n", __func__);

	pFMRadio->SetStereo();

	printf("%s -------: Tuner mode Stereo\n\n", __func__);
}

void setMono()
{
	printf("\n%s +++++++\n\n", __func__);

	pFMRadio->SetMono();

	printf("%s -------: Tuner mode Mono\n\n", __func__);
}

void MuteOn()
{
	printf("\n%s +++++++\n\n", __func__);

	pFMRadio->MuteOn();

	printf("%s -------: Mute on\n\n", __func__);
}

void MuteOff()
{
	printf("\n%s +++++++\n\n", __func__);

	pFMRadio->MuteOff();

	printf("%s -------: Mute off\n\n", __func__);
}

void SoftMute()
{
	int mute;

	printf("\n%s: Input SoftMute (1:On / 0:Off) ...\n", __func__);
	scanf(" %d", &mute);

	pFMRadio->setSoftmute((bool)mute);

	if (mute)
		printf("\n%s -------: SoftMute On\n\n", __func__);
	else
		printf("\n%s -------: SoftMute Off\n\n", __func__);
}

void SetDeconstant()
{
	int DeConstant;

	printf("\n%s: [0]:75us   [1]:50usec (Not Supported)   [2]:0us", __func__);
	printf("\n%s: Input DeConstant ...\n", __func__);
	scanf(" %d", &DeConstant);

	pFMRadio->SetDeConstant((long)DeConstant);

	if (DeConstant)
		printf("\n%s -------: Set DeConstant\n\n", __func__);
	else
		printf("\n%s -------: Release DeConstant\n\n", __func__);
}

void EnableRDS()
{
	printf("%s +++++++\n", __func__);

	pFMRadio->EnableRDS();

	printf("%s -------\n", __func__);
}

void DisableRDS()
{
	printf("%s +++++++\n", __func__);

	pFMRadio->DisableRDS();

	printf("%s -------\n", __func__);
}

void EnableDNS()
{
	printf("%s +++++++\n", __func__);
	printf("%s -------\n", __func__);
}

void DisableDNS()
{
	printf("%s +++++++\n", __func__);
	printf("%s -------\n", __func__);
}

void EnableAF()
{
	printf("%s +++++++\n", __func__);
	printf("%s -------\n", __func__);
}

void DisableAF()
{
	printf("%s +++++++\n", __func__);
	printf("%s -------\n", __func__);
}

void CancelAfSwitchingProcess()
{
	printf("%s +++++++\n", __func__);
	printf("%s -------\n", __func__);
}

void GetCurrentRSSI()
{
	long c_rssi;

	printf("\n%s +++++++\n\n", __func__);

	c_rssi = pFMRadio->GetCurrentRSSI();
	if (c_rssi == FM_FAILURE) {
		printf("%s failed to get current RSSI\n", __func__);
		return;
	}

	printf("%s -------: current RSSI: %ld\n\n", __func__, c_rssi);
}

void GetCurrentSNR()
{
	long c_snr;

	printf("\n%s +++++++\n\n", __func__);

	c_snr = pFMRadio->GetCurrentSNR();
	if (c_snr == FM_FAILURE) {
		printf("%s failed to get current SNR\n", __func__);
		return;
	}

	printf("%s -------: current SNR: %ld\n\n", __func__, c_snr);
}

void SetCurrentRSSI()
{
	long c_rssi;

	printf("\n%s: Input RSSI ...\n", __func__);
	scanf(" %ld", &c_rssi);

	pFMRadio->SetCurrentRSSI(c_rssi);

	printf("\n%s -------\n\n", __func__);
}

void GetSeekMode()
{
	long seek_mode;

	printf("\n%s +++++++\n\n", __func__);

	seek_mode = pFMRadio->GetSeekMode();
	if (seek_mode == FM_FAILURE) {
		printf("%s failed to get seek mode\n", __func__);
		return;
	}

	printf("%s: seek mode: %ld\n", __func__, seek_mode);
	printf("%s: 0. STOP SEARCH MODE\n", __func__);
	printf("%s: 1. PRESET MODE\n", __func__);
	printf("%s: 2. AUTONOMOUS SEARCH MODE\n", __func__);
	printf("%s: 3. AF JUMP MODE\n", __func__);
	printf("%s: 4. AUTONOMOUS SEARCH MODE SKIP\n\n", __func__);
}

void SetSeekMode()
{
	long seek_mode;

	printf("\n%s: Seek Mode", __func__);
	printf("\n%s: 0. STOP SEARCH MODE", __func__);
	printf("\n%s: 1. PRESET MODE", __func__);
	printf("\n%s: 2. AUTONOMOUS SEARCH MODE", __func__);
	printf("\n%s: 3. AF JUMP MODE", __func__);
	printf("\n%s: 4. AUTONOMOUS SEARCH MODE SKIP", __func__);
	printf("\n%s: Select ...\n", __func__);
	scanf(" %ld", &seek_mode);

	pFMRadio->SetSeekMode(seek_mode);

	printf("\n%s -------\n\n", __func__);
}

/*******************************************************************************
 * Functions for RDS
 ******************************************************************************/
void *rds_print_thread_handler(void *arg)
{
	int i, j;

	if (arg == NULL)
		printf("\n%s has not any arg\n", __func__);

	while (!thread_stop) {
		for (i = 0; i < TEST_DATA_SIZE; i++) {
			switch (print_mode) {
			case 1:
				printf("\n");
				for (j = 0; j < 12; j++) {
					printf("%02x ", test_read_data[i][j]);
				}
				break;

			case 2:
				printf("\n%04x %04x %04x %04x %02x %04x %04x %04x %04x",
					test_radio_data[i].rdsa, test_radio_data[i].rdsb,
					test_radio_data[i].rdsc, test_radio_data[i].rdsd,
					test_radio_data[i].curr_rssi,
					test_radio_data[i].blera, test_radio_data[i].blerb,
					test_radio_data[i].blerc, test_radio_data[i].blerd);
				break;

			case 3:
				printf("\n%04x %02x", test_piecc[i].PI, test_piecc[i].ECC);
				break;

			case 4:
				printf("\nRTPLUS");
				break;

			case 5:
				printf("\n%s,  %s,  %ld",
					final_rds_data[i].StationName, final_rds_data[i].RadioText,
					final_rds_data[i].Af_frequency);
				break;

			default:
				printf("\ndefault\n");
				break;
			}
		}
		printf("\n");
		usleep(1000000);
	}

	pthread_exit(NULL);
	return NULL;
}

void StartRDSData()
{
	int select;
	int ret;

	printf("\n%s: 1. read   data", __func__);
	printf("\n%s: 2. radio  data", __func__);
	printf("\n%s: 3. piecc  data", __func__);
	printf("\n%s: 4. rtplus data", __func__);
	printf("\n%s: 5. rds    data", __func__);
	printf("\n%s: Select data ...\n", __func__);
	scanf(" %d", &select);

	print_mode = select;
	thread_stop = false;
	ret = pthread_create(&rds_print_thread, NULL, rds_print_thread_handler, NULL);
	if (ret < 0) {
		printf("\nFailed to create thread\n\n");
		return;
	}

	printf("%s -------: thread creation success\n\n", __func__);
}

void StopRDSData()
{
	printf("\n%s +++++++\n\n", __func__);

	thread_stop = true;
	pthread_join(rds_print_thread, NULL);
	rds_print_thread = (pthread_t)NULL;

	printf("%s -------: thread termination success\n\n", __func__);
}

void SetSoftStereoBlendCoeff()
{
	long value;

	printf("\n%s: Input value ...\n", __func__);
	scanf(" %ld", &value);

	pFMRadio->SetSoftStereoBlendCoeff(value);

	printf("\n%s -------\n\n", __func__);
}

void SetSoftMuteCoeff()
{
	long value;

	printf("\n%s: Input value ...\n", __func__);
	scanf(" %ld", &value);

	pFMRadio->SetSoftMuteCoeff(value);

	printf("\n%s -------\n\n", __func__);
}

/*******************************************************************************
 * Main Function
 ******************************************************************************/
void print_command() {
	printf("======================================================\n");
	printf(" 1. on\n");
	printf(" 2. off\n");
	printf(" 3. seek up\n");
	printf(" 4. seek down\n");
	printf(" 5. seek cancel\n");
	printf(" 6. set channel\n");
	printf(" 7. get current channel\n");
	printf(" 8. set band\n");
	printf(" 9. set channel spacing\n");
	printf("10. set stereo\n");
	printf("11. set mono\n");
	printf("12. mute on\n");
	printf("13. mute off\n");
	printf("14. soft-mute\n");
	printf("15. set deconstant\n");
	printf("16. enable RDS\n");
	printf("17. disable RDS\n");
	printf("18. enable DNS\n");
	printf("19. disable DNS\n");
	printf("20. enable AF\n");
	printf("21. disable AF\n");
	printf("22. cancel AF Switching\n");
	printf("23. get current rssi\n");
	printf("24. get current snr\n");
	printf("25. set current rssi\n");
	printf("26. get seek mode\n");
	printf("27. set seek mode\n");
	printf("28. start print rds data\n");
	printf("29. stop print rds data\n");
	printf("51. set soft stereo blend coeff\n");
	printf("52. set soft mute coeff\n");
	printf("======================================================\n");
	printf("Enter Command Number... ");
}

int main() {
	int input;

	if (pFMRadio)
		printf("%s pFMRadio is already used, please exit and then re-start\n", __func__);

	pFMRadio = new FmRadioController_slsi();
	if (pFMRadio)
		printf("%s start FmRadio TEST\n\n", __func__);
	else
		return 0;

	while (1) {
		print_command();
		scanf(" %d",&input);

		switch (input) {
			case FM_CMD_ON:
				Initialise();
				break;

			case FM_CMD_OFF:
				remove_FM();
				return 0;

			case FM_CMD_SEEK_UP:
				SeekUp();
				break;

			case FM_CMD_SEEK_DOWN:
				SeekDown();
				break;

			case FM_CMD_SEEK_CANCEL:
				SeekCancel();
				break;

			case FM_CMD_SET_CHANNEL:
				SetChannel();
				break;

			case FM_CMD_GET_CHANNEL:
				GetChannel();
				break;

			case FM_CMD_SET_BAND:
				SetBand();
				break;

			case FM_CMD_SET_CHANNEL_SPACING:
				SetChannelSpacing();
				break;

			case FM_CMD_SET_STEREO:
				setStereo();
				break;

			case FM_CMD_SET_MONO:
				setMono();
				break;

			case FM_CMD_MUTE_ON:
				MuteOn();
				break;

			case FM_CMD_MUTE_OFF:
				MuteOff();
				break;

			case FM_CMD_SOFT_MUTE:
				SoftMute();
				break;

			case FM_CMD_SET_DECONSTANT:
				SetDeconstant();
				break;

			case FM_CMD_ENABLE_RDS:
				EnableRDS();
				break;

			case FM_CMD_DISABLE_RDS:
				DisableRDS();
				break;

			case FM_CMD_ENABLE_DNS:
				EnableDNS();
				break;

			case FM_CMD_DISABLE_DNS:
				DisableDNS();
				break;

			case FM_CMD_ENABLE_AF:
				EnableAF();
				break;

			case FM_CMD_DISABLE_AF:
				DisableAF();
				break;

			case FM_CMD_CANCEL_AF_SWITCHING:
				CancelAfSwitchingProcess();
				break;

			case FM_CMD_GET_CURRENT_RSSI:
				GetCurrentRSSI();
				break;

			case FM_CMD_GET_CURRENT_SNR:
				GetCurrentSNR();
				break;

			case FM_CMD_SET_CURRENT_RSSI:
				SetCurrentRSSI();
				break;

			case FM_CMD_GET_SEEK_MODE:
				GetSeekMode();
				break;

			case FM_CMD_SET_SEEK_MODE:
				SetSeekMode();
				break;

			case FM_CMD_START_RDS_DATA:
				StartRDSData();
				break;

			case FM_CMD_STOP_RDS_DATA:
				StopRDSData();
				break;

			case FM_CMD_SET_SOFT_STEREO_BLEND_COEFF:
				SetSoftStereoBlendCoeff();
				break;

			case FM_CMD_SET_SOFT_MUTE_COEFF:
				SetSoftMuteCoeff();
				break;

			default:
				printf("CMD[%d] is not supported\n", input);
				break;
		}
	}

	return 0;
}
