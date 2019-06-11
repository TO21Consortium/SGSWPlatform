/*
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 * Copyright (C) 2013 Samsung Electronics Co., LTD
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>
#include <linux/random.h>
#include <sys/poll.h>
#include <unistd.h>
#include <errno.h>

#ifdef ANDROID_CHANGES
#include <android/log.h>
#endif

#ifndef min
	#define min(a,b) (((a)>(b))?(b):(a))
#endif

typedef unsigned char bool;

#define TRUE 1
#define FALSE 0

#define RANDOM_DEVICE       "/dev/random"
#define RANDOM_DEVICE_HW    "/dev/hw_random"

#define MAX_ENT_POOL_WRITES 128			/* write pool with smaller chunks */
#define RANDOM_NUMBER_BYTES 256			/* random data byte to check randomness */
#ifdef USES_FIPS_COMPLIANCE_RNG_DRV
#define MAX_BUFFER 256				/* do not change this value       */
#else
/* Buffer to hold hardware entropy bytes (this must be 2KB for FIPS testing */
#define MAX_BUFFER 2048				/* do not change this value       */
#endif
static unsigned char databuf[MAX_BUFFER];	/* create buffer for FIPS testing */
static unsigned long buffsize;			/* size of data in buffer         */
static unsigned long curridx;			/* position of current index      */

/* User parameters */
struct user_options {
	char            input_device_name[128];
	char            output_device_name[128];
	bool            run_as_daemon;
};

/* Version number of this source */
#define APP_NAME    "exyrngd"

const char *program_version =
APP_NAME "\n"
"Copyright (c) 2011, Code Aurora Forum. All rights reserved.\n"
"This is free software; see the source for copying conditions.  There is NO\n"
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n";

const char *program_usage =
"Usage: " APP_NAME " [OPTION...]\n"
"  -b                 background - become a daemon (default)\n"
"  -f                 foreground - do not fork and become a daemon\n"
"  -r <device name>   hardware random input device (default: /dev/hw_random)\n"
"  -o <device name>   system random output device (default: /dev/random)\n"
"  -h                 help (this page)\n";

/* Logging information */
enum log_level {
	DEBUG = 0,
	INFO = 1,
	WARNING = 2,
	ERROR = 3,
	FATAL = 4,
	LOG_MAX = 4,
};

/* Logging function for outputing to stderr or log */
void log_print(int level, char *format, ...)
{
	if (level >= 0 && level <= LOG_MAX) {
#ifdef ANDROID_CHANGES
		static int levels[5] = {
			ANDROID_LOG_DEBUG, ANDROID_LOG_INFO, ANDROID_LOG_WARN,
			ANDROID_LOG_ERROR, ANDROID_LOG_FATAL
		};
		va_list ap;
		va_start(ap, format);
		__android_log_vprint(levels[level], APP_NAME, format, ap);
		va_end(ap);
#else
		static char *levels = "DIWEF";
		va_list ap;
		fprintf(stderr, "%c: ", levels[level]);
		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
		fputc('\n', stderr);
#endif
	}
}

static void title(void)
{
	printf("%s", program_version);
}

static void usage(void)
{
	printf("%s", program_usage);
}

/* Parse command line parameters */
static int get_user_options(struct user_options *user_ops, int argc, char **argv)
{
	int max_params = argc;
	int itr = 1;			/* skip program name */
	while (itr < max_params) {
		if (argv[itr][0] != '-')
			return -1;

		switch (argv[itr++][1]) {
			case 'b':
				user_ops->run_as_daemon = TRUE;
				break;

			case 'f':
				user_ops->run_as_daemon = FALSE;
				break;

			case 'r':
				if (itr < max_params) {
					if (strlen(argv[itr]) < sizeof(user_ops->input_device_name)) {
						strncpy(user_ops->input_device_name, argv[itr], strlen(argv[itr]) + 1);
						itr++;
					}
					else
						return -1;
					break;
				}
				else
					return -1;

			case 'o':
				if (itr < max_params) {
					if (strlen(argv[itr]) < sizeof(user_ops->output_device_name)) {
						strncpy(user_ops->output_device_name, argv[itr], strlen(argv[itr]) + 1);
						itr++;
					}
					else
						return -1;
					break;
				}
				else
					return -1;

			case 'h':
				return -1;


			default:
				fprintf(stderr, "ERROR: Bad option: '%s'\n", argv[itr-1]);
				return -1;
		}
	}
	return 0;
}

/* Only check FIPS 140-2 (Continuous Random Number Generator Test) */
static int fips_test(const unsigned char *buf, size_t size)
{
	unsigned long *buff_ul = (unsigned long *) buf;
	size_t size_ul = size >> 2;	/* convert byte to word size */
	unsigned long last_value;
	unsigned int rnd_ctr[256];
	unsigned int i;

	/* Continuous Random Number Generator Test */
	last_value = *(buff_ul++);
	size_ul--;

	while (size_ul > 0) {
		if (*buff_ul == last_value) {
			log_print(ERROR, "ERROR: Bad word value from hardware.");
			return -1;
		} else
			last_value = *buff_ul;
		buff_ul++;
		size_ul--;
	}

	memset(rnd_ctr, 0, sizeof(rnd_ctr));
	/* count each random number */
	for (i = 0; i < size; ++i) {
		rnd_ctr[buf[i]]++;
	}

	/* check random numbers to make sure they are not bogus */
	for (i = 0; i < RANDOM_NUMBER_BYTES; ++i) {
		if (rnd_ctr[i] == 0) {
			log_print(ERROR, "ERROR: Bad spectral random number sample.");
			return -1;
		}
	}

	return 0;
}

