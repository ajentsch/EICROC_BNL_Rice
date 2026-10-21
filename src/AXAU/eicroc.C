#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include <LOG/rtsLog.h>
volatile int rtsLogLevel = 1 ;

#include "dam_c.h"

static int sig_on ;
static dam_c *dam ;


static int reg_cou ;
struct reg_t {
	u_short reg ;
	u_char val ;
} ;
static struct reg_t regs[6000] ;
// this will get folded into dam_c.C later...
static u_char i2c_glo_shadow[256] ;



static int open_py(const char *fname) ;


static void sighandler(int sig)
{
	if(sig==2) {
		LOG(WARN,"Cntrl-C: EXITING.") ;
	}
	else {
		LOG(ERR,"Signal %d: EXITING.",sig) ;
	}

	sig_on = sig ;
}

//static void adc_print() ;




u_short eicr_wr(u_short reg, u_char value)
{
	u_short dev = 0x40 ;	// device
	int devs ;

	// lpGBT Bus #1 registers and constants
	int data0 = 0x109 ;
	int cmd = 0x10d ;
	int addr = 0x108 ;
	int status = 0x186 ;
//	int read0 = 0x188 ;

	dam->lpgbt_write(data0,(1<<2)|(1<<0)) ;
	dam->lpgbt_write(cmd,0) ;	// write this in


	dam->lpgbt_write(addr,dev) ;		// device
	dam->lpgbt_write(data0,reg&0xFF) ;	// lower 8 bits of address
	dam->lpgbt_write(cmd,0x2) ;		// 1-byte write


	devs = 0 ;
	for(int i=0;i<10;i++) {
			
		u_int val = dam->lpgbt_read(status) ;
		if(val==0) continue ;

		if(val != 0x40) {
			if(val & 0x04) {
				//LOG(INFO,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
				devs++ ;
				break ;
			}
			else {
				LOG(WARN,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
			}
		}
	}


	if(devs==0) {
		LOG(ERR,"EICR: I2C write 0x%04X",reg) ;
		return 0xFFFF ;
	}

	dam->lpgbt_write(addr,dev+1) ;		// device+1
	dam->lpgbt_write(data0,reg>>8) ;	// upper 8 bits of address ;
	dam->lpgbt_write(cmd,0x2) ;		// 1-byte write


	devs = 0 ;
	for(int i=0;i<10;i++) {
			
		u_int val = dam->lpgbt_read(status) ;
		if(val==0) continue ;

		if(val != 0x40) {
			if(val & 0x04) {
				//LOG(INFO,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
				devs++ ;
				break ;
			}
			else {
				LOG(WARN,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
			}
		}
	}


	if(devs==0) {
		LOG(ERR,"EICR: I2C write 0x%04X",reg) ;
		return 0xFFFF ;
	}

	dam->lpgbt_write(addr,dev+2) ;		// device+2
	dam->lpgbt_write(data0, value) ;		// value
	dam->lpgbt_write(cmd,0x2) ;		// 1-byte write


	devs = 0 ;
	for(int i=0;i<10;i++) {
			
		u_int val = dam->lpgbt_read(status) ;
		if(val==0) continue ;

		if(val != 0x40) {
			if(val & 0x04) {
				//LOG(INFO,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
				devs++ ;
				break ;
			}
			else {
				LOG(WARN,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
			}
		}
	}


	if(devs==0) {
		LOG(ERR,"EICR: I2C write 0x%04X",reg) ;
		return 0xFFFF ;
	}


	if((reg&0xFF00)==0x4000) {
		i2c_glo_shadow[reg&0xFF] = value ;
	}

	return 0 ;
}

u_short eicr_rd(u_short reg)
{
	u_short dev = 0x40 ;	// device
	u_short val ;
	int devs ;

	// lpGBT Bus #1 registers and constants
	int data0 = 0x109 ;
	int cmd = 0x10d ;
	int addr = 0x108 ;
	int status = 0x186 ;
	int read0 = 0x188 ;

	dam->lpgbt_write(data0,(1<<2)|(1<<0)) ;
	dam->lpgbt_write(cmd,0) ;	// write this in


	dam->lpgbt_write(addr,dev) ;		// device
	dam->lpgbt_write(data0,reg&0xFF) ;	// lower 8 bits of address
	dam->lpgbt_write(cmd,0x2) ;		// 1-byte write


	devs = 0 ;
	for(int i=0;i<10;i++) {
			
		u_int val = dam->lpgbt_read(status) ;
		if(val==0) continue ;

		if(val != 0x40) {
			if(val & 0x04) {
				//LOG(INFO,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
				devs++ ;
				break ;
			}
			else {
				LOG(WARN,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
			}
		}
	}


	if(devs==0) {
		LOG(ERR,"EICR: I2C read 0x%04X",reg) ;
		return 0xFFFF ;
	}

	dam->lpgbt_write(addr,dev+1) ;		// device+1
	dam->lpgbt_write(data0,reg>>8) ;	// upper 8 bits of address ;
	dam->lpgbt_write(cmd,0x2) ;		// 1-byte write


	devs = 0 ;
	for(int i=0;i<10;i++) {
			
		u_int val = dam->lpgbt_read(status) ;
		if(val==0) continue ;

		if(val != 0x40) {
			if(val & 0x04) {
				//LOG(INFO,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
				devs++ ;
				break ;
			}
			else {
				LOG(WARN,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
			}
		}
	}


	if(devs==0) {
		LOG(ERR,"EICR: I2C read 0x%04X",reg) ;
		return 0xFFFF ;
	}

	dam->lpgbt_write(addr,dev+2) ;		// device+2
	dam->lpgbt_write(cmd,0x3) ;		// 1-byte read


	devs = 0 ;
	for(int i=0;i<10;i++) {
			
		u_int val = dam->lpgbt_read(status) ;
		if(val==0) continue ;

		if(val != 0x40) {
			if(val & 0x04) {
				//LOG(INFO,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
				devs++ ;
				break ;
			}
			else {
				LOG(WARN,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
			}
		}
	}


	if(devs==0) {
		LOG(ERR,"EICR: I2C read 0x%04X",reg) ;
		return 0xFFFF ;
	}


	val = dam->lpgbt_read(read0) ;
	LOG(DBG,"EICROC1: I2C R: reg 0x%04X = 0x%02X",reg,val) ;
	return val ;
}


int main(int argc, char *argv[])
{
//	dam_c *dam ;
	int c ;


	const char *usb_dev = 0 ;
	u_int mode = 0 ;
	u_int rb_mode = 0 ;

	int num_events = 1 ;

	int en_acq_to_cmd = 12 ;
	int cmd_to_end_acq = 4 ;

	const char *eicroc_cfg_c = "../CONF/e1_orig_reg_values.py" ;

	rtsLogLevel = 2 ;	// default

	int asic_type = 1 ;
	int run_type = 0 ;
	int lane_mask = 1 ;	// just the first lane

	int board_type = 3 ;	//3=RBv1 with bad EICROC1

	u_int glo_threshold = 0x300 ;

	signal(2,sighandler) ;
	

	while((c=getopt(argc,argv,"D:d:m:n:W:w:M:C:A:t:l:G:")) != EOF) {
	switch(c) {
	case 'D' :
		rtsLogLevel = atoi(optarg) ;
		break ;
	case 'd' :
		usb_dev = optarg ;
		break ;
	case 'm' :
		if(sscanf(optarg,"0x%X",&mode)==0) {
			mode = atoi(optarg) ;
		}
		break ;
	case 'n' :
		num_events = atoi(optarg) ;
		break ;
	case 'M' :
		if(sscanf(optarg,"0x%X",&rb_mode)==0) {
			rb_mode = atoi(optarg) ;
		}
		break ;
	case 'W' :
		en_acq_to_cmd = atoi(optarg) ;
		break ;
	case 'w' :
		cmd_to_end_acq = atoi(optarg) ;
		break ;
	case 'C' :
		eicroc_cfg_c = optarg ;
		break ;
	case 'A' :
		asic_type = atoi(optarg) ;
		break ;
	case 't' :
		run_type = atoi(optarg) ;
		break ;
	case 'l' :
		if(sscanf(optarg,"0x%X",&lane_mask)==0) {
			lane_mask = atoi(optarg) ;
		}
		break ;
	case 'G' :
		glo_threshold = atoi(optarg) ;
		break ;

	}
	}


	LOG(INFO,"%s at %s %s: RB %s, %s",argv[0],__TIME__,__DATE__,
	    board_type==3?"RBv1+EICROC1":"UNKNOWN",
	    asic_type==1?"EICROC1noFix":"EICROC1wfix") ;
	
	dam = new dam_c(usb_dev) ;	// init PCIe version if NULL


	dam->lpgbt_board_type = board_type ;	// 3= RBv1 w EICROC1; must be set before init!


	LOG(INFO,"Running dam.init()") ;
	dam->init() ;

	if(rb_mode & 8) {	// DUMP all AXAU15 registers... for debugging
		u_short reg ;

		for(int i=0;i<32;i++) {
			reg = i ;
			u_int v = dam->reg_r(reg) ;

			printf("slv100 %2d[0x%03X] = 0x%08X[%u]\n",i,reg,v,v) ;
		}

		for(int i=0;i<32;i++) {
			const char *cc ;

			reg = 0x400+i ;
			u_int v = dam->reg_r(reg) ;

			if((i>=0)&&(i<=7)) cc = "RW" ;
			else if((i>=8)&&(i<=15)) cc = "-" ;
			else if((i>=16)&&(i<=23)) cc = "RO" ;
			else cc = "xx" ;

			printf("slv160 %2d[0x%03X] = 0x%08X[%u] %s\n",i,reg,v,v,cc) ;
		}

		for(int i=0;i<16;i++) {
			reg = 0x800+i ;
			u_int v = dam->reg_r(reg) ;

			printf("slv250 %2d[0x%03X] = 0x%08X[%u]\n",i,reg,v,v) ;
		}
	}


	LOG(INFO,"Running dam.status()") ;
	dam->status() ;

	if(rb_mode & 4) {	// reset lpGBT first
		LOG(WARN,"Running dam.lpgbt_reset") ;
		if(dam->lpgbt_reset()==0) {
			LOG(ERR,"Reset lpGBT failed. Exiting!") ;
			return -1 ;
		}
	}

	LOG(INFO,"Running dam.lpgbt_init()") ;
	dam->lpgbt_init(0) ;



	if(mode & 4) {	// reset EICROC1
		u_short gpio ;

		// keep EICROC1 in reset wiggling GPIO
		LOG(WARN,"Resetting EICROC1") ;

		dam->lpgbt_gpio(15,1) ;			// SEL_FCMD=1 ; IMPORTANT TO DO BEFORE RESET

		dam->lpgbt_gpio(8,0) ;	// RESETB=0
		dam->lpgbt_gpio(11,0) ;	// RST_I2C=0


		usleep(10000) ;

		dam->lpgbt_gpio(11,1) ;	// RST_I2C=1
		gpio = dam->lpgbt_gpio(8,1) ;	// RESETB=1

		LOG(NOTE,"GPIO is now 0x%04X",gpio) ;
	
		usleep(10000) ;		// give it time after reset
	}


	if(mode & 1) {	// full configure EICROC1

		// EDOUT00 is FCMD
		// EDIN00 is SDOUT data
		// EDIN02 is DOUT0 but not expected to use it
		// ECLK0 is 320 MHz
		// ECLK1 is 160 MHz but won't use it


		// first configure AXAU15 registers
		dam->reg_w(1,0x3636) ;			// slv100_o(1): set FCMD: lower 8bits: IDLE, upper 8bit: BCR


		LOG(INFO,"EICROC1 en_acq_to_cmd %d, cmd_to_end_acq %d",en_acq_to_cmd,cmd_to_end_acq) ;

		dam->reg_w(0x400+2, (cmd_to_end_acq<<8)|(en_acq_to_cmd<<0)) ;			// setup to slv160_i(2): mode32 register of eicroc_fcmd
		dam->reg_w(2,40000000) ;							// set tick timer! actually 28 bits only



		// EICROC1 I2C configuration
		if(usb_dev==0) {
			for(int r=0x4000;r<=0x401B;r++) {
				u_int v = eicr_rd(r) ;
				LOG(NOTE,"EICROC default reg 0x%04X = 0x%02X",r,v) ;
			}
		}

		// global register sets
		LOG(INFO,"Using defaults file %s",eicroc_cfg_c) ;
		open_py(eicroc_cfg_c) ;

		LOG(WARN,"Writing file values to EICROC via I2C -- might cause resets!") ;
		for(int i=0;i<reg_cou;i++) {
			u_int reg = regs[i].reg ;

			if(reg<0x4000) continue ;

			eicr_wr(reg,regs[i].val) ;
		}

		
		// defaults for A=0xFF, B=0x63, C=0xC0 ;
		//A 10bitDAC[7..0]	lower 8 bits of common discriminator threshold
		//B bbbb_bbdd            bb is dac_biaspa, dd is uppermost 2 bits of the discriminator
		//C 0b11pp_pppp		pp_pppp is pulser height but reverse: 0 is the largest

		//u_int glo_threshold = 0xF0F ;	// 0xF
		u_int pulser = 0x3F ;		

		glo_threshold &= 0x3FF ;	// 10 bits
	
		LOG(INFO,"Pulser 0x%02X, global threshold %d",pulser, glo_threshold) ;

		pulser = (0x3F - pulser)&0x3F ;	// 6 bits, reversed

//		u_int aa = glo_threshold&0xFF ;
//		u_int bb = 0x60|((glo_threshold>>8)&3) ;
//		u_int cc = 0xC0|pulser ;
//		LOG(TERR,"%02x %02x %02x",aa,bb,cc) ;

		eicr_wr(0x400A,glo_threshold&0xFF) ;	// 0xFF
		eicr_wr(0x400B,0x60|((glo_threshold>>8)&3)) ;	// 0x63
		eicr_wr(0x400C,0xC0|pulser) ;	// 0xC0


//		eicr_wr(0x400A,0xFF) ;	// 0xFF
//		eicr_wr(0x400B,0x63) ;	// 0x63
//		eicr_wr(0x400C,0xC0) ;	// 0xC0


		// lane clock enable machinations
		// setup EICROC1 lanes
		u_char msk ;

		msk = 0 ;

		if(lane_mask&1) {
			msk |= 0x0F ;
		}
		if(lane_mask&2) {
			msk |= 0xF0 ;
		}

		eicr_wr(0x4013,msk) ;	// enable lanes aka columns 0..7

		msk = 0 ;

		if(lane_mask&4) {
			msk |= 0x0F ;
		}
		if(lane_mask&8) {
			msk |= 0xF0 ;
		}

		eicr_wr(0x4014,msk) ;	// enable lanes aka columns 0..7

		msk = 0 ;

		if(lane_mask&0x10) {
			msk |= 0x0F ;
		}
		if(lane_mask&0x20) {
			msk |= 0xF0 ;
		}

		eicr_wr(0x4015,msk) ;	// enable lanes aka columns 0..7

		msk = 0 ;

		if(lane_mask&0x40) {
			msk |= 0x0F ;
		}
		if(lane_mask&0x80) {
			msk |= 0xF0 ;
		}

		eicr_wr(0x4016,msk) ;	// enable lanes aka columns 0..7


		LOG(INFO,"Enabling lanes 0x%02X",lane_mask) ;



		// local registers MUST come last!

		const u_int vref_use = 80 ;
		const u_int vth_use = 0 ;	// some reasonable default

		int vref = vref_use & 0x7F ;	// only 7 bits
		int on_ctest = 0 ;
		int vth = vth_use & 0x7F ;	// only 7 bits

		switch(run_type) {
		case 0 :
			vth = 0 ;
			vref = 0 ;
			break ;
		case 1 :
			vth = 0 ;
			break ;
		case 2 :
		case 20 :
			on_ctest = 0x04 ;			
			break ;
		}

		eicr_wr(0x0001,0x80|vth) ;	// discriminator threshold
		eicr_wr(0x0002,vref) ;
		eicr_wr(0x0003,on_ctest) ;
		eicr_wr(0x0004,0x01) ;	// EICROC0A was 0x29 (EN_TDC)
		eicr_wr(0x0005,0x20) ;	// EICROC0A was 0x00 (EN_TUNE_GAIN_ADC)

//		eicr_wr(0x0004,0x29) ;	// EICROC0A was 0x29 (EN_TDC)
//		eicr_wr(0x0005,0x00) ;	// EICROC0A was 0x00 (EN_TUNE_GAIN_ADC)


		LOG(INFO,"Vref %d, Vth %d, on-ctest %c",vref,vth,on_ctest?'Y':'N') ;

		// I2C verification
		for(int r=0x4000;r<=0x401B;r++) {
			u_int v = eicr_rd(r) ;

			if(v != i2c_glo_shadow[r&0xFF]) {
				LOG(WARN,"EICROC reg 0x%04X: is 0x%02X, expect 0x%02X",r,v,i2c_glo_shadow[r&0xFF]) ;
			}
			else {
				LOG(NOTE,"EICROC reg 0x%04X = 0x%02X",r,v) ;
			}

			if(usb_dev) break ;	// alow just 1 verification
		}

	}

	// take data...
	if(mode & 2) {
		int evt = 0 ;
		int cou = 0 ;
		u_int cmd_mode = 4 ;
		u_int rdout_mode = 0 ;


		dam->reg_w(0,0) ;	// slave 100 clear: stops previous run

		dam->reg_w(0x400,0) ;	// stops GTU tslices

		dam->reg_w(0x400+1,0x06050401) ;		// GEO Id
		//dam->reg_w(0x400+3,0xFFFFFFFF) ;		// ASIC mask

		// reset MEM_P and enable readout over PCIe
		dam->reg_wp(0x800,1) ;	// reset MEM_P FIFO
		dam->reg_w(0x801,0x80000000) ;	// MEM_P address
		dam->reg_wp(0x800,0) ;	// pulse MEM_P WR_EN
		dam->reg_w(0x800,(1<<16)) ;	// CPU readout

		// setup EICROC1 control
		// cmd_mode: 0 fire CMDPLLSE, 4 don't fire
		// rdout_mode: 

		const char *crt = "PEDESTAL" ;
		switch(run_type) {
		case 0 :
		default :
			cmd_mode = 4 ;
			rdout_mode = 0 ;
			crt = "PEDESTAL" ;
			break ;
		case 2 :
			cmd_mode = 0 ;
			rdout_mode = 0 ;
			crt = "PULSER" ;
			break ;
		case 3 :
			cmd_mode = 4 ;
			rdout_mode = 4 ;	// wait for external trigger
			crt = "PHYSICS" ;
			break ;
		case 4 :
			cmd_mode = 4 ;
			rdout_mode = 3 ;	// also wait for external trigger
			crt = "PHYSICS" ;
			break ;
		case 20 :
			cmd_mode = 0 ;
			rdout_mode = 3 ;
			crt = "PULSER" ;
			break ;
		}

		LOG(INFO,"Run-type %s[%d]: %s %s, events %d",crt,run_type,
		    cmd_mode==0?"CMDPULSE":"",
		    rdout_mode==4?"EXT-TRIGGER":"",
		    num_events) ;

		// WARN: bit 30 resets the FCMD state machine
		dam->reg_wp(0x400+2,30) ;

		u_int v = dam->reg_r(0x400+2) & 0xFFFF ;	// keep delays intact
		dam->reg_w(0x400+2, v | (cmd_mode<<16)|(rdout_mode<<19)) ;





		// cautiously reset: pci iface, pci_fifo_srst, asic_arbiter
		// NOT gt_iface, NOT reg_iface
		// 4: gt_iface_reset
		// 5: ?
		// 6: lpgbt_reset
		// 7: reset_slave_160
		// 8: pci_fifo_srst
		// 9: asic_arbiter_reset
		// 16,17 I2C

		// rc_i is slv100_0(0)(7..0)) ;

		dam->reg_w(0,(1<<5)|(1<<8)|(1<<9)) ;	// reset All
		usleep(1000) ;						// keep in reset a bit
		dam->reg_w(0,0) ;					// un-reset



		
		time_t now = time(0) ;
		time_t now1 = now ;

		// start run
		dam->reg_w(0,(1<<0)) ;		// goto running state
		dam->reg_w(0x400,(1<<20)) ;	// GTU tslice start



		// fire EICROC1
		LOG(INFO,"Fire 1st event...") ;
		dam->reg_wp(0x400+2,31) ;	// GO!


		u_int empty_cou = 0 ;


		dam->reg_wc(0x800,17) ;			// make sure the bit is cleared
		u_int r800 = dam->reg_r(0x800) ;	// cache thr calue

		for(u_int ic=0;ic<0xFFFFFF0;ic++) {
			u_int dta ;

			if(((ic%100000)==0) && ((time(0)-now)>5)) {

				// slice counter
				int tslice = dam->reg_r(0x400+16+1) ;


				u_int r808 = dam->reg_r(0x800+8) ;
				
				u_int err = dam->reg_r(0x400+3+16) ;

				dta = dam->reg_r(0x800+8+1) ;


				LOG(TERR,"words %d, tslice %d, reg808 0x%08X: dta 0x%08X, ASIC err 0x%08X",
				    cou,tslice,r808,dta,err) ;

				LOG(TERR,"ASIC status 0x%08X, ASIC locked 0x%08X",
				    dam->reg_r(0x400+4+16),
				    dam->reg_r(0x400+2+16)) ;
			
				now = time(0) ;

				fflush(stdout) ;
			}


			// check FIFO
			if(!(dam->reg_r(0x800+8)&(1<<4))) {	// FIFO not empty
				//u_int up = dam->reg_r(0x800+8)&0x3 ;	// read upper 2 bits
				dta = dam->reg_r(0x800+8+1) ;		// read 32 bit data

				dam->reg_w(0x800,r800|(1<<17)) ;	// bit 17 up
				dam->reg_w(0x800,r800) ;		// bit 17 downr

				//dam->reg_wp(0x800,17) ;		// pulse rd_en


				printf("Evt %d: %d = 0x%08X\n",evt,cou,dta) ;

				cou++ ;

				switch(dta) {
				case 0xABCDFFFF :
					LOG(NOTE,"Evt %d: start of tslice 1st header %d: 0x%08X",evt,cou,dta) ;
					break ;
				case 0x88CD0000 :
					LOG(NOTE,"Evt %d: ASIC packet start %d: 0x%08X",evt,cou,dta) ;
					break ;
				case 0x44760000 :
					LOG(NOTE,"Evt %d: ASIC packet end %d: 0x%08X",evt,cou,dta) ;
					evt++ ;

					if((evt%100)==1) {
						LOG(INFO,"Fired %d/%d events",evt,num_events) ;
					}

					if(evt>=num_events) {
						goto end_run ;
					}
					else {
						LOG(DBG,"Fire evt %d",evt) ;
						dam->reg_wp(0x400+2,31) ;	// GO!
					}
					break ;					
				case 0xFEED0001 :
					LOG(NOTE,"Evt %d: end of tslice 2nd header %d: 0x%08X",evt,cou,dta) ;
					cou = 0 ;
					//goto end_tslice ;
					break ;
				}
			}
			else empty_cou++ ;


			if(sig_on) break ;
		}

		end_run:;

		
		if(empty_cou) LOG(TERR,"Empty FIFO %d, secs %u",empty_cou,time(0)-now1) ;

		dam->reg_w(0,0) ;	// stop run
		dam->reg_w(0x400,0) ;	// stop GTU

		LOG(INFO,"Run ended after %d events in %d seconds.",evt,time(0)-now1) ;
	}



	LOG(INFO,"Done. Exiting.") ;

	delete dam ;
	return 0;
}


static int open_py(const char *fname)
{
	FILE *f = fopen(fname,"r") ;

	reg_cou = 0 ;	// zap it first

	if(f==0) {
		LOG(ERR,"open_py(%s): %s",fname,strerror(errno)) ;
		return -1 ;
	}

	while(!feof(f)) {
		char buff[256] ;
		char b[256] ;
		u_int reg ;

		if(fgets(buff,sizeof(buff),f)==0) continue ;


		int ret = sscanf(buff,"eic_clib.write_asic_indirect_reg(0x%X, %s",&reg,b) ;
		if(ret!=2) continue ;

		u_int val = 0 ;
		int bit = 7 ;
		if(b[0]=='0' && b[1]=='b') {
			for(int j=2;j<20;j++) {
				//printf(".... j=%d, bit=%d, bj=%c\n",j,bit,b[j]) ;

				if(b[j]=='1') {
					val |= 1<<bit ;
					bit-- ;
				}
				else if(b[j]=='0') bit-- ;
			}
		}
		else continue ;

		//printf("Reg %d: 0x%04X = [%s] -- 0x%02X\n",reg_cou,reg,b,val) ;

		regs[reg_cou].reg = reg ;
		regs[reg_cou].val = val ;
		reg_cou++ ;
	}

	fclose(f) ;

//	printf("Read %d items\n",reg_cou) ;
//	for(int i=0;i<reg_cou;i++) {
//		printf("I2C %d: reg 0x%04X = 0x%02X\n",i,regs[i].reg,regs[i].val) ;
//	}

	return reg_cou ;

}




#if 0
static void adc_print()
{
	extern int lpgbt_adc_c(char *dest, int ch, int val) ;
	extern int mux64_to_gpio(int ch) ;
	extern int mux64_adc_c(char *dest, int ch, int val) ;

	char ctmp[64] ;
	u_short adc ;
	u_char gpio ;

	dam->lpgbt_adc(-1) ;

	for(int i=0;i<16;i++) {
		// read
		adc = dam->lpgbt.mon.adc[i] ;

		lpgbt_adc_c(ctmp,i,adc) ;
		printf("ADC %02d: %s\n",i,ctmp) ;
	}

	for(int i=0;i<64;i++) {
		int ret = mux64_adc_c(ctmp,i,0) ;	// check for existence
		if(ret<0) continue ;

		gpio = mux64_to_gpio(i) ;

//		LOG(TERR,"MUX ch %d : gpio 0x%02X",i,gpio) ;

//		printf("**** mux ch %d, gpio 0x%02X\n",i,gpio) ;

		// set gpio
		for(int j=0;j<6;j++) {
			if(gpio & (1<<j)) {
				dam->lpgbt_gpio(j,1) ;
				//printf("........ gpio %d to 1\n",j) ;
			}
			else dam->lpgbt_gpio(j,0) ;
		}

		int should = dam->lpgbt_gpio(-1,0) & 0x3F ;

		if(gpio != should) {
			LOG(ERR,"oops") ;
		}

//		printf("... now 0x%02X\n",dam->lpgbt_gpio(-1,0)) ;

		// wait a bit
		usleep(50000) ;

		// read ADC1
		adc = dam->lpgbt_adc(1) ;

//		adc = dam->lpgbt.mon.adc[1] ;

//		printf(".... %d %d\n",adc,dam->lpgbt.mon.adc[0]) ;

		mux64_adc_c(ctmp,i,adc) ;	// to get the char string and conversion factors
		printf("MUX64 %02d: %s\n",i,ctmp) ;
	}

	return ;
}
#endif
