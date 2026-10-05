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
} regs[1034] ;

extern int open_py(const char *fname) ;

extern int run_asic(FILE *f, int nevents) ;

#endif

