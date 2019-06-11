/****************************************************************************
 *
 * Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd
 *
 ****************************************************************************/
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#define LOG_TAG "BT_VENDOR"

#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/param.h>

#include "bt_vendor_lib.h"

#include "bt_vendor_slsi.h"

/**
 * SSB-5242 Route HCI I/F over ABD to driver on PC.
 */
#ifndef ENABLE_HCI_TUNNELLING
#define ENABLE_HCI_TUNNELLING FALSE
#endif /* !ENABLE_HCI_TUNNELLING */
#if defined(ENABLE_HCI_TUNNELLING) && (ENABLE_HCI_TUNNELLING == TRUE)
#include <netinet/in.h>
#include <netdb.h>
#endif

#define BLUETOOTH_ADDRESS_FILE "/sys/module/scsc_bt/parameters/bluetooth_address"

/**
 * SSB-5242 Route HCI I/F over ABD to driver on PC.
 */
#if defined(ENABLE_HCI_TUNNELLING) && (ENABLE_HCI_TUNNELLING == TRUE)
static int hci_tunnelling_port_num = 10157;
#endif

static const bt_vendor_callbacks_t *vcb;
static unsigned long long bt_addr;

static char h4_file[MAXPATHLEN] = "/dev/scsc_h4_0";
static int hci_fd = -1;

/*
 * Internal helpers
 */

static void set_bluetooth_address(unsigned long long value)
{
    BTVENDORI("Setting Bluetooth address");
    if (!access(BLUETOOTH_ADDRESS_FILE, W_OK))
    {
        int bt_fd = open(BLUETOOTH_ADDRESS_FILE, O_RDWR);
        if (bt_fd != -1)
        {
            char buf[15];
            ssize_t res;

            res = sprintf(buf, "0x%llx", value);
            if (res != write(bt_fd, buf, res))
            {
                BTVENDORW("Unable setiting Bluetooth address (%s)", strerror(errno));
            }
            close(bt_fd);
        }
        else
        {
            BTVENDORW("Opening file: " BLUETOOTH_ADDRESS_FILE " - failed with: %s", strerror(errno));
        }
    }
    else
    {
        BTVENDORW("Unable to access: " BLUETOOTH_ADDRESS_FILE " - error: %s", strerror(errno));
    }
}

/*
 * Internal interface
 */

/**
 * SSB-5242 Route HCI I/F over ABD to driver on PC.
 */
#if defined(ENABLE_HCI_TUNNELLING) && (ENABLE_HCI_TUNNELLING == TRUE)
static int socket_open_client(int port_num)
{
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int fd = -1;

    BTVENDORI("HCI Tunnelling: socket_open_client(): port: 0x%08x.", port_num);

    fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0)
    {
        BTVENDORE("HCI Tunnelling: socket_open_client(): error from socket(), errno:0x%08x:%s.", errno, strerror(errno));
        fd = -1;
        return fd;
    }

    BTVENDORI("HCI Tunnelling: socket_open_client(): port: 0x%08x, fd: 0x%08x.", port_num, fd);

    server = gethostbyname("localhost");

    if (server == NULL)
    {
        BTVENDORE("HCI Tunnelling: socket_open_client(): error from gethostbyname(), errno:0x%08x:%s, fd: 0x%08x.", errno, strerror(errno), fd);
        fd = -1;
        return fd;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port_num);

    if (connect(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        BTVENDORE("HCI Tunnelling: socket_open_client(): error from connect(), errno:0x%08x:%s, fd: 0x%08x.", errno, strerror(errno), fd);
        fd = -1;
        return fd;
    }

    BTVENDORI("HCI Tunnelling: socket_open_client(): OK, fd:0x%08x.", fd);

    return fd;
}
#endif

