#ifdef _ZCU106_C_
#else

#define _ZCU106_C_	1

class zcu106_c {
public:
	zcu106_c() ;
	~zcu106_c() ;

	int dev_open(const char *usbtty) ;

	int set_flavor(int asic_flavor) ;
	int set_trg_type(int type) ;
	int get_event(u_int *buffer) ;


	int use_i2c_addr(u_char addr) ;
	int set_default_regs(const char *fname) ;



	int asic_flavor ;	// 0:EICROC0, 1:EICROC1
	int trg_type ;		// 0:pedestal...

	struct regs_t {
		u_short reg ;
		u_short val ;
	} regs[1024] ;

	int regs_cou ;

	u_int rd(int reg) ;
	u_int wr(int reg, u_int val) ;
	u_short i2c_wr(u_short reg, u_char val) ;
	u_short i2c_rd(u_short reg) ;
	
private:
	int ser_read(u_char *ch) ;
	int ser_ln_read(char *buff) ;
	int ser_write_b(const void *buffer,int bytes) ;
	int ser_write(const char *buffer) ;

	int open_eicroc0_regs(const char *fname) ;
	int open_py(const char *fname) ;

	int ser_usb ;

//	int wr() ;
//	int rd() ;

} ;


#endif
