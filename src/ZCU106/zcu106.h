#ifndef _ZCU106_H_
#define _ZCU106_H_

#include <sys/types.h>

extern u_int rd(u_int reg) ;
extern u_int wr(u_int reg, u_int val) ;

extern u_short i2c_wr(u_short reg, u_char val) ;
extern u_short i2c_rd(u_short reg) ;

extern int reg_cou ;
extern struct reg_t {
	u_short reg ;
	u_char val ;
} regs[6000] ;

extern int open_py(const char *fname) ;

extern int run_asic(FILE *f, int nevents) ;


#define ASIC_EICROC0A	0
#define ASIC_EICROC1	1	// with metal bug
#define ASIC_EICROC2	2	// future very first version of EICROC2
#define ASIC_EICROC1_F	9	// with metal fix
#define ASIC_EICROC0	10

#define RUN_TYPE_PEDESTAL	0
#define RUN_TYPE_PULSER		2	// CMDPULSE, no TRIGOUT wait
#define RUN_TYPE_PHYS		3	// TRIGOUT wait
#define RUN_TYPE_PHYS_B		4	// external trigger wait
#degine RUN_TYPE_PULSER_B	20	// CMDPULSE, with TRIGOUT wait

#endif

