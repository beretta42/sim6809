/* a simple MMU

   This *may* be possible from a real 6809 CPU - see MC6829 data sheet.

   all regs are write only
   
   0 - set task register and force a RTI
   1 - set copier's from task space
   2 - set copier's to task space
   3:4 - set copier's from address
   5:6 - set copier's to address
   7:8 - set copier's no of bytes
 
   writing reg 8 cause the memory transfer

   FIXME: copier need to deal with the wrapping of tasks' space memory

 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mmu.h"

int tr;
int itr;

extern uint8_t *ramdata;

static uint8_t  ftask;
static uint8_t  ttask;
static uint16_t faddr;
static uint16_t taddr;
static uint16_t size;

int mmu_init(int argc, char *argv[]) {
    tr = 0;
    itr = 0;
    return 0;
}


void mmu_wreg(int reg, uint8_t val) {
    switch(reg) {
    case 0:
	itr = val & 0xf; return;
    case 1:
	ftask = val & 0xf; return;
    case 2:
	ttask = val & 0xf; return;
    case 3:
	faddr = (faddr & 0x00ff) | (val << 8L); return;
    case 4:
	faddr = (faddr & 0xff00) | val; return;
    case 5:
	taddr = (taddr & 0x00ff) | (val << 8L); return;
    case 6:
	taddr = (taddr & 0xff00) | val; return;
    case 7:
	size = (size & 0x00ff) | (val << 8L); return;
    case 8:
	size = (size & 0xfff0) | val;
	/* once we write reg 8 initiate copy */
	memcpy(ramdata + (0x10000 * ttask + taddr),
	       ramdata + (0x10000 * ftask + faddr),
	       size);
    }
}
