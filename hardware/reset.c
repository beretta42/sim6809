#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <limits.h>
#include <sys/inotify.h>

#include "hardware.h"
#include "reset.h"
#include "../emu/motorola.h"
#include "../emu/config.h"
#include "../emu/emu6809.h"

#define O_RESET  1
#define O_STOP   2
#define O_IE     128


static char *start = NULL;
static int nfd;
static int irqen = 0;
static int irqf  = 0;


static void reboot(void) {
    m6809_init();
    memory_init();
    load_motos1(start);
    hard_reinit();
    reset();
}


int reset_init(int argc, char *argv[]) {
    int i;
    int ret;

    if (start == NULL) {
	for (i = 1; i < argc; i++) {
	    if (argv[i][0] != '-') {
		start = argv[i];
		break;
	    }
	}
    }
    
    ret = inotify_init1(IN_NONBLOCK);
    if (ret < 0) {
	perror("in reset_init(): inotify_init");
	return 1;
    }
    nfd = ret;

    ret = inotify_add_watch(nfd, start, IN_MODIFY);

    irqen = 0;
    irqf = 0;
    
    return 0;
}

void reset_deinit(void) {
    close(nfd);
}

#define NMAX (sizeof(struct inotify_event)+ NAME_MAX + 1)
void reset_run(void) {
    int buf[NMAX];
    int ret;

    if (irqen && irqf) irq();
    /* see if read works on our watched file */
    ret = read(nfd, buf, NMAX);
    if (ret < 1) return;
    /* our start.s file has changed  */
    irqf = 1;
    if (irqen == 0){
	fprintf(stderr,"*** REBOOT: change in start file\n");
	reboot();
    }
}

uint8_t reset_rreg(int adr) {
    int i = 0;
    if (irqf) i |= O_IE;
    irqf = 0;
    return i;
}

void reset_wreg(int adr, uint8_t val) {
    irqen = val & O_IE;
    if (val & O_RESET) {
	fprintf(stderr,"*** REBOOT: coco forced\n");
	reboot();
    }
    if (val & O_STOP) {
	fprintf(stderr,"*** process stoped\n");
	exit(0);
    }
}
