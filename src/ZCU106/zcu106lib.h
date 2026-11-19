#ifndef _ZCU106LIB_H_
#define _ZCU106LIB_H_

extern int zcu_reg_dump() ;

extern int zcu_ser_open(const char *dev) ;

extern u_short zcu_i2c_set_dev(u_char dev) ;

extern u_short zcu_i2c_rd(u_short reg) ;
extern u_short zcu_i2c_wr(u_short reg, u_char val) ;
extern u_int zcu_wr(u_int reg, u_int val) ;
extern u_int zcu_rd(u_int reg) ;



#endif