/* Read data from the hardware RNG source */
static int read_src(int fd, void *buf, size_t size)
{
	size_t offset = 0;
	char *chr = (char *) buf;
	ssize_t ret;

	if (!size)
		return -1;
	do {
		ret = read(fd, chr + offset, size);
		/* any read failure is bad */
		if (ret == -1)
			return -1;
		size -= ret;
		offset += ret;
	} while (size > 0);

	return 0;
}

/* The beginning of everything */
int main(int argc, char **argv)
{
	struct user_options user_ops;		/* holds user configuration data     */
	struct rand_pool_info *rand = NULL;	/* structure to pass entropy (IOCTL) */
	int random_fd = 0;			/* output file descriptor            */
	int random_hw_fd = 0;			/* input file descriptor             */
	int write_size;				/* max entropy data to pass          */
	struct pollfd fds[1];			/* used for polling file descriptor  */
	int ret;
	int exitval = 0;

	/* set default parameters */
	user_ops.run_as_daemon = TRUE;
	strncpy(user_ops.input_device_name, RANDOM_DEVICE_HW, strlen(RANDOM_DEVICE_HW) + 1);
	strncpy(user_ops.output_device_name, RANDOM_DEVICE, strlen(RANDOM_DEVICE) + 1);

	/* display application header */
	title();

	/* get user preferences */
	ret = get_user_options(&user_ops, argc, argv);
	if (ret < 0) {
		usage();
		exitval = 1;
		goto exit;
	}

	/* open hardware random device */
	random_hw_fd = open(user_ops.input_device_name, O_RDONLY);
	if (random_hw_fd < 0) {
		fprintf(stderr, "Can't open hardware random device file %s\n", user_ops.input_device_name);
		exitval = 1;
		goto exit;
	}

	/* open random device */
	random_fd = open(user_ops.output_device_name, O_RDWR);
	if (random_fd < 0) {
		fprintf(stderr, "Can't open random device file %s\n", user_ops.output_device_name);
		exitval = 1;
		goto exit;
	}

	/* allocate memory for ioctl data struct and buffer */
	rand = malloc(sizeof(struct rand_pool_info) + MAX_ENT_POOL_WRITES);
	if (!rand) {
		fprintf(stderr, "Can't allocate memory\n");
		exitval = 1;
		goto exit;
	}

	/* setup poll() data */
	memset(fds, 0, sizeof(fds));
	fds[0].fd = random_fd;
	fds[0].events = POLLOUT;

	/* run as daemon if requested to do so */
	if (user_ops.run_as_daemon) {
		fprintf(stderr, "Starting daemon.\n");
		if (daemon(0, 0) < 0) {
			fprintf(stderr, "can't daemonize: %s\n", strerror(errno));
			exitval = 1;
			goto exit;
		}
#ifndef ANDROID_CHANGES
		openlog(APP_NAME, 0, LOG_DAEMON);
#endif
	}

	/* log message */
	log_print(INFO, APP_NAME " has started:\n" "Reading device:'%s' updating entropy for device:'%s'",
		  user_ops.input_device_name,
		  user_ops.output_device_name);

	/* main loop to get data from hardware and feed RNG entropy pool */
	while (1) {
		/* Check for empty buffer and fill with hardware random generated numbers */
		if (buffsize == 0) {
			/* fill buffer with random data from hardware */
			ret = read_src(random_hw_fd, databuf, MAX_BUFFER);
			if (ret < 0) {
				log_print(ERROR, "ERROR: Can't read from hardware source.");
				continue;
			}

			/* reset buffer variables to indicate full buffer */
			buffsize = MAX_BUFFER;
			curridx  = 0;

#ifndef USES_FIPS_COMPLIANCE_RNG_DRV
			/* run FIPS test on buffer, if buffer fails then ditch it and get new data */
			ret = fips_test(databuf, MAX_BUFFER);
			if (ret < 0) {
				buffsize = 0;
				log_print(INFO, "ERROR: Failed FIPS test.");
			}
#endif
		}
		/* We should have data here, if not then something bad happened above and we should wait and try again */
		if (buffsize == 0) {
			log_print(ERROR, "ERROR: Timeout getting valid random data from hardware.");
			continue;
		}

		/* fill entropy pool */
		write_size = min(buffsize, MAX_ENT_POOL_WRITES);

		/* Write some data to the device */
		rand->entropy_count = write_size * 8;
		rand->buf_size      = write_size;
		memcpy(rand->buf, &databuf[curridx], write_size);
		curridx  += write_size;
		buffsize -= write_size;

		/* Issue the ioctl to increase the entropy count */
		if (ioctl(random_fd, RNDADDENTROPY, rand) < 0) {
			log_print(ERROR,"ERROR: RNDADDENTROPY ioctl() failed.");
			exitval = 1;
			goto exit;
		}

		/* Wait if entropy pool is full */
		ret = poll(fds, 1, -1);
		if (ret < 0) {
			log_print(ERROR,"ERROR: poll call failed.");
			exitval = 1;
			goto exit;
		}
	}

exit:
	/* free other resources */
	if (rand)
		free(rand);
	if (random_fd >= 0)
		close(random_fd);
	if (random_hw_fd >= 0)
		close(random_hw_fd);
	return exitval;
}