static int hci_open(int *fds)
{
    int h4_fd;

    BTVENDORI("HCI open");

/**
 * SSB-5242 Route HCI I/F over ADB to driver on PC.
 */
#if defined(ENABLE_HCI_TUNNELLING) && (ENABLE_HCI_TUNNELLING == TRUE)
    h4_fd = socket_open_client(hci_tunnelling_port_num);
#else
    if (bt_addr != 0)
    {
        set_bluetooth_address(bt_addr);
    }

    BTVENDORI("Opening h4 device %s", h4_file);
    h4_fd = open(h4_file, O_RDWR);
#endif
    if (h4_fd == -1)
    {
        BTVENDORE("Open h4 device failed: %s (%d)", strerror(errno), errno);
        return -1;
    }

    BTVENDORI("Open h4 device %s succeded", h4_file);

    hci_fd = h4_fd;
    *fds = h4_fd;

    return 1;
}

static int hci_close(void)
{
    int fd = hci_fd;

    if (bt_addr != 0)
    {
        set_bluetooth_address(0);
    }

    hci_fd = -1;
    return close(fd);
}

static int init(const bt_vendor_callbacks_t *p_cb, unsigned char *local_bdaddr)
{
    /* Sanity check */
    if (p_cb == NULL)
    {
        ALOGE("init failed with no user callbacks!");
        return -1;
    }

    /* Store a reference to user callbacks */
    vcb = (bt_vendor_callbacks_t *) p_cb;

    /* Store the Bluetooth address */
    bt_addr = (((unsigned long long) local_bdaddr[0]) << 40) |
              (((unsigned long long) local_bdaddr[1]) << 32) |
              (((unsigned long long) local_bdaddr[2]) << 24) |
              (((unsigned long long) local_bdaddr[3]) << 16) |
              (((unsigned long long) local_bdaddr[4]) << 8) |
              ((unsigned long long) local_bdaddr[5]);

    return 0;
}

static int op(bt_vendor_opcode_t opcode, void *param)
{
    int retval = 0;

    switch (opcode)
    {
        case BT_VND_OP_POWER_CTRL:
        {
            break;
        }
        case BT_VND_OP_FW_CFG:
        {
            vcb->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);
            break;
        }
        case BT_VND_OP_SCO_CFG:
        {
            vcb->scocfg_cb(BT_VND_OP_RESULT_SUCCESS);
            break;
        }
        case BT_VND_OP_USERIAL_OPEN:
        {
            retval = hci_open((int *) param);
            BTVENDORD("hci_open returns: %d", retval);
            break;
        }
        case BT_VND_OP_USERIAL_CLOSE:
        {
            retval = hci_close();
            BTVENDORD("hci_close returns: %d", retval);
            break;
        }
        case BT_VND_OP_GET_LPM_IDLE_TIMEOUT:
        {
            uint32_t *timeout_ms = (uint32_t *) param;
            *timeout_ms = 3000; /* 3 Seconds delay until the idle alarm is fired */
            break;
        }
        case BT_VND_OP_LPM_SET_MODE:
        {
            /* LPM not required on SoC solutions - return fail to disable it */
            vcb->lpm_cb(BT_VND_OP_RESULT_FAIL);
            break;
        }
        case BT_VND_OP_LPM_WAKE_SET_STATE:
        {
            break;
        }
        case BT_VND_OP_EPILOG:
        {
            vcb->epilog_cb(BT_VND_OP_RESULT_SUCCESS);
            break;
        }
        case BT_VND_OP_SET_AUDIO_STATE:
        {
            /* Unhandled - just ignore */
            BTVENDORE("BT_VND_OP_SET_AUDIO_STATE - ignoring");
            break;
        }
        default:
        {
            BTVENDORW("Hit default with opcode:0x%02X", opcode);
            retval = -1;
            break;
        }
    }

    return retval;
}

static void cleanup(void)
{
    if (hci_fd != -1)
    {
        int fd = hci_fd;
        hci_fd = -1;
        (void) close(fd);
    }

    return;
}

const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE = {
    sizeof(bt_vendor_interface_t),
    init,
    op,
    cleanup,
};
