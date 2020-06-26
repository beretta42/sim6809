#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <sys/select.h>

#include "../emu/config.h"
#include "acia.h"
#include "packet.h"
#include "timer.h"
#include "reset.h"

#define MAXFDS 10

int fds[MAXFDS];
int fdp = 0;
int maxfd = 0;


/* wait until one of the fd's above are ready to read 
   or a signals has been generated.
*/
void hard_wait(void) {
    int ret;
    fd_set s;
    int i;

    FD_ZERO(&s);
    for (i = 0; i < fdp; i++)
	FD_SET(fds[i], &s);

    ret = select(maxfd+1, &s, NULL, &s, NULL);
    if (ret < 1 && errno != EINTR)
	perror("hard_wait: select");
}

/* called by hardware to add file descriptors to be selected for 
   while waiting */
void hard_addfd(int fd) {
    if (fdp >= MAXFDS) {
	fprintf(stderr, "error: too many hardware files\n");
	exit(1);
    }
    fds[fdp++] = fd;
    if (fd > maxfd) maxfd = fd;
}

void hard_poll(void) {
    acia_run();
    packet_run();
    timer_run();
    reset_run();
}

void hard_init(int argc, char *argv[]) {
    fdp = 0;
    maxfd = 0;
    acia_init(argc, argv);
    packet_init(1);
    timer_init();
    reset_init(argc, argv);
}

void hard_deinit(void) {
    acia_deinit();
    packet_deinit();
    timer_deinit();
    reset_deinit();
}


/* get a byte from hardware */
uint8_t hard_get(uint16_t adr) {
  switch (adr & 0x00f0) {
  case 0x00:
      return acia_rreg(adr & 0xf);
  case 0x10:
      return packet_rreg(adr & 0xf);
  case 0x20:
      return timer_rreg(adr & 0xf);
  case 0x30:
      return reset_rreg(adr & 0xf);
  default:
      return 0;
  }
}

/* store a byte to hardware */
void hard_set(uint16_t adr, uint8_t val) {
    switch (adr & 0x00f0) {
    case 0x00:
        return acia_wreg(adr & 0xf, val);
    case 0x10:
	return packet_wreg(adr & 0xf, val);
    case 0x20:
	return timer_wreg(adr & 0xf, val);
    case 0x30:
	return reset_wreg(adr & 0xf, val);
    default:
        return;
    }
}
