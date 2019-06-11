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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <android/log.h>
#include "rpmbd.h"

/* Logging function for outputing to stderr or log */
void log_print(int level, char *format, ...)
{
	if (level >= 0 && level <= LOG_MAX) {
		static int levels[5] = {
			ANDROID_LOG_DEBUG, ANDROID_LOG_INFO, ANDROID_LOG_WARN,
			ANDROID_LOG_ERROR, ANDROID_LOG_FATAL
		};
		va_list ap;
		va_start(ap, format);
		__android_log_vprint(levels[level], APP_NAME, format, ap);
		va_end(ap);

	}
}

static void title(void)
{
	printf("%s", program_version);
}

static void dump_packet(unsigned char *data, int len)
{
	unsigned char	s[17];
	int	i, j;

	s[16]='\0';
	for (i = 0; i < len; i += 16) {
		log_print(ERROR, "%06x :", i);
		for (j=0; j<16; j++) {
			log_print(ERROR, " %02x", data[i+j]);
			s[j] = (data[i+j]<' ' ? '.' : (data[i+j]>'}' ? '.' : data[i+j]));
		}
		log_print(ERROR, " |%s|\n",s);
	}
	log_print(ERROR, "\n");
}

static void swap_packet(u8 *p, u8 *d)
{
	int i;
	for (i = 0; i < RPMB_PACKET_SIZE; i++)
		d[i] = p[RPMB_PACKET_SIZE - 1 - i];
}

int rpmbd_code_for_cipher_string(char *user_name)
{
	int code = 0;

	/* RPMB 1KB area is used per one code number.
	   Code 1, 2 shouldn't be used, because that is already used in bootloader.
	   Code number has to be used over 3. */
	if (strncmp(user_name, "unittest", 10) == 0)
		code = 3;
	/* Will add additional scenario */

	return code;
}

