#ifndef _DAM_C_H_
#define _DAM_C_H_

#include <sys/types.h>
#include <stdint.h>

class dam_c {
public:

	dam_c(const char *dev=0) ;
	dam_c() {
		LOG(WARN,"dam_c") ;
	}
	~dam_c() ;

	int init() ;

	int dam_mode ;	// 0: use PCIe, 1:use USB
	int dam_ix ;
	volatile u_int *dam_ptr ;	// pointer to DAM's start of memory aka 0x4000_0000

	// statics
	static int dam_count ;

	static volatile u_int *resmem_glo ;
	static uint64_t resmem_bytes_glo ;
	static uint64_t resmem_hwaddr_glo ;

	volatile u_int *slv100 ;
	volatile u_int *slv160 ;
	volatile u_int *slv250 ;
	
	static char *hwicap_version(u_int v) ;

	u_int reg_r(int reg) ;
	u_int reg_w(int reg, u_int val) ;
	u_int reg_ws(int reg, int bit) ;
	u_int reg_wc(int reg, int bit) ;
	u_int reg_wp(int reg, int bit) ;




	int status() ;
	int clock_check(int do_log=1) ;

	void fast_cmd(u_char val) ;

	
	// lpgbt section
	u_char lpgbt_board_type ; //0:ETL, 1:ETL, 2:RBv1
	u_int lpgbt_dev_id ;
	u_int lpgbt_write(u_short reg, u_char val) ;
	u_int lpgbt_read(u_short reg) ;
	u_int lpgbt_rcv() ;

	int lpgbt_reset() ;


	int lpgbt_i2c_scan(int bus) ;
	u_short lpgbt_i2c_read(int bus, u_char dev, u_short reg) ;
	u_short lpgbt_i2c_write(int bus, u_char dev, u_short reg, u_char val) ;

	u_short lpgbt_gpio(int pin, int on) ;
	u_short lpgbt_gpio_init() ;
	u_short lpgbt_gpio_inited ;
	u_short lpgbt_gpio_cached ;

	int lpgbt_adc(int ch) ;
	int lpgbt_mux64(int ch) ;
	int lpgbt_vtrx() ;

	int lpgbt_dac() ;	// set DAC

	u_int lpgbt_chip_id() ;
	u_int lpgbt_user_id(u_int uid) ;

	int lpgbt_clocks() ;
	int lpgbt_edin() ;
	int lpgbt_edout() ;

	int lpgbt_init(int level) ;
	int lpgbt_send_config(int level) ;
	int lpgbt_run_start() ;
	int lpgbt_run_stop() ;
	int lpgbt_monitor(int level) ;

	int lpgbt_round_trip() ;

	struct {
		struct {
			u_short adc[16] ;
			u_int process_freq[4] ;
			u_short gpio ;
			u_short seu ;
			u_char pll_timeout[3] ;
			u_char wdog[3] ;
			u_char brownout ;
			u_char porbor ;
		} mon ;
	} lpgbt ;

	u_short lpgbt_etroc_rd(u_char dev, u_short reg) ;
	u_short lpgbt_etroc_wr(u_char dev, u_short reg, u_char val) ;

	// obscure
//	u_short fmc_si5345_config() ;
	u_int fmc_i2c_scan() ;

private:
	int usb_desc ;

	void i2c_scl(int on) ;
	void i2c_sda(int on) ;
	u_char i2c_sdz() ;
	void i2c_start() ;
	u_char i2c_stop() ;
	u_char i2c_write(u_char val) ;

	int i2c_device ;

	u_short i2c_reg_wr(u_char reg, u_char val) ;
//	u_short fmc_si5345_page(int page) ;


} ;



#endif
