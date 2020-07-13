SRCS = console.c dis6809.c emu6809.c inst6809.c int6809.c intel.c memory.c
SRCS += misc.c miscutils.c motorola.c ../hardware/acia.c ../hardware/packet.c
SRCS += ../hardware/timer.c ../hardware/hardware.c ../hardware/reset.c
SRCS += ../hardware/hd.c ../hardware/mmu.c

CFLAGS = -I. -lutil

all: sim6809

sim6809: $(SRCS)
	gcc $(CFLAGS) -o sim6809 $(SRCS) -lrt

clean:
	rm -f *~ sim6809 *.o