/* The beginning of everything */
int main(int argc, char **argv)
{
	int server_sock, rval, connfd = 0, ret, code = 0, cnt = 0;
	struct sockaddr_un serv_addr;
	Rpmb_Req *req;
#ifdef USE_MMC_RPMB
	MMC_Ioctl_Command *ic;
	u8 *result_buf = NULL;
#else
	Scsi_Ioctl_Command *ic;
#endif
	char buf[4118];
	u16 address = 0;
	struct rpmb_packet packet;

	/* display application header */
	title();
	log_print(INFO, "*************** start rpmb daemon *************** \n");

	req = (Rpmb_Req *)malloc(sizeof(Rpmb_Req));
	if (req == NULL) {
		log_print(ERROR, "Memory allocation fail");
		exit(1);
	}

	server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_sock < 0) {
            log_print(ERROR, "Can't open stream socket");
	    free(req);
            exit(1);
        }

	bzero(&serv_addr, sizeof(serv_addr));
        serv_addr.sun_family = AF_UNIX;

	unlink("/data/app/rpmbd");
	strncpy(serv_addr.sun_path, "/data/app/rpmbd", sizeof(serv_addr.sun_path) - 1);

	if (bind(server_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
		log_print(ERROR, "Can't bind stream socket");
		close(server_sock);
		free(req);
		exit(1);
	}
	log_print(INFO, "Socket has name %s\n", serv_addr.sun_path);

	if (listen(server_sock, 5) == -1) {
		log_print(ERROR, "Can't listen for connection socket");
		close(server_sock);
		free(req);
		exit(1);
	}

	if (chmod("/data/app/rpmbd", 0660) < 0) {
		log_print(ERROR, "chmod fail");
		close(server_sock);
		free(req);
		exit(1);
	}

#ifdef USE_MMC_RPMB
	dev = open("/dev/block/mmcblk0rpmb", O_RDWR);
#else
	dev = open("/dev/block/sdb", O_RDWR);
#endif
	if (dev < 0) {
		log_print(ERROR, "<device open> fail");
		close(server_sock);
		free(req);
		exit(1);
	}

	while(1) {
		connfd = accept(server_sock, 0, 0);
		if (connfd == -1) {
			log_print(ERROR, "Err accept");
			continue;
		}

		bzero(buf, sizeof(buf));
		if ((rval = read(connfd, buf, sizeof(buf))) <= 0) {
			log_print(ERROR, "Reading stream message");
			goto con;
		} else {
			req = (Rpmb_Req *)buf;

			log_print(INFO, "type: %x", req->type);
			log_print(INFO, "user: %s\n", req->user);

			ret = 0;
			switch(req->type) {
			case GET_WRITE_COUNTER:
				if (dev == 0) {
					log_print(ERROR, "<device open> fail");
					ret = -1;
					break;
				}

				if (req->data_len != RPMB_PACKET_SIZE) {
					log_print(ERROR, "data len is invalid");
					ret = -1;
					break;
				}

				if (cnt != 0) {
					log_print(ERROR, "already lock: user: %s", req->user);
					ret = ERROR_RETRY;
					break;
				}
				cnt++;

#ifdef USE_MMC_RPMB
				ic = (MMC_Ioctl_Command *)malloc(sizeof(MMC_Ioctl_Command) + req->data_len);
				if (ic == NULL) {
					log_print(ERROR, "memory allocation fail");
					ret = -1;
					break;
				}

				/* Security OUT protocol */
				mmc_cmd_init(ic);
				ic->write_flag = true;
				ic->flags = MMC_RSP_R1;
				ic->opcode = MMC_WRITE_MULTIPLE_BLOCK;
				ic->data_ptr = (unsigned long)(buf + sizeof(Rpmb_Req));

				ret = ioctl(dev, MMC_IOC_CMD, ic);
				if (ret != 0) {
					log_print(ERROR, "<mmc write multiple block> ioctl fail!");
					free(ic);
					break;
				}

				/* Security IN protocol */
				memset(ic->data_ptr, 0x0, RPMB_PACKET_SIZE);
				ic->write_flag = false;
				ic->opcode = MMC_READ_MULTIPLE_BLOCK;
				ret = ioctl(dev, MMC_IOC_CMD, ic);
				if (ret != 0) {
					log_print(ERROR, "<mmc read multiple block> ioctl fail!");
					free(ic);
					break;
				}
				memcpy(buf + sizeof(Rpmb_Req), ic->data_ptr, req->data_len);
#else
				ic = (Scsi_Ioctl_Command *)malloc(sizeof(Scsi_Ioctl_Command) + req->data_len);
				if (ic == NULL) {
					log_print(ERROR, "memory allocation fail");
					ret = -1;
					break;
				}
				memcpy(ic->data , buf + sizeof(Rpmb_Req), req->data_len);
				ic->outlen = req->data_len;

				/* Security OUT protocol */
				ret = ioctl(dev, SCSI_IOCTL_SECURITY_PROTOCOL_OUT, ic);
				if (ret != 0) {
					log_print(ERROR, "<security protocol out> ioctl fail!");
					free(ic);
					break;
				}

				/* Security IN protocol */
				memset(ic->data, 0x0, req->data_len);
				ic->inlen = req->data_len;

				ret = ioctl(dev, SCSI_IOCTL_SECURITY_PROTOCOL_IN, ic);
				if (ret != 0) {
					log_print(ERROR, "<security protocol out> ioctl fail!");
					free(ic);
					break;
				}
				memcpy(buf + sizeof(Rpmb_Req), ic->data, req->data_len);
#endif
				free(ic);
				break;
			case WRITE_DATA:
				if (dev == 0) {
					log_print(ERROR, "<device open> fail");
					ret = -1;
					break;
				}

				if (cnt <= 0) {
					log_print(ERROR, "wrong cnt: %d", cnt);
					ret = -1;
					break;
				}

				if ((req->data_len < RPMB_PACKET_SIZE) || (req->data_len > RPMB_PACKET_SIZE * (RPMB_AREA/DATA_SIZE))) {
					log_print(ERROR, "data len is invalid");
					ret = -1;
					break;
				}

#ifdef USE_MMC_RPMB
				ic = (MMC_Ioctl_Command *)malloc(sizeof(MMC_Ioctl_Command) + req->data_len);
				if (ic == NULL) {
					log_print(ERROR, "memory allocation fail");
					ret = -1;
					break;
				}

				/* Security OUT protocol */
				mmc_cmd_init(ic);
				ic->write_flag = RELIABLE_WRITE_REQ_SET;
				ic->flags = MMC_RSP_R1;
				ic->blocks = req->data_len/RPMB_PACKET_SIZE;
				ic->opcode = MMC_WRITE_MULTIPLE_BLOCK;
				ic->data_ptr = (unsigned long)(buf + sizeof(Rpmb_Req));

				ret = ioctl(dev, MMC_IOC_CMD, ic);
				if (ret != 0) {
					log_print(ERROR, "<mmc write multiple block> ioctl fail!");
					goto wout;
				}

				result_buf = (u8 *)malloc(RPMB_PACKET_SIZE);
				if (result_buf == NULL) {
					log_print(ERROR, "memory allocation fail");
					ret = -1;
					goto wout;
				}
				ic->write_flag = true;
				ic->blocks = 1;
				ic->data_ptr = (unsigned long)result_buf;
				memset(&packet, 0x0, RPMB_PACKET_SIZE);
				packet.request = RESULT_READ_REQ;
				swap_packet((unsigned char *)&packet, result_buf);
				ret = ioctl(dev, MMC_IOC_CMD, ic);
				if (ret != 0) {
					log_print(ERROR, "<mmc write multiple block> ioctl fail!");
					free(result_buf);
					goto wout;
				}

				/* Security IN protocol */
				memset((void *)result_buf, 0, RPMB_PACKET_SIZE);
				ic->write_flag = false;
				ic->blocks = 1;
				ic->opcode = MMC_READ_MULTIPLE_BLOCK;
				ret = ioctl(dev, MMC_IOC_CMD, ic);
				if (ret != 0) {
					log_print(ERROR, "<mmc read multiple block> ioctl fail!");
					free(result_buf);
					goto wout;
				}
				memcpy(buf + sizeof(Rpmb_Req), result_buf, RPMB_PACKET_SIZE);
				free(result_buf);
#else
				ic = (Scsi_Ioctl_Command *)malloc(sizeof(Scsi_Ioctl_Command) + req->data_len);
				if (ic == NULL) {
					log_print(ERROR, "memory allocation fail");
					goto wout;
				}
				memcpy(ic->data , buf + sizeof(Rpmb_Req), req->data_len);
				ic->outlen = req->data_len;

				/* Security OUT protocol */
				ret = ioctl(dev, SCSI_IOCTL_SECURITY_PROTOCOL_OUT, ic);
				if (ret != 0) {
					log_print(ERROR, "<security protocol out> ioctl fail!");
					goto wout;
				}

				memset(ic->data, 0x0, req->data_len);
				memset(&packet, 0x0, RPMB_PACKET_SIZE);
				packet.request = RESULT_READ_REQ;
				swap_packet((unsigned char *)&packet, ic->data);
				ic->outlen = RPMB_PACKET_SIZE;

				ret = ioctl(dev, SCSI_IOCTL_SECURITY_PROTOCOL_OUT, ic);
				if (ret != 0) {
					log_print(ERROR, "<security protocol out> ioctl fail!");
					goto wout;
				}

				/* Security IN protocol */
				memset(ic->data, 0x0, req->data_len);
				ic->inlen = RPMB_PACKET_SIZE;

				ret = ioctl(dev, SCSI_IOCTL_SECURITY_PROTOCOL_IN, ic);
				if (ret != 0) {
					log_print(ERROR, "<security protocol out> ioctl fail!");
					goto wout;
				}
				memcpy(buf + sizeof(Rpmb_Req), ic->data, req->data_len);
#endif
wout:
				cnt--;
				free(ic);
				break;
			case READ_DATA:
				if (dev == 0) {
					log_print(ERROR, "<device open> fail");
					ret = -1;
					break;
				}

				if ((req->data_len < RPMB_PACKET_SIZE) || (req->data_len > RPMB_PACKET_SIZE * (RPMB_AREA/DATA_SIZE))) {
					log_print(ERROR, "data len is invalid");
					ret = -1;
					break;
				}

#ifdef USE_MMC_RPMB
				ic = (MMC_Ioctl_Command *)malloc(sizeof(MMC_Ioctl_Command) + req->data_len);
				if (ic == NULL) {
					log_print(ERROR, "memory allocation fail");
					ret = -1;
					break;
				}

				/* Security OUT protocol */
				mmc_cmd_init(ic);
				ic->write_flag = true;
				ic->flags = MMC_RSP_R1;
				ic->opcode = MMC_WRITE_MULTIPLE_BLOCK;
				ic->data_ptr = (unsigned long)(buf + sizeof(Rpmb_Req));

				result_buf = (u8 *)malloc(RPMB_PACKET_SIZE);
				if (result_buf == NULL) {
					log_print(ERROR, "memory allocation fail");
					ret = -1;
					free(ic);
					break;
				}

				/* At read data with MMC, block count has to '0' */
				swap_packet((u8 *)((addr_size_t)ic->data_ptr), result_buf);
				((struct rpmb_packet *)(result_buf))->count = 0;
				swap_packet(result_buf, (u8 *)((addr_size_t)ic->data_ptr));
				free(result_buf);

				ret = ioctl(dev, MMC_IOC_CMD, ic);
				if (ret != 0) {
					log_print(ERROR, "<mmc write multiple block> ioctl fail!");
					free(ic);
					break;
				}

				/* Security IN protocol */
				memset(ic->data_ptr, 0x0, req->data_len);
				ic->write_flag = false;
				ic->opcode = MMC_READ_MULTIPLE_BLOCK;
				ic->blocks = req->data_len/RPMB_PACKET_SIZE;

				ret = ioctl(dev, MMC_IOC_CMD, ic);
				if (ret != 0) {
					log_print(ERROR, "<mmc read multiple block> ioctl fail!");
					free(ic);
					break;
				}
				memcpy(buf + sizeof(Rpmb_Req), ic->data_ptr, req->data_len);
#else
				ic = (Scsi_Ioctl_Command *)malloc(sizeof(Scsi_Ioctl_Command) + req->data_len);
				if (ic == NULL) {
					log_print(ERROR, "memory allocation fail");
					ret = -1;
					break;
				}
				memcpy(ic->data , buf + sizeof(Rpmb_Req), RPMB_PACKET_SIZE);
				ic->outlen = RPMB_PACKET_SIZE;

				/* Security OUT protocol */
				ret = ioctl(dev, SCSI_IOCTL_SECURITY_PROTOCOL_OUT, ic);
				if (ret != 0) {
					log_print(ERROR, "<security protocol out> ioctl fail!");
					free(ic);
					break;
				}

				/* Security IN protocol */
				memset(ic->data, 0x0, req->data_len);
				ic->inlen = req->data_len;
				ret = ioctl(dev, SCSI_IOCTL_SECURITY_PROTOCOL_IN, ic);
				if (ret != 0) {
					log_print(ERROR, "<security protocol in> ioctl fail!");
					free(ic);
					break;
				}
				memcpy(buf + sizeof(Rpmb_Req), ic->data, req->data_len);
#endif

				free(ic);
				break;
			case GET_ADDRESS:
				code = rpmbd_code_for_cipher_string(req->user);
				if (!code) {
					log_print(ERROR, "Invalid user");
					ret = -1;
					break;
				}
				address = (code - 1) * RPMB_AREA/DATA_SIZE;
				memcpy(buf + sizeof(Rpmb_Req), &address, sizeof(address));

				break;
			default:
				log_print(ERROR, "Invalid request type!");
				break;
			}

			req->ret = ret;
			rval = 0;
			memcpy(buf, req, sizeof(Rpmb_Req));
			if ((rval = write(connfd, buf, sizeof(buf))) < 0)
				log_print(ERROR, "socket write fail!");
		}
con:
		close(connfd);
		connfd = 0;
	}

	if (dev)
		close(dev);

	close(server_sock);
	unlink("/data/app/rpmbd");
	free(req);

	log_print(INFO,"\n *************** end rpmb daemon *************** \n");

	return 0;
}
