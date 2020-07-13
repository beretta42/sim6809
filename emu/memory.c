/* memory.c -- Memory emulation
   Copyright (C) 1998 Jerome Thoen

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "emu6809.h"
#include "console.h"

#include "../hardware/hardware.h"
#include "../hardware/mmu.h"

tt_u8 *ramdata;    /* 64 kb of ram */

int memory_init(void)
{
  ramdata = (tt_u8 *)mmalloc(0x10000 * 16);
  memset(ramdata, 0, 0x10000 * 16);
  return 1;
}


tt_u8 get_memb(tt_u16 adr)
{
    uint8_t x;
    /* don't access hardware if we're not task 0 */
    if (tr == 0 && hard_ishard(adr)) return hard_get(adr);
    /* if we are switching tasks flip mmu back to user task
       and force return a 'rti' instruction
     */
    if (itr) {
	tr = itr;
	itr = 0;
	return 0x3b;
    }
    return ramdata[tr * 0x10000 + adr];
}

tt_u16 get_memw(tt_u16 adr)
{
  return (tt_u16)get_memb(adr) << 8 | (tt_u16)get_memb(adr + 1);
}

void set_memb(tt_u16 adr, tt_u8 val)
{
    if (watchpoint != -1 && adr == watchpoint) {
	printf("watchpoint hit, addr %x, val %x\n", adr, val);
	activate_console = 1;
    }
    if (tr == 0 && hard_ishard(adr)) hard_set(adr, val);
    ramdata[tr * 0x10000 + adr] = val;
}

void set_memw(tt_u16 adr, tt_u16 val)
{
  set_memb(adr, (tt_u8)(val >> 8));
  set_memb(adr + 1, (tt_u8)val);
}
