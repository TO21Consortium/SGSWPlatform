/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <string.h>

#include <cutils/log.h>

#include <linux/input.h>

#include "SensorBase.h"

/*****************************************************************************/

SensorBase::SensorBase(
        const char* dev_name,
        const char* data_name)
    : dev_name(dev_name), data_name(data_name),
      dev_fd(-1), data_fd(-1)
{
    if (data_name) {
        data_fd = openInput(data_name);
	STLOGI("SensorBase: data_name %s,data_fd%d", data_name, data_fd);
    }
}

SensorBase::~SensorBase() {
    if (data_fd >= 0) {
        close(data_fd);
    }
    if (dev_fd >= 0) {
        close(dev_fd);
    }
}

int SensorBase::open_device() {
    if (dev_fd<0 && dev_name) {
        dev_fd = open(dev_name, O_RDONLY);
        STLOGE_IF(dev_fd<0, "Couldn't open %s (%s)", dev_name, strerror(errno));
    }
    return 0;
}

int SensorBase::close_device() {
    if (dev_fd >= 0) {
        close(dev_fd);
        dev_fd = -1;
    }
    return 0;
}

int SensorBase::getFd() const {
    if (!data_name) {
        return dev_fd;
    }
    return data_fd;
}

int SensorBase::setDelay(int32_t handle, int64_t ns) {
    return 0;
}

bool SensorBase::hasPendingEvents() const {
    return false;
}

int64_t SensorBase::getTimestamp() {
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return int64_t(t.tv_sec)*1000000000LL + t.tv_nsec;
}

int SensorBase::getSysfsDevicePath(char* sysfs_path ,const char* inputDeviceName)
{
	int fd = -1;
	const char *dirname = "/dev/input";
	char devname[PATH_MAX];
	char *filename;
	DIR *dir;
	struct dirent *de;

	sysfs_path[0] ='\0';
	dir = opendir(dirname);

	if(dir == NULL)
		return -1;

	strcpy(devname, dirname);
	filename = devname + strlen(devname);
	*filename++ = '/';

	while((de = readdir(dir))) {
		if(de->d_name[0] == '.' && (de->d_name[1] == '\0' || (de->d_name[1] == '.' && de->d_name[2] == '\0')))
			continue;

		strcpy(filename, de->d_name);
		fd = open(devname, O_RDONLY);
		if (fd >= 0) {
			char name[80];
			if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1)
				name[0] = '\0';

			if (!strcmp(name, inputDeviceName)) {
				strcpy(sysfs_path,"/sys/class/input/input2/");
				ALOGE("getSysfsDevicePath:%s",sysfs_path);
				break;
			} else {
				close(fd);
				fd = -1;
			}
		}
	}
	closedir(dir);
	STLOGE_IF(fd < 0, "couldn't find sysfs path for device '%s' ", inputDeviceName);
	return fd;
}

int SensorBase::openInput(const char* inputDeviceName)
{
	int fd = -1;
	fd = getSysfsDevicePath(input_sysfs_path, inputDeviceName);
	input_sysfs_path_len = strlen(input_sysfs_path);
	return fd;
}

