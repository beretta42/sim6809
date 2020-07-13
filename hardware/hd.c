/*
  A harddrive

  this is a "magic" hd - blocks will magically
  be transferred to/from memory.

  reg 0 - 3:
    read/write: big endian LSN
  reg 4 - 5:
    read/write: big endian xfer address
  reg 6:
    read - bit 7 error, bit 6 media not present
    write -
	if bit 7 clear then lower bit are command:
	  0 - read sector
	  1 - write sector
	  2 - put hd size (in sectors) in the LSN regs
	if bit 7 is set
	  bit - 0:3 is sector size: 0 - 256, 1 - 512, ..., 7 - 32k

   wraps cpu memory
   doesn't write or read from hardware addresses, ram only
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "hd.h"
#include "hardware.h"

extern uint8_t *ramdata;

#define MIN(a,b) ((a < b) ? a : b)

static int fd;
static int secz;
static uint64_t lsn;
static uint16_t addr;
static int openf;
static int error;


static void size(void) {
    off_t ret = lseek(fd, 0, SEEK_END);
    if (ret < 0) {
	error = 1;
	perror("hd: in size(): lseek");
	return;
    }
    lsn = (ret + 255) / secz;
}

static void xfer(int way) {
    int left;
    int x;
    off_t ret;

    ret = lseek(fd, lsn*secz, SEEK_SET);
    if (ret < 0) {
	perror("hd: in xfer(): lseek");
	error = 1;
	return;
    }
    left = secz;
    while (left) {
	x = MIN(left, (0x10000 - addr));
	if (way)
	    ret = write(fd, ramdata+addr, x);
	else
	    ret = read(fd, ramdata+addr, x);
	if (ret == 0) {
	    printf("hd: file closed!");
	    error = 1;
	    return;
	}
	if (ret < 0) {
	    if (errno != EINTR) {
		perror("hd: in read(): read");
		error = 1;
		return;
	    }
	    ret = 0;
	}
	left -= ret;
	addr = (addr + ret) & 0xffff;
    }
    return;
}



int hd_init(int argc, char *argv[]) {
    char *filename;
    int x;

    secz = 256;
    openf = 0;
    lsn = 0;
    addr = 0;
    error = 0;

    filename = "harddrive.img";
    for(x = 1; x < argc; x++) {
	if (!strncmp(argv[x],"-dhd:file:",10)) {
	    filename = argv[x] + 10;
	}
    }
    fd = open(filename, O_RDWR|O_CREAT, 0644);
    if (fd < 0) {
	perror("in hd_init(): open");
	return -1;
    }
    openf = 1;
    return 0;
}


void hd_deinit() {
    close(fd);
}

void hd_run() {
    return;
}

uint8_t hd_rreg(int reg) {
    int val;
    switch(reg) {
    case 0:
	return (lsn >> 24) & 0xff;
    case 1:
	return (lsn >> 16) & 0xff;
    case 2:
	return (lsn >>  8) & 0xff;
    case 3:
	return lsn & 0xff;
    case 4:
	return (addr >> 8) & 0xff;
    case 5:
	return addr & 0xff;
    case 6:
	val = 0;
	if (error)  val | 0x80;
	if (!openf) val | 0x40;
	return val;
    default:
	return 0;
    }
}

void hd_wreg(int reg, uint8_t val) {
    switch (reg) {
    case 0:
	lsn = (lsn & 0x00ffffff) | (val<<24L);
	break;
    case 1:
	lsn = (lsn & 0xff00ffff) | (val<<16L);
	break;
    case 2:
	lsn = (lsn & 0xffff00ff) | (val<<8L);
	break;
    case 3:
	lsn = (lsn & 0xffffff00) | val;
	break;
    case 4:
	addr = (addr & 0x00ff) | (val<<8L);
	break;
    case 5:
	addr = (addr & 0xff00) | val;
	break;
    case 6:
	if (val & 0x80) {
	    secz = 256L << (val & 7);
	}
	else {
	    error = 0;
	    switch(val) {
	    case 0:
		xfer(0); return;
	    case 1:
		xfer(1); return;
	    case 2:
		size(); return;
	    }
	}
	return;
    }
}
