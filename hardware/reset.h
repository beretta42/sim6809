/*
  reset hardware 

read: 
  bit 7 - interrupt flag (read clears)

write:
  bit 7 - enable interrupt
  bit 1 - stop emulation & exit simulator
  bit 0 - force reboot of machine  
*/


int reset_init(int argc, char *argv[]);
void reset_deinit(void);
void reset_run(void);
uint8_t reset_rreg(int reg);
void reset_wreg(int reg, uint8_t val);
