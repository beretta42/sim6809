/*
   packet driver

   read device addresses:
   0  - status
   1  - rx packet length: high byte
   2  - rx packet length: low byte
   3  - data next rx packet dataum
   write device addresses:
   0  - data tx
   1  - xmit packet
   2  - drop rx packet

   todo:
   * argv/argv init parameters (tap dev name)
   * interrupt on tx empty (why?)
   * MAC filtering
   * auto MRU/MTU filtering
   * pass back a fd for select()/poll() efficiency
   * allow setting max backlog of tap iface

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include "../emu/config.h"
#include "../emu/emu6809.h"
#include "hardware.h"


#define MAXBUF 2048

static char obuf[MAXBUF];
static uint16_t olen;


static char ibuf[MAXBUF];
static char *ipos;
static uint16_t ilen;

static int tunfd;
static char tundev[256];

static uint8_t status;
static int irqen = 0;

#define ST_RX     1
#define ST_TX     2
#define ST_INT  128


/* Allocate (open) a new tap device
   Take symbolic name of device and tap/tun flags. Returns fd, or < 0 on
   error.
*/
static int tun_alloc(char *dev, int flags)
{
    struct ifreq ifr;
    int fd, err;
    char *clonedev = "/dev/net/tun";

    /* open the clone device */
    if ((fd = open(clonedev, O_RDWR|O_NONBLOCK)) < 0)
    {
	perror("tun_alloc");
	return fd;
    }
    /* preparation of the struct ifr, of type "struct ifreq" */
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = flags;
    if (*dev)
    {
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }
    /* try to create the device */
    if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0)
    {
	perror("in tun_alloc(): ioctl");
	close(fd);
	return err;
    }
    /* write acquired tap device name back to struct */
    strcpy(dev, ifr.ifr_name);

    return fd;
}

int packet_init(int argc, char *argv[]) {
    char cmd[256];
    int i;
    for (i = 1; i < argc; i++) {
	if (!strncmp(argv[i],"-dpkt:dev:", 10)) {
	    strcpy(tundev, argv[i]+10);
	    goto go;
	}
    }
    strcpy(tundev, "tap0");
 go:
    tunfd = tun_alloc(tundev, IFF_TAP | IFF_NO_PI);
    if (tunfd < 0) {
	fprintf(stderr,"%s: Cannot open tunnel\n", tundev);
	return -1;
    }
    else
	fprintf(stderr,"opened tap device: %s\n", tundev);
    hard_addfd(tunfd);

    if ( getpid() == 0) {
	sprintf(cmd, "ip link set %s up", tundev);
	system(cmd);
    }
    olen = 0;
    ilen = 0;
    ipos = ibuf;
    status = 0;
    irqen = 0;
}

void packet_deinit(void) {
    close(tunfd);
}

void packet_run(void) {
    int ret;
    if (ilen == 0) {
	ret = read(tunfd, ibuf, MAXBUF);
	if (ret <= 0) return;
	ilen = ret;
	ipos = ibuf;
	status |= ST_INT | ST_RX;
    }
    if (status & ST_INT && irqen)
	irq();
}

uint8_t packet_rreg(int reg) {
    int ret;
    switch(reg & 3) {
    case 0:
	ret = status;
	status &= ~ST_INT;
	return ret;
    case 1:
	return ilen >> 8;
    case 2:
	return ilen & 255;
    case 3:
	if (ilen) {
	    if (--ilen == 0) status &= ~ST_RX;
	    return *ipos++;
	}
	return 0;
    }
}


void packet_wreg(int reg, uint8_t val) {
    switch(reg & 3) {
    case 0:
	obuf[olen++] = val;
	break;
    case 1:
	write(tunfd, obuf, olen);
	olen = 0;
	break;
    case 2:
	status &= ~(ST_INT | ST_RX);
	ilen = 0;
	break;
    case 3:
	irqen = val & ST_INT;
	break;
    }
}
