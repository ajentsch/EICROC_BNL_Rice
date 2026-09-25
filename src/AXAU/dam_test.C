#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>


#include <LOG/rtsLog.h>
volatile int rtsLogLevel = 1 ;

#include "dam_c.h"

static int sig_on ;
static dam_c *dam ;

void sighandler(int sig)
{
	if(sig==2) {
		LOG(WARN,"Cntrl-C: EXITING.") ;
	}
	else {
		LOG(ERR,"Signal %d: EXITING.",sig) ;
	}

	sig_on = sig ;
}

static void adc_print() ;


int main(int argc, char *argv[])
{
//	dam_c *dam ;
	int c ;
	u_short reg ;
	u_char val ;

	int reg_dump = 0 ;
	int do_reset = 0 ;
	int lpgbt_play = 0 ;
	int i2c_mode = 0 ;
	int adc_mode = 0 ;
	int gpio_mode = 0 ;
	int clock_mode = 0 ;
	int etroc_mode = 0 ;
	int run_mode = 0 ;
	const char *usb_dev = 0 ;

	signal(2,sighandler) ;

	while((c=getopt(argc,argv,"rRl:2AGCE:s:D:d:")) != EOF) {
	switch(c) {
	case 'r' :
		reg_dump = 1 ;
		break ;
	case 'R' :
		do_reset = 1 ;
		break ;
	case 'l' :
		lpgbt_play = atoi(optarg) ;
		break ;
	case '2' :
		i2c_mode = 1 ;
		break ;
	case 'A' :
		adc_mode = 1 ;
		break ;
	case 'G' :
		gpio_mode = 1 ;
		break ;
	case 'C' :
		clock_mode = 1 ;
		break ;
	case 'E' :
		etroc_mode = atoi(optarg) ;
		break ;
	case 's' :
		run_mode = atoi(optarg) ;
		break ;
	case 'D' :
		rtsLogLevel = atoi(optarg) ;
		break ;
	case 'd' :
		usb_dev = optarg ;
		break ;
	}
	}
	
	dam = new dam_c(usb_dev) ;	// init PCIe version if NULL


	dam->lpgbt_board_type = 2 ;	// 2: FTOF RBv1
	dam->init() ;

	if(reg_dump==1) {
		u_short reg ;

		for(int i=0;i<32;i++) {
			reg = i ;
			u_int v = dam->reg_r(reg) ;

			printf("slv100 %2d[0x%03X] = 0x%08X[%u]\n",i,reg,v,v) ;
		}

		for(int i=0;i<32;i++) {
			reg = 0x400+i ;
			u_int v = dam->reg_r(reg) ;

			printf("slv160 %2d[0x%03X] = 0x%08X[%u]\n",i,reg,v,v) ;
		}

		for(int i=0;i<16;i++) {
			reg = 0x800+i ;
			u_int v = dam->reg_r(reg) ;

			printf("slv250 %2d[0x%03X] = 0x%08X[%u]\n",i,reg,v,v) ;
		}
	}



	dam->status() ;

	if(do_reset) {
		LOG(WARN,"Issuing RESET") ;
		if(dam->lpgbt_reset()==0) {
			LOG(ERR,"Exiting!") ;
			return -1 ;
		}
	}

	LOG(INFO,"Running lpgbt_init") ;
	dam->lpgbt_init(0) ;


#if 0
	if(1) {	// fuse chipid
		u_int chipid = 0 ;
		dam->lpgbt_write(0x119,(1<<1)) ;



		for(int i=0;i<100;i++) {
			u_int v = dam->lpgbt_read(0x1B1) ;
			if(v & (1<<2)) {
				//printf("... valid at %d 0x%X\n",i,v) ;
				break ;
			}
		}


		dam->lpgbt_write(0x11F,0) ;

		for(int r=0;r<4;r++) {
			u_int v = dam->lpgbt_read(0x1B2+r) ;

			//printf("... 0x%03X = 0x%02X\n",0x1BB+r,v) ;

			chipid |= (v)<<(r*8) ;
		}

		dam->lpgbt_write(0x119,0) ;

		printf("lpGBT chip-id 0x%08X\n",chipid) ;
	}
#endif

	if(reg_dump) {
		u_short reg_max = 0x1EC ;

		u_char reg_etl[512] ;
		u_char reg_me[512] ;
		FILE *f = fopen("lpgbt_daq_dump_C42AFE4A_v1_tonko.txt","r") ;
		while(!feof(f)) {
		u_int a, v ;

		char buff[128] ;
		if(fgets(buff,sizeof(buff),f)==0) continue ;

		int ret = sscanf(buff,"%x %x",&a,&v) ;
		if(ret != 2 ) continue ;

		reg_etl[a] = v ;
		}
		fclose(f) ;

		for(int r=0;r<=reg_max;r++) {
			reg_me[r] = dam->lpgbt_read(r) ;

			if(reg_me[r] != reg_etl[r]) {
				//printf("lpGBT DIFF: reg 0x%03x: 0x%02X me, 0x%02X ETL\n",r,reg_me[r],reg_etl[r]) ;
				printf("%s",ANSI_RED) ;
			}

			printf("lpGBT reg 0x%03x: 0x%02X me, 0x%02X ETL\n",r,reg_me[r],reg_etl[r]) ;
			printf("%s",ANSI_RESET) ;
		}
		


	}

	if(etroc_mode) {
		u_short val ;
		u_short tmp ;
		u_char pattern = 0x5C ;

		for(int i=0;i<32;i++) {
			val = dam->lpgbt_etroc_rd(0x64,i) ;
			LOG(INFO,"ETROC reg 0x%04X[%d] = 0x%02X",i,i,val) ;
		}

		for(int i=0x100;i<0x120;i++) {
			val = dam->lpgbt_etroc_rd(0x64,i) ;
			LOG(INFO,"ETROC reg 0x%04X[%d] = 0x%02X",i,i,val) ;
		}
#if 1

		dam->lpgbt_etroc_wr(0x64,29,0xFF) ;
		dam->lpgbt_etroc_wr(0x64,28,0xFF) ;
		dam->lpgbt_etroc_wr(0x64,27,0xFF) ;
		dam->lpgbt_etroc_wr(0x64,26,pattern) ;	// this is the only one that seems to work!?

#endif

		LOG(INFO,"ETROC: bit error %d",dam->lpgbt_etroc_rd(0x64,0x102)&1) ;
		LOG(INFO,"ETROC: AFC busy %d",dam->lpgbt_etroc_rd(0x64,0x100)&1) ;
		LOG(INFO,"ETROC: FC align state %d [9?]",dam->lpgbt_etroc_rd(0x64,0x101)>>4) ;
		LOG(INFO,"ETROC: GLO FSM state %d [11?]",dam->lpgbt_etroc_rd(0x64,0x101)&0xF) ;
		LOG(INFO,"ETROC: FC self-align error 0x%X",dam->lpgbt_etroc_rd(0x64,0x102)>>2) ;

		u_int fc_invalid_cou = ((dam->lpgbt_etroc_rd(0x64,0x104)&0xF)<<8)|dam->lpgbt_etroc_rd(0x64,103) ;
		u_int pll_unlock_cou = ((dam->lpgbt_etroc_rd(0x64,0x105))<<4)|(dam->lpgbt_etroc_rd(0x64,0x104))>>4 ;

		LOG(INFO,"ETROC: FC invalid %d, PLL unlock %d",fc_invalid_cou,pll_unlock_cou) ;

		// program chip-id
		dam->lpgbt_etroc_wr(0x64,22,0x49) ;

		//dam->lpgbt_etroc_wr(0x64,0x11,0xF0) ;	// drive IDLE? I'm not sure this is correct!
		dam->lpgbt_etroc_wr(0x64,0x11,0x98) ;	// default is 0x98

		// disable Clock and FCMD termination. Because the ASIC Module Board terminates them!
		dam->lpgbt_etroc_wr(0x64,7,0xA1) ;
		dam->lpgbt_etroc_wr(0x64,9,0xE1) ;

		u_char reg19 = 0x43 ;	// 320 Mbs

		reg19 = (0<<4)|(0<<2)|(1<<1)|(1<<0) ;
		LOG(TERR,"reg19 is 0x%02X",reg19) ;
		dam->lpgbt_etroc_wr(0x64,19,reg19) ;

#if 1
		// reset PLL
		tmp = dam->lpgbt_etroc_rd(0x64,13) ;
		val = tmp & (~0x80) ;
		dam->lpgbt_etroc_wr(0x64,13,val) ;
		dam->lpgbt_etroc_wr(0x64,13,tmp) ;

		// reset fast command
		tmp = dam->lpgbt_etroc_rd(0x64,14) ;
		val = tmp & (~0x40) ;
		dam->lpgbt_etroc_wr(0x64,14,val) ;
		dam->lpgbt_etroc_wr(0x64,14,tmp) ;

		// reset Global Readout
		tmp = dam->lpgbt_etroc_rd(0x64,14) ;
		val = tmp & (~0x80) ;
		dam->lpgbt_etroc_wr(0x64,14,val) ;
		dam->lpgbt_etroc_wr(0x64,14,tmp) ;
#endif


		
		val = dam->lpgbt_etroc_rd(0x64,13) ;
#if 0
		val |= (1<<6) ;	// set pattern
		LOG(WARN,"Pattern Data 0x%02X",pattern) ;
#else
		val &= ~(1<<6) ;	// set real data
		LOG(TERR,"Real data") ;
#endif
		dam->lpgbt_etroc_wr(0x64,13,val) ;
		LOG(TERR,"reg13 was 0x%02X",val) ;


		// reset asic_arbiter_reset (and asic locked)
		dam->reg_wp(0x000,9) ;

		dam->reg_w(0x400,0xFF000000) ;
		u_int val32 = dam->reg_r(0x400+16+2) ;
		LOG(TERR,"ASIC Locked 0x%08X",val32) ;


		for(int ii=0;ii<4;ii++) {
			u_int st = dam->reg_r(0x400+16) ;
			int bitslip ;
			int rr ;

			bitslip = (st>>8)&0xF ;
			rr = st&0xFF ;

			dam->reg_w(0x400,0xFF000000+1) ;
			val32 = dam->reg_r(0x400+16+2) ;

			dam->reg_w(0x400,0xFF000000) ;
			u_int vall = dam->reg_r(0x400+16+2) ;

			LOG(TERR,"ASIC val 0x%08X, locked 0x%08X, bitslip %d, rr %d",val32,vall,bitslip,rr) ;
			dam->fast_cmd(0x96) ;

			usleep(1000) ;
			st = dam->reg_r(0x400+16) & 0xFF ;
			usleep(1000) ;
			rr = dam->reg_r(0x400+16) & 0xFF ;

			LOG(TERR,"   rr = %d %d",st,rr) ;

			usleep(1000000) ;
			if(sig_on) return -1 ;

//			for(int i=0x152;i<=0x16E;i++) {
//				LOG(TERR,"   reg 0x%03X = 0x%02X",i,dam->lpgbt_read(i)) ;
//			}
			
//			break ;

		}

	}

	if(clock_mode) {
		u_int hi ;
		u_int lo ;
		int c = 0 ;

		for(int r=0x6E;r<=0xA6;r+=2) {
			hi = dam->lpgbt_read(r) ;
			lo = dam->lpgbt_read(r+1) ;

			hi = (hi<<8)|lo ;

			printf("lpGBT: clock out %2d = 0x%04X\n",c,hi) ;

			c++ ;
			
		}

		for(int r=0xA8;r<=0xAD;r++) {
			printf("lpGBT: ePort TX 0x%03X = 0x%02X\n",r,dam->lpgbt_read(r)) ;
		}

		for(int r=0xC8;r<=0xF6;r++) {
			printf("lpGBT: ePort RX 0x%03X = 0x%02X\n",r,dam->lpgbt_read(r)) ;
		}

	}


	if(gpio_mode) {
		dam->lpgbt_gpio(1,1) ;
	}

	if(adc_mode) {
		u_short adc[64] ;

		memset(adc,0,sizeof(adc)) ;

		dam->lpgbt_adc(-1) ;
		for(int ch=0;ch<16;ch++) {
			LOG(INFO,"lpGBT: ADC %d = %u",ch,dam->lpgbt.mon.adc[ch]) ;
			printf("ADC %2d = %u\n",ch,dam->lpgbt.mon.adc[ch]) ;
		}	
	
#if 1	
		LOG(INFO,"Now reading MUX64...") ;

		for(int i=0;i<64;i++) {
			char bit[8] ;

			
			for(int j=0;j<6;j++) {
				bit[j] = (i & (1<<j))?1:0 ;
			}
//			printf("Val 0x%02X = ",i) ;
			for(int j=5;j>=0;j--) {
//				printf("%d ",bit[j]) ;

				dam->lpgbt_gpio(j,bit[j]) ;
			}
//			printf("\n") ;

			u_short mux64 = (bit[0]<<5)|(bit[1]<<4)|(bit[2]<<6)|(bit[3]<<3)|(bit[4]<<1)|(bit[5]<<2) ;
			mux64 >>= 1 ;



			usleep(50000) ;
			adc[mux64] = dam->lpgbt_adc(1) ;

//			adc[mux64] = dam->lpgbt.mon.adc[1] ;
			//printf("GPIO %d, MUX %d = %d\n",i,mux64,dam->lpgbt.mon.adc[1]) ;

		}


		for(int i=0;i<=13;i++) {
			LOG(INFO,"MUX64 %2d = %4d",i,adc[i]) ;
			printf("MUX64 %2d = %4d\n",i,adc[i]) ;
		}
#endif

	}


	if(i2c_mode) {
		//dam->lpgbt_i2c_scan(0) ; // not connected on ETL RB2.2
		dam->lpgbt_i2c_scan(1) ;	// ASICs
		dam->lpgbt_i2c_scan(2) ;	// 0x50 VTRX+, 0x70 2nd lpGBT

		// read VTRX+ reg0 which enables TX
		u_short val = dam->lpgbt_i2c_read(2,0x50,0) ;
		LOG(NOTE,"VTRX: I2C transmitters: 0x%02X",val) ;

		// read VTRX ID whould read 0x15
		val = dam->lpgbt_i2c_read(2,0x50,0x15) ;
		if(val != 0x15) {
			LOG(ERR,"VTRX: I2C ID (should be 0x15): 0x%02X",val) ;
		}
		//LOG(TERR,"VTRX: I2C ID (should be 0x15): 0x%02X",val) ;


		// turn ON the 2nd VTRX fiber by writing a 0x3
		val = dam->lpgbt_i2c_write(2,0x50,0,0x3) ;
		if(val != 0) {
			LOG(ERR,"VTRX: I2C set 2 transmitters (ret 0x%X)",val) ;
		}
		//LOG(TERR,"VTRX: I2C set 2 transmitters (ret 0x%X)",val) ;

		// and verify that it's 0x3
		val = dam->lpgbt_i2c_read(2,0x50,0) ;
		if(val != 0x3) {
			LOG(ERR,"VTRX: I2C transmitters now: 0x%02X",val) ;
		}
		else {
			LOG(INFO,"VTRX: I2C transmitters now: 0x%02X",val) ;
		}
	}

	switch(lpgbt_play) {
	case 1 :
		reg = 0x1D7 ;
		val = dam->lpgbt_read(reg) ;
		LOG(TERR,"lpGBT reg 0x%03X = 0x%02X",reg,val) ;
		dam->status() ;

		reg = 0xFB ;
		val = dam->lpgbt_read(reg) ;
		LOG(TERR,"lpGBT reg 0x%03X = 0x%02X",reg,val) ;
		dam->status() ;


		break ;
	case 2 :
		{
		//FILE *f = fopen("lpgbt_default_regs.txt","w") ;
		for(reg=0;reg<=0x1ED;reg++) {
			val = dam->lpgbt_read(reg) ;
			LOG(TERR,"lpGBT reg 0x%03X = 0x%02X",reg,val) ;
			//fprintf(f,"0x%03X 0x%02X\n",reg,val) ;
		}
		//fclose(f) ;
		}
		break ;
	case 3 :	// dump those really important regs
		for(int r=0x20;r<= 0x31;r++) {
			printf("lpGBT 0x%03X = 0x%02X\n",r,dam->lpgbt_read(r)) ;
		}
		break ;
	case 100 :
		// Try to reset this piece of garbage lpGBT according to a ETL python script, arg
		for(int i=0;i<10;i++) {
			dam->reg_wp(0x0,4) ;	// reset GTH
			usleep(100000) ;

			
			LOG(TERR,"Initializing lpGBT 0x%02X",dam->lpgbt_dev_id) ;

			dam->lpgbt_write(0x128,0xC0) ;
			usleep(50000) ;
			dam->status() ;

			dam->lpgbt_write(0x128,0x0) ;
			usleep(50000) ;
			dam->status() ;

			dam->lpgbt_write(0x36,0x80) ;	// MUST be 0x80!!!!
			usleep(50000) ;
			dam->status() ;

			dam->lpgbt_write(0xFB,0x6) ;
			usleep(50000) ;
			dam->status() ;

			val = dam->lpgbt_read(0x1D7) ;
			usleep(10000) ;
			dam->status() ;
			if(val==0xA6) {
				LOG(INFO,"lpGBT reset-to-READY successfull at attempt %d.",i+1) ;
				break ;
			}
			
		}
		break ;
	}

	if(run_mode) {
		LOG(INFO,"Starting Run.") ;

		// start a CPU run
		dam->reg_w(0x00,0) ;	// slave 100 clear

		dam->reg_w(0x400+1,0x06050401) ;	// GEO Id

		// cautiously reset: pci iface, pci_fifo_srst, asic_arbiter
		// NOT gt_iface, NOT reg_iface

		// rc_i is slv100_0(0)(7..0)) ;

		dam->reg_w(0,(1<<5)|(1<<8)|(1<<9)) ;	// reset All
		usleep(1000) ;						// keep in reset a bit
		dam->reg_w(0,0) ;					// un-reset


		// reset MEM_P
		dam->reg_wp(0x800,1) ;	// reset MEM_P FIFO
		dam->reg_w(0x801,0x80000000) ;	// MEM_P address
		dam->reg_wp(0x800,0) ;	// pulse MEM_P WR_EN

		// prepare for CPU read
		// RC_I(5) is reset
		// oreg(0)(1..0) bits 33,32
		// oreg(0)(4)	-- FIFO empty
		// oreg(0)(23..16) M2S STATUS
		// oreg1 -- fifo_dout

		// ireg(0)(16) -- PCIE fifo owner:1 is CPU
		// ireg(0(17) -- fifo_rd_en
		//
		dam->reg_w(0x800,(1<<16)) ;	// CPU readout
		LOG(TERR,"0x800 0x%08X 0x%08X 0x%08X",dam->reg_r(0x800),dam->reg_r(0x800+8),dam->reg_r(0x800+8+2)) ;

		int cou = 0 ;
		volatile u_int *store = dam->resmem_glo ;
		int max_words = (128*1024*1024)/4 ;

		// start run
		dam->reg_w(0,(1<<0)) ;

		// start GTU
		dam->reg_w(0x400,(1<<20)) ;

//		LOG(TERR,"0x800 0x%08X 0x%08X 0x%08X",dam->reg_r(0x800),dam->reg_r(0x800+8),dam->reg_r(0x800+8+2)) ;
//		dam->fast_cmd(0x96) ;	// L1A

		// cpu read
	
		for(int i=0;i<2000000000;i++) {
			u_int dta ;



			if(i%3==0) {
				//dam->fast_cmd(0x66) ;	// Charge injection 0x69
				//usleep(10) ;
				dam->fast_cmd(0x96) ;	// L1A
				//u_int val = dam->reg_r(0x400+16) & 0xFF ;
				//LOG(TERR,"round trip to 0x96 = %d",val) ;
			}

			if(((i+1)%200000)==0) {
				//dam->fast_cmd(0x96) ;	// L1A

				// slice counter
				int tslice = dam->reg_r(0x400+17) ;


				u_int r808 = dam->reg_r(0x800+8) ;
				
				dta = dam->reg_r(0x800+8+1) ;

				LOG(TERR,"tslice %d, erg808 0x%08X: dta 0x%08X",tslice,r808,dta) ;

			}
#if 0
			time_t curr = time(0) ;
			if(curr>now) {
				// slice counter
				int tslice = dam->reg_r(0x400+17) ;


				u_int r808 = dam->reg_r(0x800+8) ;
				
				dta = dam->reg_r(0x800+8+1) ;

				LOG(TERR,"tslice %d, erg808 0x%08X: dta 0x%08X",tslice,r808,dta) ;
				now = curr+1 ;
			}
#endif

			if(cou>=max_words) break ;

			// check FIFO
			if(!(dam->reg_r(0x800+8)&(1<<4))) {	// FIFO not empty
				//u_int up = dam->reg_r(0x800+8)&0x3 ;	// read upper 2 bits
				dta = dam->reg_r(0x800+8+1) ;		// read 32 bit data
				//LOG(TERR,"Data %d = 0x%d_%08X",cou,up,dta) ;
				dam->reg_wp(0x800,17) ;		// pulse rd_en


				store[cou] = dta ;
				//printf("Data %d =  %d %08X\n",cou,up,dta) ;

				cou++ ;
			}
			if(sig_on) break ;
		}

		dam->reg_w(0,0) ;	// stop run
		dam->reg_w(0x400,0) ;	// stop GTU

		LOG(TERR,"Run ended after %d words",cou) ;

		for(int i=0;i<cou;i++) {
			printf("%d = 0x%08X\n",i,store[i]) ;
		}
	}

	
	LOG(INFO,"Entering status-check loop.") ;

	int rr_expect = 115 ;
	for(int i=0;i<10;i++) {
		int round_trip = dam->lpgbt_round_trip() ;
		if(i==0) LOG(INFO,"lpGBT round trip is %d, expected %d",round_trip,rr_expect) ;
		if(rr_expect != round_trip) {
			LOG(WARN,"lpGBT round trip is %d, expect %d",round_trip,rr_expect) ;
		}
		usleep(100000) ;
	}

//	dam->fast_cmd(0xFF) ;	// tested, kinda

	time_t now = time(0) ;
	time_t now_adc = now ;

//	int flip = 0 ;
//	int lo_flip = 0 ;


//	adc_print() ;

	int dta_ix = 0 ;

	for(int i=0;i<10000000;i++) {
		usleep(200000) ;
		if(sig_on) return -1 ;	// Ctrl-C pressed

		dam->status() ;

		dam->lpgbt_monitor(100) ;

// This blinks ETL board LEDs, just skitp it
#if 0
		if(flip) {				
//			dam->lpgbt_gpio(1,1) ;
//			dam->lpgbt_gpio(2,0) ;
			dam->lpgbt_gpio(15,0) ;
			flip = 0 ;
		}
		else {
//			dam->lpgbt_gpio(1,0) ;
//			dam->lpgbt_gpio(2,1) ;
			dam->lpgbt_gpio(15,1) ;
			flip = 1 ;
		}
#endif

		if(time(0)>now) {
			u_short gpio ;

#if 0	
			// RB2.2
			if(lo_flip) {
				dam->lpgbt_gpio(3,1) ;
				lo_flip = 0 ;
			}
			else {
				dam->lpgbt_gpio(3,0) ;
				lo_flip = 1 ;
			}
#endif

			gpio = dam->lpgbt_gpio(-1,0) ;

			dam->reg_w(0x400,0xFF000000+dta_ix) ;
			u_int data[2] ;
			data[0] = dam->reg_r(0x400+16+1) ;
			data[1] = dam->reg_r(0x400+16+2) ;



			u_char a6 = dam->lpgbt_read(0x1D7) ;
			if(a6 != 0xA6) {
				LOG(ERR,"lpGBT bad") ;
			}
			else {
				LOG(INFO,"Still checking: GPIO 0x%04X, data_ix %d 0x%08X 0x%08X -- all OK...",gpio,
				    dta_ix,data[0],data[1]) ;
			}

			if(time(0)>now_adc) {
				adc_print() ;
				now_adc = time(0) + 30 ;
			}

			now = time(0) + 5 ;

			dta_ix++ ;
			if(dta_ix>=6) dta_ix = 0 ;

		}
	}


	delete dam ;
	return 0;
}


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
