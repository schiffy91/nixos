#ifndef NIXOS_DECKPAD_NATIVE_H
#define NIXOS_DECKPAD_NATIVE_H
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/usbdevice_fs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

/* The header reader cannot evaluate ioctl macros containing sizeof. */
enum {
    deckpadUsbControl = USBDEVFS_CONTROL,
    deckpadUsbBulk = USBDEVFS_BULK,
    deckpadUsbClaim = USBDEVFS_CLAIMINTERFACE,
    deckpadUsbRelease = USBDEVFS_RELEASEINTERFACE
};
#endif
