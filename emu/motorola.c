/* vim: set noexpandtab ai ts=4 sw=4 tw=4:
   motorola.c -- Motorola S1 support
   Copyright (C) 1999 Noah Vawter

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
#include <string.h>
#include <ctype.h>
#include "motorola.h"
#include "config.h"
#include "emu6809.h"


static char buf[201];
static int ilen = 0;
static int ipos = 0;
static int cksum = 0;

static int hex_to_int(char x) {
    x=toupper(x);

    if( x>= '0' && x<= '9') return(x-'0');
    return(10+(x-'A'));
}

static int getn(void) {
    return hex_to_int(buf[ipos++]);
}

static int getb(void) {
    int i = (getn() << 4) + getn();
    cksum += i;
    return i;
}

static int mygetw(void) {
    return (getb() << 8) + getb();
}

int load_motos1(char *filename) {
    FILE *fi;
    int ret;
    printf("loading %s\n", filename);
    fi = fopen(filename,"r");
    if (fi==NULL) {
	printf("can't open it, sorry.\n");
	return(0);
    }
    ret = load_motos1_2(fi);
    fclose(fi);
    return ret;
}


int load_motos1_2(FILE *fi)
{
  int num_bytes;
  int i;
  int start_addr;
  int recnum = 0;

  while (fgets(buf,200,fi)) {
      ipos = 2;
      ilen = strlen(buf);
      if (ilen == 0) return 0;
      if (buf[0] == '.' ) return 0;
      if (buf[0] == 10 || buf[0] == 13) continue;
      if (ilen < 10) {
	  fprintf(stderr,"BAD SREC line length\n");
	  return -1;
      }
      if (buf[0] != 'S') {
	  fprintf(stderr,"SREC doesn't start with 'S'\n");
	  return -1;
      }
      cksum = 0;
      num_bytes = getb() - 3;
      start_addr=mygetw();

      switch (buf[1]) {
      case '0':
	  printf("S5 header: ");
	  for(i = 0; i < num_bytes; i++)
	      printf("%c", getb());
	  printf("\n");
	  break;
      case '1':
	  recnum++;
	  for(i=0;i<num_bytes;i++)
	      set_memb(start_addr++,getb());
	  break;
      case '5':
	  if (start_addr != recnum) {
	      fprintf(stderr,"ERROR: bad number of s-records\n");
	      return -1;
	  }
	  recnum = 0;
	  continue;
      case '9':
	  rpc = start_addr;
	  break;
      default:
	  fprintf(stderr,"unsupported srec type: %c\n", buf[1]);
	  return -1;
      }
      i = (~cksum) & 0xff;
      if (i != getb())
	  fprintf(stderr,"warning bad checksum in line %d\n", recnum);
  }
}
