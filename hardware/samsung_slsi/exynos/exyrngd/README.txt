Exrng Daemon
=======================================================

Description:
The exyrng daemon is used to check and fill entropy.
By default it opens the "/dev/random" device and "dev/hw_random" device.
It requires H/W random driver(/dev/hw_random) for random data.
It requires random driver(/dev/random) for entropy

Parameters:
It will accept the following optional arguments:
	-b                 background - become a daemon(default)
	-f                 foregrount = do not fork and become a daemon
	-r <device name>   hardware random input device (default: /dev/hw_random)
	-o <device name>   system random output device (default: /dev/random)
	-h		   help

Return:
It will return 0 if all cases succeed otherwise it
returns -1:

Usage:
By init.rc exyrng daemon is automatically executed in booting time.
Exyrng daemon should be applied default root permission of user and group
To approach a random driver in order to check entropy, the permission is necessary

Details:
Main loop check for entropy,  get random data and feed entropy pool
2048 byte daemon buffer is filled from H/W random driver if buffer is zero.
exyrng daemon makes increase 128 bytes of entropy at a time if entropy count is insufficient.

Files:
	README			this file
	LICENSE			terms of distribution and reuse(BSD)

	Android.mk		script file, inform about native executable file
	exyrngd.c		exyrng daemon

Targets:
        Exynos
