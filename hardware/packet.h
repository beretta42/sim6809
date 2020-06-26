int packet_init(int argc, char *argv[]);
void packet_run(void);
uint8_t packet_rreg(int reg);
void packet_wreg(int reg, uint8_t val);
void packet_deinit(void);
