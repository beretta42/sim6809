/*
  This is a timer that fires IRQ's
  write address 0:
    bit 7 enables timer
    bit 0-6 interval
        0       interval = 16.666666 ms (~60hz)
        1-100   interval in ms
	101     1 secs
        102     5 secs
        103     10 secs
        104     30 secs
        105     60 secs
        106-128 reserved (disables timer)

  read address 0 (clears interrupt):
    bit 7 set if interrupt

*/

#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>

#include "../emu/config.h"
#include "../emu/emu6809.h"


static volatile int iflg = 0;

static uint8_t creg = 0;
#define EN_INT 0x80

static timer_t t;


static void do_vtalrm(int sig) {
    iflg = 1;
    signal(SIGVTALRM, do_vtalrm);
}

int timer_init(void) {
    int ret;
    struct sigevent s;

    creg = 0;
    signal(SIGVTALRM, do_vtalrm);
    s.sigev_notify = SIGEV_SIGNAL;
    s.sigev_signo = SIGVTALRM;
    ret = timer_create(CLOCK_MONOTONIC, &s, &t);
    if (ret < 0) perror("timer_init: timer_create");
    return 0;
}

void timer_deinit(void) {
    timer_delete(t);
}

void timer_run(void) {
    int ret;
    if (iflg) {
	if (creg & EN_INT)
	    irq();
    }
}

uint8_t timer_rreg(int reg) {
    if (iflg) {
	iflg = 0;
	return EN_INT;
    }
    else return 0;
}


void timer_wreg(int reg, uint8_t val) {
    int ret;
    struct itimerspec it;
    long i;
    int s = 0;

    creg = val & EN_INT;

    i = (val & ~EN_INT);
    if (creg == 0)
	i = 0;
    else if (i == 0)
	i = 16666666;
    else if (i <= 100)
	i = i * 1000000;
    else {
	switch (i) {
	case 101: s = 1; break;
	case 102: s = 5; break;
	case 103: s = 10; break;
	case 104: s = 30; break;
	case 105: s = 60; break;
	default: break;
	}
	i = 0;
    }
    it.it_interval.tv_sec = s;
    it.it_interval.tv_nsec = i;
    it.it_value.tv_sec = s;
    it.it_value.tv_nsec = i;
    ret = timer_settime(t, 0, &it, NULL);
    if (ret < 0) perror("timer_init: timer_settime");
}
