/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __RPMBD_H__
#define __RPMBD_H__

/* Version number of this source */
#define APP_NAME				"rpmbd"
#define RPMB_PACKET_SIZE			512

#define GET_WRITE_COUNTER			1
#define WRITE_DATA				2
#define READ_DATA				3
#define GET_ADDRESS				4

#define AUTHEN_KEY_PROGRAM_RES			0x0100
#define AUTHEN_KEY_PROGRAM_REQ			0x0001
#define RESULT_READ_REQ				0x0005
#define RPMB_END_ADDRESS			0x4000
#define ERROR_RETRY				0xff

#define RPMB_AREA				1024
#define DATA_SIZE				256

int dev;

const char *program_version =
APP_NAME "\n"
"version 0.0";

typedef unsigned char bool;
typedef unsigned short u16;
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
#ifdef TARGET_IS_64_BIT
/* define address size type for 32bit and 64bit address */
typedef uint64_t addr_size_t;
#else
typedef uint32_t addr_size_t;
#endif

typedef struct rpmb_req {
	unsigned int type;
	unsigned char user[10];
	unsigned int ret;
	unsigned int data_len;
	unsigned char rpmb_data[0];
} Rpmb_Req;

#ifdef USE_MMC_RPMB

#define AUTHEN_DATA_WRITE_RES		0x0300
#define	AUTHEN_DATA_READ_RES		0x0400
#define RELIABLE_WRITE_REQ_SET		(1 << 31)

#define true 1
#define false 0

#define MMC_RSP_PRESENT	(1 << 0)
#define MMC_RSP_CRC	(1 << 2)		/* expect valid crc */
#define MMC_RSP_OPCODE	(1 << 4)		/* response contains opcode */

#define MMC_RSP_NONE	(0)
#define MMC_RSP_R1	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

#define MMC_BLOCK_MAJOR			179
#define MMC_IOC_CMD _IOWR(MMC_BLOCK_MAJOR, 0, MMC_Ioctl_Command)
#define MMC_READ_MULTIPLE_BLOCK  18   /* adtc [31:0] data addr   R1  */
#define MMC_WRITE_MULTIPLE_BLOCK 25   /* adtc                    R1  */

typedef struct mmc_ioc_cmd {
	/* Implies direction of data.  true = write, false = read */
	int write_flag;

	/* Application-specific command.  true = precede with CMD55 */
	int is_acmd;

	u32 opcode;
	u32 arg;
	u32 response[4];  /* CMD response */
	unsigned int flags;
	unsigned int blksz;
	unsigned int blocks;

	/*
	 * Sleep at least postsleep_min_us useconds, and at most
	 * postsleep_max_us useconds *after* issuing command.  Needed for
	 * some read commands for which cards have no other way of indicating
	 * they're ready for the next command (i.e. there is no equivalent of
	 * a "busy" indicator for read operations).
	 */
	unsigned int postsleep_min_us;
	unsigned int postsleep_max_us;

	/*
	 * Override driver-computed timeouts.  Note the difference in units!
	 */
	unsigned int data_timeout_ns;
	unsigned int cmd_timeout_ms;

	/*
	 * For 64-bit machines, the next member, ``__u64 data_ptr``, wants to
	 * be 8-byte aligned.  Make sure this struct is the same size when
	 * built for 32-bit.
	 */
	u32 __pad;

	/* DAT buffer */
	u64 data_ptr;
} MMC_Ioctl_Command;

static void mmc_cmd_init(MMC_Ioctl_Command *cmd)
{
	cmd->is_acmd = false;
	cmd->arg = 0;
	cmd->flags = MMC_RSP_R1;
	cmd->blksz = RPMB_PACKET_SIZE;
	cmd->blocks = 1;
	cmd->postsleep_min_us = 0;
	cmd->postsleep_max_us = 0;
	cmd->data_timeout_ns = 0;
	cmd->cmd_timeout_ms = 0;
}

#else
#define SCSI_IOCTL_SECURITY_PROTOCOL_IN		7
#define SCSI_IOCTL_SECURITY_PROTOCOL_OUT	8

typedef struct scsi_ioctl_command {
	unsigned int inlen;
	unsigned int outlen;
	unsigned char data[0];
} Scsi_Ioctl_Command;
#endif

struct rpmb_packet {
       u16     request;
       u16     result;
       u16     count;
       u16     address;
       u32     write_counter;
       u8      nonce[16];
       u8      data[256];
       u8      Key_MAC[32];
       u8      stuff[196];
};

/* Logging information */
enum log_level {
	DEBUG = 0,
	INFO = 1,
	WARNING = 2,
	ERROR = 3,
	FATAL = 4,
	LOG_MAX = 4,
};

#endif // __RPMBD_H__
