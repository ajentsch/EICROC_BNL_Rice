#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <termios.h>
#include <sys/file.h>
#include <ctype.h>
#include <stdlib.h>

#include <LOG/rtsLog.h>
#include <RESMEM/resmem.h>

#include "dam_c.h" 


#define DAM_PCI_MEM	"/dev/dam1/resource0"


int dam_c::lpgbt_vtrx()
{
	// bus #2, I2C device 0x50
	u_int val ;
	u_int ver ;
	u_int tx_ena ;

	ver = lpgbt_i2c_read(2,0x50,0x15) ;	// ID: should be 0x15
	if(ver != 0x15) {
		LOG(ERR,"VTRX: version reg 0x15 = 0x%02X",ver) ;
	}

	tx_ena = lpgbt_i2c_read(2,0x50,0) ;	// transmitters: should be 1

	u_int uid = 0 ;
	for(int r=0;r<4;r++) {
		val = lpgbt_i2c_read(2,0x50,0x16+r) ;
		uid |= (val<<(r*8)) ;
	}

	u_int seu = 0 ;
	for(int r=0;r<4;r++) {
		val = lpgbt_i2c_read(2,0x50,0x1A+r) ;
		seu |= (val<<(r*8)) ;
	}

	
	LOG(INFO,"VTRX: V 0x%02X, UID 0x%08X, TX enables 0x%X, SEUs %u",ver,uid,tx_ena,seu) ;
	
	return 0 ;
}


u_int dam_c::lpgbt_user_id(u_int uid) 
{
	if(uid==0xFFFFFFFF) {

	}
	else {
		lpgbt_write(0x004,(uid>>0)&0xFF) ;
		lpgbt_write(0x005,(uid>>8)&0xFF) ;
		lpgbt_write(0x006,(uid>>16)&0xFF) ;
		lpgbt_write(0x007,(uid>>24)&0xFF) ;
	}

	uid = 0 ;

	uid |= (lpgbt_read(0x004)<<0) ;
	uid |= (lpgbt_read(0x005)<<8) ;
	uid |= (lpgbt_read(0x006)<<16) ;
	uid |= (lpgbt_read(0x007)<<24) ;

	return uid ;
}

u_int dam_c::lpgbt_chip_id()
{
	u_int chipid = 0 ;

	lpgbt_write(0x119,(1<<1)) ;



	for(int i=0;i<100;i++) {
		u_int v = lpgbt_read(0x1B1) ;
		if(v & (1<<2)) {
			//printf("... valid at %d 0x%X\n",i,v) ;
			break ;
		}
	}


	lpgbt_write(0x11F,0) ;

	for(int r=0;r<4;r++) {
		u_int v = lpgbt_read(0x1B2+r) ;

		//printf("... 0x%03X = 0x%02X\n",0x1BB+r,v) ;

		chipid |= (v)<<(r*8) ;
	}

	lpgbt_write(0x119,0) ;

//	printf("lpGBT chip-id 0x%08X\n",chipid) ;
	
	return chipid ;
}

int dam_c::lpgbt_round_trip()
{
//	int before = reg_r(0x400+16) & 0xFF ;	// these are accidentals; if 0xFF is received over the link

	// switch lpGBT to loopback

	int mode = 6 ;	//4:fixed pattern; 6:fcmd loopback
//	LOG(WARN,"lpGBT: Using test pattern %d",mode) ;
	lpgbt_write(0x129,(mode<<3)|(mode<<0)) ;	// uplink groups 1 i 0
	lpgbt_write(0x12A,(mode<<3)|(mode<<0)) ;	// uplink groups 3 i 2
	lpgbt_write(0x12B,(mode<<3)|(mode<<0)) ;	// uplink groups 5 i 4
	lpgbt_write(0x12C,(mode<<0)) ;			// uplink groups 6

	usleep(1000) ;

#if 0
	for(int i=0;i<100;i++) {
		int before = reg_r(0x400+16) & 0xFF ;
		LOG(TERR,"0x%X",before) ;
		usleep(100) ;
	}
#endif

	fast_cmd(0xFF) ;	// this initiates the timer

	usleep(100) ;

	int after = reg_r(0x400+16) & 0xFF ;

	// return lpGBT from loopback
	lpgbt_write(0x129,0) ;	
	lpgbt_write(0x12A,0) ;
	lpgbt_write(0x12B,0) ;
	lpgbt_write(0x12C,0) ;

//	LOG(INFO,"Round-trip: before %d, after %d",before,after) ;

	return after ;	// I measure 102 -- these are 315.2 MHz ticks
}


/************* ETROC I2C Write ******************************/
u_short dam_c::lpgbt_etroc_wr(u_char dev, u_short reg, u_char val)
{
	u_short ret = 0 ;

	int data0 = 0x109 ;
	int data1 = 0x10A ;
	int data2 = 0x10B ;
	int cmd = 0x10d ;
	int addr = 0x108 ;
	int status = 0x186 ;
//	int read0 = 0x188 ;

	lpgbt_write(data0,(3<<2)|(1<<0)) ;	// 2 words written
	lpgbt_write(cmd,0) ;

	lpgbt_write(data0,reg&0xFF) ;
	lpgbt_write(data1,reg>>8) ;
	lpgbt_write(data2,val) ;
	lpgbt_write(cmd,0x8) ;

	lpgbt_write(addr,dev) ;
	lpgbt_write(cmd,0xC) ;

	int devs = 0 ;
	for(int i=0;i<10;i++) {
			
		u_int val = lpgbt_read(status) ;
		if(val==0) continue ;

		if(val != 0x40) {
			if(val & 0x04) {
				//LOG(INFO,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
				devs++ ;
				break ;
			}
			else {
				//LOG(WARN,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
			}
		}
	}
	if(devs==0) {
		LOG(ERR,"ETROC write") ;
		return 0xFFFF ;
	}


//	LOG(INFO,"ETROC reg 0x%04X: wrote 0x%02X",reg,val) ;

	return ret ;
}


/************* ETROC I2C Read ******************************/
u_short dam_c::lpgbt_etroc_rd(u_char dev, u_short reg)
{
	u_short ret = 0 ;

	int data0 = 0x109 ;
	int data1 = 0x10A ;
	int cmd = 0x10d ;
	int addr = 0x108 ;
	int status = 0x186 ;
	int read0 = 0x188 ;

	lpgbt_write(data0,(2<<2)|(1<<0)) ;	// 2 words written
	lpgbt_write(cmd,0) ;

	lpgbt_write(data0,reg&0xFF) ;
	lpgbt_write(data1,reg>>8) ;
	lpgbt_write(cmd,0x8) ;

	lpgbt_write(addr,dev) ;
	lpgbt_write(cmd,0xC) ;

	int devs = 0 ;
	for(int i=0;i<10;i++) {
			
		u_int val = lpgbt_read(status) ;
		if(val==0) continue ;

		if(val != 0x40) {
			if(val & 0x04) {
				//LOG(INFO,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
				devs++ ;
				break ;
			}
			else {
				//LOG(WARN,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
			}
		}
	}
	if(devs==0) {
		LOG(ERR,"ETROC rd") ;
		return 0xFFFF ;
	}

	lpgbt_write(data0,(0<<2)|(1<<0)) ;
	lpgbt_write(cmd,0) ;


	lpgbt_write(addr,dev) ;
	lpgbt_write(cmd,0x3) ;

	devs = 0 ;
	for(int i=0;i<10;i++) {
			
		u_int val = lpgbt_read(status) ;
		if(val==0) continue ;

		if(val != 0x40) {
			if(val & 0x04) {
				//LOG(INFO,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
				devs++ ;
				break ;
			}
			else {
				//LOG(WARN,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
			}
		}
	}
	if(devs==0) {
		LOG(ERR,"ETROC rd") ;
		return 0xFFFF ;
	}

	ret = lpgbt_read(read0) ;

//	LOG(INFO,"ETROC reg 0x%04X = 0x%02X",reg,ret) ;

	return ret ;
}

/************** Issue a single Fast Command by hand ******************/
void dam_c::fast_cmd(u_char val)
{
	u_int cache = reg_r(0x400+0) ;

	LOG(DBG,"fast_cmd 0x%02X (cached 0x%08X)",val,cache) ;

	cache &= 0x00FFFFFF ;

	cache |= (val<<24) ;

	reg_w(0x400+0,cache) ;

	// data(23..20) is gtu_control
	reg_wp(0x400+0,20+3) ;

	return ;
}


/************************************************

	Called at DAQ code startup.

	Level 0 is -- start of code
	Level 100 is -- from complete power loss

************************************************/

int dam_c::lpgbt_init(int level)
{
	int ret = 0 ;
	int mode ;
	u_int user_id ;
	const char *cver ;

	LOG(INFO,"lpGBT: init(%d)",level) ;

	// first: make sure I can read the version
	u_int ver  = lpgbt_read(0x1D7) ;
	if(ver != 0xA6) {
		LOG(ERR,"lpGBT: wrong version 0x%02X (not 0xA6)",ver) ;
	}

	switch(ver) {
	case 0xA5 :
		cver = "V0" ;
		break ;
	case 0xA6 :
		cver = "V1" ;
		break ;
	default :
		cver = "UNKNOWN" ;
		break ;
	}

	LOG(INFO,"lpGBT: %s[0x%02X], CHIP_ID 0x%08X, USER_ID 0x%08X",cver,ver,lpgbt_chip_id(),lpgbt_user_id(0xFFFFFFFF)) ;
	user_id = lpgbt_user_id(0xFFFFFFFF) ;

	LOG(INFO,"lpGBT: setting USER_ID to 0x%08X",lpgbt_user_id(0x00FF)) ;
	if(user_id!=0xAAFF) {
		LOG(WARN,"    lpGBT power-cycled...") ;
	}

	lpgbt_vtrx() ;

	// enable slight brownout config at 1V
	lpgbt_write(0x03D,(1<<3)|4) ;

	// disable DAC
	lpgbt_write(0x06A,0) ;
	lpgbt_write(0x06B,0) ;
	lpgbt_write(0x06C,0) ;
	lpgbt_write(0x06D,0) ;

	// reset temperature sensor
	lpgbt_write(0x122,(1<<5)) ;
	lpgbt_write(0x122,0) ;
	
	// test patterns and modes
	// first clear!
	lpgbt_write(0x129,0) ;	
	lpgbt_write(0x12A,0) ;
	lpgbt_write(0x12B,0) ;
	lpgbt_write(0x12C,0) ;
#if 0
	mode = 6 ;	//4:fixed pattern; 6:fcmd loopback
	LOG(WARN,"lpGBT: Using test pattern %d",mode) ;
	lpgbt_write(0x129,(mode<<3)|(mode<<0)) ;	// uplink groups 1 i 0
	lpgbt_write(0x12A,(mode<<3)|(mode<<0)) ;	// uplink groups 3 i 2
	lpgbt_write(0x12B,(mode<<3)|(mode<<0)) ;	// uplink groups 5 i 4
	lpgbt_write(0x12C,(mode<<0)) ;			// uplink groups 6
#endif

	// pattern word
	lpgbt_write(0x12E,0x1A) ;
	lpgbt_write(0x12F,0x2B) ;
	lpgbt_write(0x130,0x3C) ;
	lpgbt_write(0x131,0x4D) ;	// shows up as the least 8 bits


	// General Setup
	lpgbt_gpio(-1,0) ;


	lpgbt_adc(0) ;		// will switch of VREF and ADC after it finishes
//	lpgbt_monitor(level) ;



	// Output Clocks

	// Enable Clocks for the ETL RB2.2
	u_int clock_mask = 0xFFFFFFFF;
#if 1
	clock_mask = (1<<20) ;			// for SMA
	clock_mask |= 0x3F ;			// 6 ASICs
	clock_mask |= 0x3F << 22 ;		// 6 ASICs
#endif
	int clock_freq = 4 ;
	int real_freq ;
	switch(clock_freq) {
	case 1 :
		real_freq = 40 ;
		break ;
	case 4 :
		real_freq = 320 ;
		break ;
	default :
		real_freq = 0 ;
		break ;
	}

	LOG(INFO,"lpGBT: setting downlink clock freq to %3d MHz; mask 0x%08X",real_freq,clock_mask) ;

	for(int i=0;i<29;i++) {
		u_int reg ;

		int ena = 0 ;

		if(clock_mask & (1<<i)) ena = 1 ;
		else ena = 0 ;

		// CLOCK "i":

		reg = 0x06E + i*2 ;

		int strength = 4 ;	// drive strength: 2=1.5 mA
		int invert = 0 ;
		if(ena) {
			lpgbt_write(reg,(invert<<6)|(strength<<3)|(clock_freq<<0)) ;
		}
		else {
			lpgbt_write(reg,0) ;
		}

		LOG(DBG,"lpGBT: clock %d, enable %d, freq %3d MHz (@reg 0x%03X)",i,ena,real_freq,reg) ;

		reg++ ;

		// preemphasis
		strength = 1 ;	// pre_emphasis strength 2=1.5 mA
		int mode = 0 ;	// pre_emphasis mode: 0=disabled
		int width = 0 ;	// pre_emphasis width: 0=120 ps

		if(ena) {
			lpgbt_write(reg,(strength<<5)|(mode<<3)|(width<<0)) ;
		}
		else {
			lpgbt_write(reg,0) ;
		}
	}

	// Output EDOUT or TX or downlink
	// set to 320; mirror
	int rate = 3 ;	// 320 Mbps
	int real_rate ;
	switch(rate) {
	case 3 :
		real_rate = 320 ;
		break ;
	default :
		real_rate = 0 ;
		break ;
	}

	// ETL RB2.2 has 6 outputs
	//	00,02
	//	10
	//	20,22
	//	30
	u_char edout_ena[4] ;

	edout_ena[0]=0xF ;
	edout_ena[1]=0xF ;
	edout_ena[2]=0xF ;
	edout_ena[3]=0xF ;

#if 0	// ETL RB2.2
	edout_ena[0]=0x5 ;
	edout_ena[1]=0x1 ;
	edout_ena[2]=0x5 ;
	edout_ena[3]=0x1 ;
#endif

	edout_ena[0] = 0x5 ;
	edout_ena[1] = 0x5 ;
	edout_ena[2] = 0x5 ;
	edout_ena[3] = 0x5 ;

	LOG(INFO,"lpGBT: setting downlink/EDOUT rate to %3d Mbs",real_rate) ;
	for(int g=0;g<4;g++) {
		LOG(TERR,"lpGBT: EDOUT group %d, enabled 0x%X",g,edout_ena[g]) ;
	}

	lpgbt_write(0x0A8,(rate<<6)|(rate<<4)|(rate<<2)|(rate<<0)) ;

	lpgbt_write(0x0A9,0xF) ;	// mirror all groups

	lpgbt_write(0x0AA,(edout_ena[1]<<4)|edout_ena[0]) ;	// enable all 4 channels in groups 0 & 1 
	lpgbt_write(0x0AB,(edout_ena[3]<<4)|edout_ena[2]) ;	// enable all 4 channels in groups 2 & 3

	lpgbt_write(0x0AC,0) ;		// disable EC data
	lpgbt_write(0x0AD,0) ;		// disable EC data



	int pre_strength = 2 ;	// pre-emphasis strength: 2=1.5 mA
	mode = 0 ;		// 0=disabled
	int drive = 2 ;		// drive strength: 2=1.5 mA

	int i = 0 ;
	for(int reg=0x0AE;reg<=0x0BD;reg++) {
//		int group = i/4 ;
//		int ch = i%4 ;

		lpgbt_write(reg,(pre_strength<<5)|(mode<<3)|(drive<<0)) ;

//		LOG(NOTE,"lpGBT: EDOUT drive: ch %d [%d%d]: enabled %d (@reg 0x%03X)",
//		    i,group,ch,drive,reg) ;
		i++ ;
	}

	i = 0 ;
	int pre_width = 2 ;     // 2=1.5 mA
	for(int reg=0x0BE;reg<=0x0C5;reg++) {
		lpgbt_write(reg,(pre_width<<4)|(pre_width<<0)) ;

//		LOG(NOTE,"lpGBT: EDOUT pre_width: ch %d & %d (@reg 0x%03X)",i,i+1,reg) ;
		i += 2 ;
	}

	lpgbt_write(0x0C6,0) ;	// decrease the power supply resistance for group 0 & 1 ;
	lpgbt_write(0x0C7,0) ;	// decrease the power supply resistance for group 2 & 3 ;

	/****************** EDIN ************************************/


	// EPRXDllConfig (0xF1, 0xF2, 0xF3)
	lpgbt_write(0xF1,(1<<6)|(2<<4)|(0<<3)|(0<<2)|(0<<1)|(0<<0)) ;	// EPRXDllConfig
	lpgbt_write(0xF2,(5<<3)|(5<<0)) ;				// EPRXLockFilter
	lpgbt_write(0xF3,5) ;					// EPRXLockFilter2


	// set to 320
	rate = 1 ;	// 1=320@10Gbs
	mode = 2 ;	// track mode: 0=fixed phase, etc...

	switch(rate) {
	case 1 :
		real_rate = 320 ;
		break ;
	default :
		real_rate = 0 ;
		break ;
	}

	LOG(INFO,"lpGBT: setting uplink/EDIN rate to %3d Mbs",real_rate) ;
	u_char edin_ena[7] ;

	edin_ena[0] = 0xF ;
	edin_ena[1] = 0xF ;
	edin_ena[2] = 0xF ;
	edin_ena[3] = 0xF ;
	edin_ena[4] = 0xF ;
	edin_ena[5] = 0xF ;
	edin_ena[6] = 0xF ; 

#if 1
	edin_ena[0] = 0x5 ;
	edin_ena[1] = 0x5 ;
	edin_ena[2] = 0x5 ;
	edin_ena[3] = 0x5 ;
	edin_ena[4] = 0x5 ;
	edin_ena[5] = 0x5 ;
	edin_ena[6] = 1; 
#endif
	i = 0 ;
	for(int reg=0x0C8;reg<=0xCE;reg++) {	// go over 7 groups of input
		lpgbt_write(reg,(edin_ena[i]<<4)|(rate<<2)|(mode<<0)) ;	// enable group 

		LOG(TERR,"lpGBT: EDIN enable group %d 0x%X (@0x%03X)",i,edin_ena[i],reg) ;
		i++ ;
	}

	lpgbt_write(0x0CF,0) ;	// disable EC EDIN

	i = 0 ;
	// sets termination, phase, bias etc...

	int phase = 0 ;
	int invert = 0 ;
	int ac_bias = 0 ;
	int termination = 1 ;
	int eq_bit1 = 0 ;

	for(int reg=0x0D0;reg<=0xEB;reg++) {
		lpgbt_write(reg,(phase<<4)|(invert<<3)|(ac_bias<<2)|(termination<<1)|(eq_bit1<<0)) ;	// enable 100 ohm

//		LOG(NOTE,"lpGBT: EDIN termination ch %d 0x%03X",i,reg) ;
		i++ ;
	}

	lpgbt_write(0x0EC,0) ;	// disable EC EDIN


	i = 0 ;
	int eq_bit0 = 0x00 ;	// 8 channels at once
	// also sets the pahse etc.
	LOG(WARN,"lpGBT: EDIN eq_control is unclear in the Manual") ;
	for(int reg=0x0ED;reg<=0x0F0;reg++) {
		// WARN: Manual seems buggy!
		lpgbt_write(reg,eq_bit0) ;

//		LOG(NOTE,"lpGBT: EDIN eq_bit0 ch [%d..%d] 0x%03X",i,i+3,reg) ;
		i += 4 ;
	}


//	lpgbt_gpio(8,1) ;	// lower ETROC RESET1
//	lpgbt_gpio(11,1) ;	// lower ETROC RESET2
//	lpgbt_gpio(8,0) ;	// lower ETROC RESET1
//	lpgbt_gpio(11,0) ;	// lower ETROC RESET2

	// 1,1 works and shows 0x64
	// 0,0 doesn;t work
	// 0,1 doesn't work
	// 1,0 works and shows 0x64
	lpgbt_gpio(8,1) ;	// 
	lpgbt_gpio(11,1) ;	// 



	lpgbt_user_id(0xAAFF) ;

	LOG(INFO,"lpGBT: init(%d) complete: 0x%X, USER_ID set to 0x%08X",level,ret,lpgbt_user_id(0xFFFFFFFF)) ;

	return ret ;
}

/**************************************************

	Generally switch of power consumers,
	noise generators.

	Read Monitor data at run-start.


**************************************************/

int dam_c::lpgbt_run_start()
{
	int ret = 0 ;

	// switch of unnecessary power
	// switch of noise generators (e.g. I2C)

	return ret ;
}

/************************************************

	Level:
	0 -- normal, fast,
	100 -- from complete power loss

*************************************************/

int dam_c::lpgbt_send_config(int level)
{
	int ret = 0 ;


	return ret ;
}

/****************************************

	Usually nothing much

****************************************/

int dam_c::lpgbt_run_stop()
{
	int ret = 0 ;



	return ret ;
}

/*************************************************************
	Called every N seconds during idle times

	Data obtained will be sent to Monitoring/Databases/logs

	The higher the level, the more data is read/sent

**************************************************************/

int dam_c::lpgbt_monitor(int level)
{
	int ret = 0 ;
	u_int hihi, hi, lo ;

	// Timout/Brownout Monitors
	// PLL timeout

	lpgbt.mon.pll_timeout[0] = lpgbt_read(0x1DE) ;
	lpgbt.mon.pll_timeout[1] = lpgbt_read(0x1DF) ;
	lpgbt.mon.pll_timeout[2] = lpgbt_read(0x1E0) ;

	// Watchdogs
	lpgbt.mon.wdog[0] = lpgbt_read(0x1DA) ;
	lpgbt.mon.wdog[1] = lpgbt_read(0x1DB) ;
	lpgbt.mon.wdog[2] = lpgbt_read(0x1DC) ;

	// Brownout
	lpgbt.mon.brownout = lpgbt_read(0x1DD) ;
	if(lpgbt.mon.brownout) {
		LOG(ERR,"lpGBT; brownout 0x%02X",lpgbt.mon.brownout) ;
		lpgbt_write(0x1DD,0) ;
	}

	// PORBOR
	lpgbt.mon.porbor = lpgbt_read(0x1D8) ;
	if(lpgbt.mon.porbor) {
		LOG(ERR,"lpGBT; PORBOR 0x%02X",lpgbt.mon.porbor) ;
	}

//	LOG(NOTE,"lpGBT: porbor 0x%02X, brownout 0x%02X",lpgbt.mon.porbor,lpgbt.mon.brownout) ;

	for(int i=0;i<3;i++) {
		u_int reg = 0x1DE ;
		if(lpgbt.mon.pll_timeout[i]) {	
			LOG(ERR,"lpGBT: PLL timeout %d: %d",i,lpgbt.mon.pll_timeout[i]) ;
			lpgbt_write(reg+i,0) ;
		}

		reg = 0x1DA ;
		if(lpgbt.mon.wdog[i]) {	
			LOG(ERR,"lpGBT: Watchdog %d: %d",i,lpgbt.mon.wdog[i]) ;
			lpgbt_write(reg+i,0) ;
		}

		//LOG(NOTE,"lpGBT: timeout %d: %u %u",i,lpgbt.mon.pll_timeout[i],lpgbt.mon.wdog[i]) ;
	}


	
	// SEU Monitors
	// enable counter... now or at init time!?
//	lpgbt_write(0x127,(1<<3)|(1<<0)) ;	// processandseumonitor: seu & process enable
	lpgbt_write(0x127,(1<<3)) ;	// processandseumonitor: seu & process enable

	hi = lpgbt_read(0x1ba);	// seucounth
	lo = lpgbt_read(0x1bb) ;	// seucountl
	lpgbt.mon.seu = (hi<<8)|lo ;
	if(lpgbt.mon.seu) {	
		LOG(WARN,"lpGBT: SEU %d",lpgbt.mon.seu) ;
	}
	// disable counter...
	lpgbt_write(0x127,0) ;	// processandseumonitor


	// Process Monitors: this needs to be saved to dbas
	// for long-term TID radiation monitoring
	for(int ch=0;ch<4;ch++) {

//		lpgbt_write(0x127,(ch<<1)|(1<<3)|(1<<0)) ;	// processandseumonitor: seu & process enable
		lpgbt_write(0x127,(ch<<1)|(1<<0)) ;	// processandseumonitor: seu & process enable

		// wait until done
		int ok = 0 ;
		for(int i=0;i<10000;i++) {
			lo = lpgbt_read(0x1b6) ;	// processmonitorstatus
			if(lo & (1<<1)) {
				ok = 1 ;
				break ;
			}
		}

		if(!ok) {
			LOG(ERR,"lpGBT: ProcessMonitor: ch %d: failed",ch) ;
			ret |= 1 ;
			break ;
		}
		
		hihi = lpgbt_read(0x1b7) ;	// pmfreqa
		hi = lpgbt_read(0x1b8) ;	// pmfreqb
		lo = lpgbt_read(0x1b9) ;	//pmfreqc

		// these values keep counting up -- not sure what to do with them???
		lpgbt.mon.process_freq[ch] = (hihi<<16)|(hi<<8)|lo ;

//		//lpgbt_write(0x127,(ch<<1)|(1<<3)) ;	// disable process monitor
		lpgbt_write(0x127,(ch<<1)) ;	// disable process monitor

		LOG(DBG,"lpGBT: ProcessMonitor: ch %d: %u",ch,lpgbt.mon.process_freq[ch]) ;
	}

	// GPIO? Do we have inputs at all?
	lpgbt_gpio(-1,0) ;

	// ADC values: lpGBT local
	// ADC value: MUX64
	lpgbt_adc(-1) ;

	// other?

	return ret ;
}


// Set DAC to 1.0V
int dam_c::lpgbt_dac()
{
	int vref = 4095 ;


	lpgbt_write(0x01C,0x80) ;		// VREF enable
	lpgbt_write(0x01D,0x63) ;		// VREFTUNE

	lpgbt_write(0x6A,0x80|(vref>>8)) ;	// DACConfigH
	lpgbt_write(0x6B,vref&0xFF) ;		// DACConfigL

	return 0 ;
}

/***********************************************************

	Read ALL ADCs and dump them to internal array

***********************************************************/

int dam_c::lpgbt_adc(int ch)
{
	static int first ;
	u_short enable = (1<<2) ;
	u_short gainx2 = (0<<0) ;
	u_short convert = (1<<7) ;
	int min_ch, max_ch ;

	// Paragraph 13.1.6 for examples

	if(first==0) {
		int dac_vref = 4095 ;

		LOG(INFO,"lpGBT: adc_init") ;

		//ADCConfig:
		//	7 ADCConvert
		//	2 ADCEnable
		//	1..0 Gain
		lpgbt_write(0x123,enable|gainx2) ;	// adcconfig: enable, gainx2

		// ADCMon:	TEMPSensReset 5
		//		VDDmonEna	4
		//		VDDTXmonEna 3
		//		XDDRXmonEna 2
		//		VDDANmonEna	0
		lpgbt_write(0x122,(1<<5)|(1<<4)|(1<<3)|(1<<2)|(1<<0)) ;
		lpgbt_write(0x122,(0<<5)|(1<<4)|(1<<3)|(1<<2)|(1<<0)) ;
			   
		// calibration: vrefenable, vreftune 0x63


		// power-up
		lpgbt_write(0x01C,1<<7) ;		// vrefcntr; enable
		lpgbt_write(0x01D,0x63) ;		// vreftune

		
		lpgbt_write(0x06A,0xC0|(dac_vref>>8)) ;		// DACConfigH: enable voltage ADC
		lpgbt_write(0x06B,dac_vref&0xFF) ;		// DACConfigL
		lpgbt_write(0x06C,28) ;				// CURDACValue: 100uA
		lpgbt_write(0x06D,1) ;				// CURDAC ADC pin bitmask
		first = 1 ;
	}


	if(ch<0) {
		min_ch = 0 ;
		max_ch = 15 ;
	}
	else {
		min_ch = ch ;
		max_ch = ch ;
	}


	// configure single ended mode with VREF2 as the N-side
	for(int c=min_ch;c<=max_ch;c++) {
		u_short hi ;
		u_short lo ;

		// P is channel, N is VREF/2
		lpgbt_write(0x121, (c<<4) | 0xF) ; // ADCSelect

		//usleep(10000) ;

		// start conversion
		lpgbt_write(0x123,convert|enable|gainx2) ;

		//usleep(10000) ;

		int ok = 0 ;
		for(int i=0;i<100;i++) {
			hi = lpgbt_read(0x1CA) ;	// adcstatusH
			if(hi & (1<<6)) {
				ok = 1 ;
				break ;
			}
		}

		if(ok) {
			lo = lpgbt_read(0x1CB) ;		// adcstatusL

			lpgbt.mon.adc[c] = ((hi&0x3)<<8) | lo ;
		}
		else {
			LOG(ERR,"lpGBT: ADC %2d timeout",c) ;
		}

		lpgbt_write(0x123,enable|gainx2) ;	//adcconfig: clear convert
		usleep(10000) ;

		if(ch>=0) return lpgbt.mon.adc[c] ;
	}
			
		
		// power-down
//	lpgbt_write(0x01C,0) ;		// vrefctnr; disable
//	lpgbt_write(0x123,gainx2) ;	// adcconfig; disable, gainx2

	return -1 ;
}


u_short dam_c::lpgbt_gpio_init()
{
	u_int lo, hi ;


	for(int r=0x53;r<=0x5C;r+=2) {
		hi = lpgbt_read(r) ;
		lo = lpgbt_read(r+1) ;

		//LOG(NOTE,"lpGBT: GPIO init 0x%02X: 0x%02X%02X",r,hi,lo) ;
	}

	// write known defaults FIRST, before setting outputs
	lpgbt_write(0x55,lpgbt_gpio_cached>>8) ;
	lpgbt_write(0x56,lpgbt_gpio_cached&0xFF) ;


	u_short output = 0 ;

	switch(lpgbt_board_type) {
	case 0 :
		output = (1<<1)|(1<<2)|(1<<3) ;	// LEDs on RB2.2
		break ;
	case 1 :
		output = (1<<15)|(1<<11)|(1<<8)|0x3F ;		// for RB3
		break ;
	case 2 :
		output = 0xFFFF ;		// for RBv1
		break ;
	}

	// set direction to output
	lpgbt_write(0x53,output>>8) ;
	lpgbt_write(0x54,output&0xFF) ;


	// read back values
	hi = lpgbt_read(0x1AF) ;
	lo = lpgbt_read(0x1B0) ;

	hi = (hi<<8)|lo ;

	lpgbt.mon.gpio = hi ;

	LOG(INFO,"lpGBT: GPIO init: out mask 0x%04X, now 0x%04X, default 0x%04X",output,hi,lpgbt_gpio_cached) ;

	lpgbt_gpio_inited = 1 ;

	return hi ;
}

u_short dam_c::lpgbt_gpio(int pin, int on)
{
	u_int hi, lo ;

	if(!lpgbt_gpio_inited) {
		lpgbt_gpio_init() ;
	}


	if(pin>=0) {
		if(on) {
			lpgbt_gpio_cached |= (1<<pin) ;
		}
		else {
			lpgbt_gpio_cached &= ~(1<<pin) ;
		}

		lpgbt_write(0x55,lpgbt_gpio_cached>>8) ;
		lpgbt_write(0x56,lpgbt_gpio_cached&0xFF) ;
	}

	// read values
	hi = lpgbt_read(0x1AF) ;
	lo = lpgbt_read(0x1B0) ;

	hi = (hi<<8)|lo ;

	lpgbt.mon.gpio = hi ;

//	LOG(INFO,"GPIO: pin %d:%d: 0x%04X",pin,on,hi) ;

	return hi ;
}

u_short dam_c::lpgbt_i2c_read(int bus, u_char dev, u_short reg)
{
	u_short val = 0xFFFF ;
	int devs = 0 ;

	// defaults for bus 2
	int data0 = 0x110 ;
	int cmd = 0x114 ;
	int addr = 0x10F ;
	int status = 0x19B ;
	int read0 = 0x19D ;

	switch(bus) {
	case 0 :
		data0 = 0x102 ;
		cmd = 0x106 ;
		addr = 0x101 ;
		status = 0x171 ;
		read0 = 0x173 ;

		break ;
	case 1 :
		data0 = 0x109 ;
		cmd = 0x10d ;
		addr = 0x108 ;
		status = 0x186 ;
		read0 = 0x188 ;
		break ;
	default :	// or bus #2, where VTRX+ is
		break ;
	}

	lpgbt_write(data0,(1<<2)|(1<<0)) ;	// 1 byte; freq: 00=100kHz,01=200,10=400;11=1MHz
	lpgbt_write(cmd,0) ;	// write this in


	lpgbt_write(addr,dev) ;
	lpgbt_write(data0,reg) ;
	lpgbt_write(cmd,0x2) ;	// 1-byte write


	devs = 0 ;
	for(int i=0;i<10;i++) {
			
		u_int val = lpgbt_read(status) ;
		if(val==0) continue ;

		if(val != 0x40) {
			if(val & 0x04) {
			//	LOG(INFO,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
				devs++ ;
				break ;
			}
			else {
				LOG(WARN,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
			}
		}
	}

	if(devs==0) return 0xFFFF ;

	lpgbt_write(addr,dev) ;
	lpgbt_write(cmd,0x3) ;	// 1-byte read


	devs = 0 ;
	for(int i=0;i<10;i++) {
			
		u_int val = lpgbt_read(status) ;
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


	if(devs) val = lpgbt_read(read0) ;
	else val = 0xFFFF ;

	return val ;
}


u_short dam_c::lpgbt_i2c_write(int bus, u_char dev, u_short reg, u_char val)
{
	int devs = 0 ;

	// defaults for bus 2
	int data0 = 0x110 ;
	int data1 = 0x111 ;	//?
	int cmd = 0x114 ;
	int addr = 0x10F ;
	int status = 0x19B ;
//	int read0 = 0x19D ;

	switch(bus) {
	case 0 :
		data0 = 0x102 ;
		data1 = 0x103 ;
		cmd = 0x106 ;
		addr = 0x101 ;
		status = 0x171 ;
//		read0 = 0x173 ;

		break ;
	case 1 :
		data0 = 0x109 ;
		data1 = 0x10A ;
		cmd = 0x10d ;
		addr = 0x108 ;
		status = 0x186 ;
//		read0 = 0x188 ;
		break ;
	default :	// or bus #2, where VTRX+ is
		break ;
	}

	lpgbt_write(data0,(2<<2)|(1<<0)) ;	// 2 bytes; freq: 00=100kHz,01=200,10=400;11=1MHz
	lpgbt_write(cmd,0) ;	// write this in


	lpgbt_write(data0,reg) ;
	lpgbt_write(data1,val) ;
	lpgbt_write(cmd,0x8) ;

	lpgbt_write(addr,dev) ;
	lpgbt_write(cmd,0xC) ;	// 2-byte write


	devs = 0 ;
	for(int i=0;i<10;i++) {
			
		u_int val = lpgbt_read(status) ;
		if(val==0) continue ;

		if(val != 0x40) {
			if(val & 0x04) {
			//	LOG(INFO,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
				devs++ ;
				break ;
			}
			else {
				LOG(WARN,"lpGBT: I2C: dev 0x%02X: status 0x%02X",dev,val) ;
			}
		}
	}

	if(devs==0) return 0xFFFF ;

	return 0 ;
}	



int dam_c::lpgbt_i2c_scan(int bus)
{
	int devs = 0 ;

	// Bus #0 is NOT connected on ETL RB2.2
	// Bus #1 connected to ASICs
	// Bus #2: device 0x50 VTRX+, device 0x70 2nd lpGBT

	LOG(INFO,"lpGBT: I2C: bus %d: scan started...",bus) ;

	// defaults for bus 2
	int data0 = 0x110 ;
	int cmd = 0x114 ;
	int addr = 0x10F ;
	int status = 0x19B ;
	int config = 0x10e ;

	switch(bus) {
	case 0 :
		data0 = 0x102 ;
		cmd = 0x106 ;
		addr = 0x101 ;
		status = 0x171 ;
		config = 0x100 ;
		break ;
	case 1 :
		data0 = 0x109 ;
		cmd = 0x10d ;
		addr = 0x108 ;
		status = 0x186 ;
		config = 0x107 ;
		break ;
	default :	// or bus #2, where VTRX+ is
		break ;
	}

	lpgbt_write(config,(1<<3)|(1<<5)) ;
//	lpgbt_write(config,0) ;

	lpgbt_write(data0,1) ;	// freq etc: 1=200 kHz, 0=100 kHz
	lpgbt_write(cmd,0) ;	// write this in

	for(int d=1;d<=127;d++) {
		lpgbt_write(addr,d) ;
		lpgbt_write(data0,0) ;
		lpgbt_write(cmd,0x3) ;	// 1-byte read=0x3; 1-byte write=0x2 


		int got_something = 0 ;
		for(int i=0;i<10;i++) {
			
			u_int val = lpgbt_read(status) ;
			if(val==0) continue ;

			LOG(DBG,"I2C: b %d: d %3d = 0x%X %d",bus,d,val,i) ;
			got_something = 1 ;
			if(val==0x40) break ;

			if(val != 0x40) {
				if(val & 0x04) {
					LOG(INFO,"lpGBT: I2C: dev 0x%02X: status 0x%02X",d,val) ;
					devs++ ;
					break ;
				}
				else {
					LOG(WARN,"lpGBT: I2C: dev 0x%02X: status 0x%02X",d,val) ;
				}
			}
		}

		if(!got_something) {
			LOG(ERR,"I2C") ;
		}
	}

	LOG(INFO,"lpGBT: I2C: bus %d: found %d devices",bus,devs) ;

	return devs ;
}

int dam_c::clock_check(int do_log)
{
	u_int c[7] ;

	// reset
	reg_wp(0,19) ;


	if(!do_log) return 0 ;

	usleep(10000) ;
	
	for(int i=0;i<7;i++) {
		c[i] = reg_r(16+i+1) ;
	}

	LOG(INFO,"CLK_160 used %.3f",(float)c[0]/10000.0) ;
	LOG(INFO,"CLK_250 used %.3f",(float)c[1]/10000.0) ;
	LOG(INFO,"CLK_40 used %.3f",(float)c[2]/10000.0) ;
	LOG(INFO,"CLK_160_gth %.3f",(float)c[3]/10000.0) ;
	LOG(INFO,"CLK_160_gth_rx %.3f",(float)c[4]/10000.0) ;
	LOG(INFO,"CLK_250_pcie %.3f",(float)c[5]/10000.0) ;
	LOG(INFO,"CLK_fmc_98 %.3f",(float)c[6]/10000.0) ;

	// check frequency
	int f_gth = reg_r(16+1+3) ;
	int f_gth_rx = reg_r(16+1+4) ;

	if(abs(f_gth-f_gth_rx)>5) {
		LOG(ERR,"CLK: GTH clock TX %u differs from clock RX %u",f_gth,f_gth_rx) ;
	}
	

	return 0 ;
}

int dam_c::status()
{
	u_int v ;

	// General board status
	v = reg_r(16) ;
	u_int v_mask = 0xFF00FFFF ;	// kill of SDA which can be anything...

	int f_ix = 2 ;	// using so-called fiber 2

	if((v&((f_ix-1)<<28))) ;
	else if((v&v_mask) != 0xEE000000) {
		LOG(ERR,"slv100: 0x%08X",v) ;
	}

	if(!(v&(1<<21))) {
		LOG(ERR,"PCIe link NOT up.") ;
	}

//	Only with TEF0008
//	if(!(v&(1<<22))) {
//		LOG(ERR,"FMC CLK NOT locked.") ;
//	}
	 
	// check frequency
	int f_gth = reg_r(16+1+3) ;
	int f_gth_rx = reg_r(16+1+4) ;

	if(abs(f_gth-f_gth_rx)>5) {
		LOG(ERR,"GTH: clock TX %u differs from clock RX %u",f_gth,f_gth_rx) ;
	}
	

	// GTH status
	v = reg_r(0x400+20) ;
	u_int lp_stat = v & 0xFF ;
	u_int gt_stat = (v>>8)&0xFFF ;

	// Skipped the gtwiz_reset_rx_cdr_strobe_out because it
	// flickers and it is supposedly not stable
	if((gt_stat&0xFE) != 0xFE) {
		LOG(ERR,"gt_status: 0x%04X",gt_stat) ;
	}

	// analyze lp_stat
	int d_done, d_rdy ;
	int u_done, u_rdy, u_err, u_overrun ;

	d_done = lp_stat & (1<<0) ;
	d_rdy = lp_stat & (1<<1) ;
	u_done = lp_stat & (1<<2) ;
	u_rdy = lp_stat & (1<<3) ;
	u_err = lp_stat & (1<<4) ;
	u_overrun = lp_stat & (1<<5) ;

	if(!d_rdy) {
		LOG(ERR,"Downlink not ready!") ;
	}

	// should not happen
	if(reg_r(0x400+4)&(1<<3)) {
		LOG(ERR,"Waiting for lpgbt_snd...") ;
	}

	// Should not happen -- encapsulated in lpgbt_read/write
	if(d_done) {
		LOG(ERR,"Previous lpgbt_snd complete -- clearing.") ;
		LOG(ERR,"    Data was 0x%08X 0x%08X 0x%08X",reg_r(0x400+7),reg_r(0x400+6),reg_r(0x400+5)) ;
		reg_wp(0x400+4,3) ;
	}


	// not working well yet
	if(u_err) {
		LOG(ERR,"lpGBT Uplink ERROR") ;
		reg_wp(0x404,4) ;
	}

	if(!u_rdy) {
		LOG(ERR,"UPLINK NOT READY! ... but continuing anyway...") ;
	}

	// uplink done
	if(u_done) {
		lpgbt_rcv() ;
	}

	if(u_overrun) {
		LOG(ERR,"lpgbt rcv overrun -- clearing") ;
		reg_wp(0x400+4,5) ;
	}

	u_int asic_err = reg_r(0x400+16+3) ;

	if(asic_err) {
		LOG(ERR,"ASIC error 0x%08X",asic_err) ;
	}

	return 0 ;
}

u_int dam_c::lpgbt_read(u_short reg)
{
	u_int store[3] ;
//	u_char *dta = (u_char *)store ;
//	u_char parity ;
//	u_char parity_mask = 0 ;	// unused...


	store[0] = 0x80000000 | (reg<<8) | lpgbt_dev_id ;
	store[1] = 0 ;
	store[2] = 0 ;

#if 0
	dta[0] = 0x7E ;
	dta[1] = (lpgbt_dev_id<<1)|1 ;
	dta[2] = 0 ;
	dta[3] = 1 ;	// 1 word
	dta[4] = 0 ;
	dta[5] = reg & 0xFF ;
	dta[6] = (reg>>8)&0xFF ;
	dta[7] = 0 ;	// parity
	dta[8] = 0x7E ;
	dta[9] = 0xFF ;
	dta[10] = 0xFF ;
	dta[11] = 0xFF ;

	parity = (dta[1] xor dta[3] xor dta[4] xor dta[5] xor dta[6]) xor parity_mask ;
	dta[7] = parity ;
#endif

	reg_w(0x405,store[0]) ;
	reg_w(0x406,store[1]) ;
	reg_w(0x407,store[2]) ;

	reg_ws(0x400+4,3) ;	// go!

	//LOG(TERR,"lpgbt_snd 0x%08X",reg_r(0x400+4)) ;

	// wait for done
	int ok = 0 ;
	for(;;) {
		u_int lp_stat = reg_r(0x400+20) & 0xFF ;

		int d_done = lp_stat & (1<<0) ;
		if(d_done) {
			ok = 1 ;
			break ;
		}
	}

	if(!ok) {
		LOG(ERR,"lpgbt_read: 0x%08X 0x%08X 0x%08X",store[2],store[1],store[0]) ;
		return 0xFFFF ;
	}
	
	reg_wp(0x400+4,3) ;	// clear

//	LOG(NOTE,"lpgbt_read: 0x%08X 0x%08X 0x%08X",store[2],store[1],store[0]) ;

	// now check the receive part
	u_int val = lpgbt_rcv() ;

//	LOG(TERR,"lpgbt_read(0x%03X) = 0x%02X",reg,val) ;

	return val ;
}

u_int dam_c::lpgbt_write(u_short reg, u_char val)
{
	u_int store[3] ;
//	u_char *dta = (u_char *)store ;
//	u_char parity ;
//	u_char parity_mask = 0 ;	// unused...

	store[0] = (reg << 8) | lpgbt_dev_id ;
	store[1] = val ;
	store[2] = 0 ;

#if 0	
	dta[0] = 0x7E ;
	dta[1] = (lpgbt_dev_id<<1)|0 ;
	dta[2] = 0 ;
	dta[3] = 1 ;	// 1 word
	dta[4] = 0 ;
	dta[5] = reg & 0xFF ;
	dta[6] = (reg>>8)&0xFF ;
	dta[7] = val ;
	dta[8] = 0 ;	// parity
	dta[9] = 0x7E ;
	dta[10] = 0xFF ;
	dta[11] = 0xFF ;

	parity = (dta[1] xor dta[3] xor dta[4] xor dta[5] xor dta[6] xor dta[7]) xor parity_mask ;
	dta[8] = parity ;
#endif

	reg_w(0x405,store[0]) ;
	reg_w(0x406,store[1]) ;
	reg_w(0x407,store[2]) ;

	reg_ws(0x400+4,3) ;	// go!

	// wait for done
	int ok = 0 ;
	for(;;) {
		u_int lp_stat = reg_r(0x400+20) & 0xFF ;

		int d_done = lp_stat & (1<<0) ;
		if(d_done) {
			ok = 1 ;
			break ;
		}
	}

	if(!ok) {
		LOG(ERR,"lpgbt_write: 0x%08X 0x%08X 0x%08X",store[2],store[1],store[0]) ;
		return -1 ;
	}
	
	reg_wp(0x400+4,3) ;	// clear

//	LOG(NOTE,"lpgbt_write: 0x%08X 0x%08X 0x%08X",store[2],store[1],store[0]) ;

	u_int valx = lpgbt_rcv() ;

//	LOG(TERR,"lpgbt_write(0x%03X,0x%02X) = 0x%02X",reg,val,valx) ;

	return valx ;
}


static u_int reverse(u_int in) ;	// used only in lpgbt_rcv

u_int dam_c::lpgbt_rcv()
{

	
	u_int store[3] ;
	u_int r_store[3] ;
	u_char parity ;
	u_char parity_mask = 0 ;	// not sure what I do with this

	u_int lp_stat = reg_r(0x400+20) & 0xFF ;
	u_int u_done = lp_stat & (1<<2) ;

	if(!u_done) {
		return 0xFFFF ;
	}

	store[0] = reg_r(0x400+16+5) ;
	store[1] = reg_r(0x400+16+6) ;
	store[2] = reg_r(0x400+16+7) ;

	reg_wp(0x400+4,6) ;

	for(int i=0;i<3;i++) {
		r_store[i] = reverse(store[i]) ;
	}

	u_char *dta = (u_char *)r_store ;

#if 0
	u_char id = dta[1]>>1 ;
	u_char rw = dta[1]&1 ;

	u_short nb = (dta[4]<<8) | dta[3] ;
	u_short reg = (dta[6]<<8) | dta[5] ;
	u_char val = dta[7] ;

	LOG(NOTE,"lpGBT rcv: 0x%02X[%c]: reg 0x%04X = 0x%02X [nb %d]",id,rw?'R':'W',reg,val,nb) ;


	// parity
//	parity = (dta[1] xor dta[3] xor dta[4] xor dta[5] xor dta[6] xor dta[7]) xor parity_mask ;
	parity = (dta[1] xor dta[2] xor dta[3] xor dta[4] xor dta[5] xor dta[6] xor dta[7]) xor parity_mask ;

	LOG(NOTE,"lpGBT parity 0x%02X",parity) ;

	if(parity != dta[8]) {
		LOG(ERR,"lpGBT: reg 0x%03X, val 0x%02X -- bad parity",reg,val) ;
	}

	for(int i=0;i<12;i++) {
		LOG(NOTE,"lpGBT datum %2d = 0x%02X",i,dta[i]) ;
	}
#endif

	// fix the bitstuffing
	u_char bits[96] ;
	for(int i=1;i<12;i++) {	// skip the first b"0111_1110"
		for(int j=0;j<8;j++) {
			bits[8*i+j] = dta[i] & (1<<j) ;
		}
	}

	u_char nbits[96] ;
	u_char cou = 0 ;
	u_char ix = 0 ;
	for(int i=8;i<96;i++) {
		if(bits[i]) {
			cou++ ;
			if(cou>=5) {
				//LOG(WARN,"bitslip %d",i) ;
				i++ ;
				cou = 0 ;
				nbits[ix++] = 1 ;
			}
			else {
				nbits[ix++] = 1 ;
			}
		}
		else {
			nbits[ix++] = 0 ;
			cou = 0 ;
		}
	}

//	LOG(TERR,"Bits %d",ix) ;

	cou = 0 ;
	for(int i=1;i<12;i++) {
		dta[i] = 0 ;
		for(int j=0;j<8;j++) {
			dta[i] |= (nbits[cou++]?1:0)<<j ;
		}
	}

//	u_char id = dta[1]>>1 ;
//	u_char rw = dta[1]&1 ;

//	u_short nb = (dta[4]<<8) | dta[3] ;
	u_short reg = (dta[6]<<8) | dta[5] ;
	u_char val = dta[7] ;

//	LOG(NOTE,"lpGBT rcv: 0x%02X[%c]: reg 0x%04X = 0x%02X [nb %d]",id,rw?'R':'W',reg,val,nb) ;


	// parity
//	parity = (dta[1] xor dta[3] xor dta[4] xor dta[5] xor dta[6] xor dta[7]) xor parity_mask ;
	parity = (dta[1] xor dta[2] xor dta[3] xor dta[4] xor dta[5] xor dta[6] xor dta[7]) xor parity_mask ;

//	LOG(NOTE,"lpGBT parity 0x%02X",parity) ;

	if(parity != dta[8]) {
		LOG(ERR,"lpGBT: reg 0x%03X, val 0x%02X -- bad parity",reg,val) ;
		return 0xFFFF ;
	}

//	for(int i=0;i<12;i++) {
//		LOG(NOTE,"lpGBT datum %2d = 0x%02X",i,dta[i]) ;
//	}

	return val ;
}

#if 0
u_short dam_c::fmc_si5345_page(int page)
{
	u_char ack = 0 ;
	i2c_device = 105 ;

	i2c_start() ;
	ack |= i2c_write((i2c_device<<1)|0) ;
	if(ack) return 0x1FF ;
	
	ack |= i2c_write(1) ;
	ack |= i2c_write(page) ;
	i2c_stop() ;

	if(ack) {
		LOG(ERR,"Si5345_page(%d)",page) ;
		return 0x2FF ;
	}
	return 0 ;

}

u_short dam_c::fmc_si5345_config()
{
	u_short ack = 0 ;

	i2c_device = 105 ;

	for(int i=0;i<SI5345_REVB_REG_CONFIG_NUM_REGS;i++) {
		u_short reg = si5345_revb_registers[i].address ;
		u_char val = si5345_revb_registers[i].value ;

		LOG(TERR,"Si5345: %d: reg 0x%04X = 0x%02X",i,reg,val) ;

		int page = reg>>8 ;

		ack |= fmc_si5345_page(page) ;
		ack |= i2c_reg_wr(reg&0xFF,val) ;

		if(ack) {
			LOG(ERR,"ack is 0x%03X",ack) ;
		}

		if(i==2) {
			usleep(10000) ;
		}
	}

	fmc_si5345_page(0) ;

	return ack ;
}
#endif

static int ser_open(const char *c_dev)
{
	int ret = -1 ;
	int dev = -1 ;
	int baud = 115200 ;

	dev = open(c_dev,O_RDWR|O_NOCTTY|O_NONBLOCK|O_NDELAY,0666) ;

	if(dev<0) return dev ;

	LOG(INFO,"%s: opened, baud %d",c_dev,baud) ;

	struct termios ts ;
	memset(&ts,0,sizeof(ts)) ;
	cfmakeraw(&ts) ;
	cfsetspeed(&ts,baud) ;
	ts.c_cflag |= CLOCAL | CREAD | CSTOPB ;
	tcflush(dev,TCIOFLUSH) ;

	ret = tcsetattr(dev,TCSANOW, &ts) ;
	if(ret < 0) {
		LOG(ERR,"%s: tcsetattr [%s]",c_dev,strerror(errno)) ;
		return ret ;
	}

	return dev ;
}

static int ser_read(int desc, char *b)
{
	int sret = 0 ;
	char *c = b ;

	for(;;) {
		char ch ;
		int ret = read(desc,&ch,1) ;
		
		if(ret<0) {
			if(errno==EAGAIN) continue ;
			return -1 ;
		}
		if(ret==0) {
			return -1 ;	// can't be??
		}

		if(ch=='\n') break ;

		*c++ = ch ;
		sret++ ;
	}

	*c++ = 0 ;

	return sret ;
}

static int ser_write(int desc, const char *b)
{
	int bytes = strlen(b) ;
	
	int ret = write(desc,b,bytes) ;
	if(ret != bytes) return -1 ;

	return bytes ;

}


char *dam_c::hwicap_version(u_int v)
{
        static char cv[32] ;

        int s = v & 0x3F ;
        int m = (v>>6)&0x3F ;
        int h = (v>>12)&0x1F ;
        int y = ((v>>17)&0x3F) ;
        int mo = (v>>23)&0xF ;
        int d = (v>>27)&0x1F ;


        sprintf(cv,"%02d-%02d-%02d %02d:%02d:%02d",
                mo,d,y,h,m,s) ;

        return cv ;


} ;


u_int dam_c::reg_r(int reg)
{
	char cmd[64] ;
	u_int val ;
	int ret = 0 ;

	if(dam_mode==0) {
		return *(slv100+reg) ;
	}


	sprintf(cmd,"r %u\n",reg) ;
	ret = ser_write(usb_desc,cmd) ;
	if(ret<0) {
		LOG(ERR,"ser_write: [%s]",strerror(errno)) ;
		return 0 ;
	}
	ret = ser_read(usb_desc,cmd) ;
	if(ret<0) {
		LOG(ERR,"ser_read: [%s]",strerror(errno)) ;
		return 0 ;
	}

	if(sscanf(cmd,"R %d = 0x%X",&reg,&val)!=2) {
		LOG(ERR,"reg_r: [%s]",cmd) ;
		return 0 ;
	}

	return val ;
}

u_int dam_c::reg_w(int reg, u_int val)
{
	char cmd[64] ;
	int ret = 0 ;

	if(dam_mode==0) {
		*(slv100+reg) = val ;
		return *(slv100+reg) ;
	}

	sprintf(cmd,"w %u,0x%X\n",reg,val) ;
	ret = ser_write(usb_desc,cmd) ;
	if(ret<0) {
		LOG(ERR,"ser_write: [%s]",strerror(errno)) ;
		return 0 ;
	}
	ret = ser_read(usb_desc,cmd) ;
	if(ret<0) {
		LOG(ERR,"ser_read: [%s]",strerror(errno)) ;
		return 0 ;
	}

	if(sscanf(cmd,"R %d = 0x%X",&reg,&val)!=2) {
		LOG(ERR,"reg_r: [%s]",cmd) ;
		return 0 ;
	}

	return val ;
}

u_int dam_c::reg_wp(int reg, int val)
{
	char cmd[64] ;
	int ret = 0 ;

	if(dam_mode==0) {
		*(slv100+reg) |= (1<<val) ;
		*(slv100+reg) &= ~(1<<val) ;
		return *(slv100+reg) ;
	}

	sprintf(cmd,"wp %u,0x%X\n",reg,val) ;
	ret = ser_write(usb_desc,cmd) ;
	if(ret<0) {
		LOG(ERR,"ser_write: [%s]",strerror(errno)) ;
		return 0 ;
	}
	ret = ser_read(usb_desc,cmd) ;
	if(ret<0) {
		LOG(ERR,"ser_read: [%s]",strerror(errno)) ;
		return 0 ;
	}

	if(sscanf(cmd,"R %d = 0x%X",&reg,&val)!=2) {
		LOG(ERR,"reg_r: [%s]",cmd) ;
		return 0 ;
	}

	return val ;
}

u_int dam_c::reg_ws(int reg, int val)
{
	char cmd[64] ;
	int ret = 0 ;

	if(dam_mode==0) {
		slv100[reg] |= (1<<val) ;
		return slv100[reg] ;
	}

	sprintf(cmd,"ws %u,0x%X\n",reg,val) ;
	ret = ser_write(usb_desc,cmd) ;
	if(ret<0) {
		LOG(ERR,"ser_write: [%s]",strerror(errno)) ;
		return 0 ;
	}
	ret = ser_read(usb_desc,cmd) ;
	if(ret<0) {
		LOG(ERR,"ser_read: [%s]",strerror(errno)) ;
		return 0 ;
	}

	if(sscanf(cmd,"R %d = 0x%X",&reg,&val)!=2) {
		LOG(ERR,"reg_r: [%s]",cmd) ;
		return 0 ;
	}

	return val ;
}

u_int dam_c::reg_wc(int reg, int val)
{
	char cmd[64] ;
	int ret = 0 ;

	if(dam_mode==0) {
		slv100[reg] &= ~(1<<val) ;
		return slv100[reg] ;
	}

	sprintf(cmd,"wc %u,0x%X\n",reg,val) ;
	ret = ser_write(usb_desc,cmd) ;
	if(ret<0) {
		LOG(ERR,"ser_write: [%s]",strerror(errno)) ;
		return 0 ;
	}
	ret = ser_read(usb_desc,cmd) ;
	if(ret<0) {
		LOG(ERR,"ser_read: [%s]",strerror(errno)) ;
		return 0 ;
	}

	if(sscanf(cmd,"R %d = 0x%X",&reg,&val)!=2) {
		LOG(ERR,"reg_r: [%s]",cmd) ;
		return 0 ;
	}

	return val ;
}


int dam_c::dam_count ;
volatile u_int *dam_c::resmem_glo ;
unsigned long dam_c::resmem_bytes_glo ;
unsigned long dam_c::resmem_hwaddr_glo ;


dam_c::~dam_c()
{
	LOG(INFO,"DAM %d: destructor",dam_ix) ;

	if(usb_desc>=0) {
		close(usb_desc) ;
		usb_desc = -1 ;
	}
}


int dam_c::init()
{
	const char *c_board = "???" ;

	lpgbt_gpio_cached = (1<<10)|(1<<11)|(1<<8) ;

	switch(lpgbt_board_type) {
	case 0 :	// ETL RB2
		c_board = "ETL RB2" ;
		break ;
	case 1 :	// ETL RB3
		c_board = "ETL RB3" ;
		break ;
	case 2:		// FTOF RBv1
		c_board = "FTOF RBv1" ;
		break ;
	default :
		LOG(ERR,"Unspecified RDO on the other end %d",lpgbt_board_type) ;
		break ;
	}


	LOG(INFO,"DAM %d:init: board type %s(%d), lpgbt id 0x%02X, default GPIO 0x%04X",
	    dam_ix, c_board, lpgbt_board_type,lpgbt_dev_id,lpgbt_gpio_cached) ;


	
	return 0 ;

}

dam_c::dam_c(const char *usb_dev)
{
	int cfd = -1 ;
	void *vptr = 0 ;
	u_int v ;


	if(usb_dev) dam_mode = 1 ;
	else dam_mode = 0 ;	// PCIe

//	dam_mode = mode ;

	usb_desc = -1 ;

	dam_ptr = 0 ;
	slv100 = slv160 = slv250 = 0 ;

	dam_ix = dam_count ;


	lpgbt_gpio_inited = 0 ;
	lpgbt_board_type = -1 ;
	lpgbt_dev_id = 0x73 ;	// typically for FTOF RB1

#if 0
	if(lpgbt_board==1) {
		lpgbt_gpio_cached = (1<<10)|(1<<11) | (1<<8) ;
	}
	else {
		lpgbt_gpio_cached = (1<<10)|(1<<11)|(1<<8) ;	// default values: RESET1 and RESET2 are active-low and so is LD_RSTN
	}

#endif



	LOG(INFO,"DAM %d: constructor: mode %s",
	    dam_ix,dam_mode==1?"USB":"PCIe") ;


	if(dam_mode) {
		//open usb here...
		usb_desc = ser_open(usb_dev) ;
		if(usb_desc<0) {
			LOG(CRIT,"DAM %d: ser_open %s [%s]",dam_ix,usb_dev,strerror(errno)) ;
		}
		dam_count++ ;
		return ;
	}

	if(dam_count==0) {
		u_int first_val = 0 ;

		// open resmem if we are the first DAM
		resmem_glo = 0 ;
		resmem_bytes_glo = 0 ;
		resmem_hwaddr_glo = 0x20000000ull ;


		// do it here...
		int fd = open("/dev/resmem",O_RDWR|O_SYNC) ;
		if(fd < 0) {
			LOG(CRIT,"resmem open [%s]",dam_ix,strerror(errno)) ;
			// don't return -- I might play with the PCIe slave only
		}

		if(ioctl(fd,RESMEM_IOC_HWADDR,&resmem_hwaddr_glo)<0) {
			LOG(ERR,"resmem ioctl [%s]",strerror(errno)) ;
		}
		if(ioctl(fd,RESMEM_IOC_LENGTH,&resmem_bytes_glo)<0) {
			LOG(ERR,"resmem ioctl [%s]",strerror(errno)) ;
		}

		vptr = mmap(0,resmem_bytes_glo,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0) ;

		if(vptr == MAP_FAILED) {
			LOG(CRIT,"resmem mmap [%s]",strerror(errno)) ;
			close(fd) ;
			// don't return -- perhaps I want to play with the PCIe slave
		}
		else {
			resmem_glo = (volatile u_int *) vptr ;

			first_val = *resmem_glo ;

			memset((void *)resmem_glo, 0xAB, resmem_bytes_glo) ;
		}

		LOG(INFO,"RESMEM: %u MB @ 0x%08X, (0x%08X)",resmem_bytes_glo/1024/1024,resmem_hwaddr_glo,first_val) ;
	}

	
	// and now open the PCIe image
	cfd = open(DAM_PCI_MEM,O_RDWR|O_SYNC) ;

	if(cfd < 0) {
		LOG(CRIT,"%s: open [%s]",DAM_PCI_MEM,strerror(errno)) ;
		goto no_dam ;
	}

	vptr = mmap(0,1024*1024,PROT_READ|PROT_WRITE,MAP_SHARED,cfd,0) ;

	if(vptr == MAP_FAILED) {
		LOG(CRIT,"%s: mmap [%s]",DAM_PCI_MEM,strerror(errno)) ;
		close(cfd) ;
		goto no_dam ;
	}
	

	dam_ptr = (u_int *)vptr ;
	slv100 = dam_ptr+0 ;
	slv160 = dam_ptr+(0x1000/4) ;
	slv250 = dam_ptr+(0x2000/4) ;
	
	dam_count++ ;

	v = reg_r(14) ;
	LOG(INFO,"FW: V-bit version 0x%08X [%s]",v,hwicap_version(v)) ;
	v = reg_r(15) ;
	LOG(INFO,"FW: V-all version 0x%08X [%s]",v,hwicap_version(v)) ;


	LOG(INFO,"clock_check") ;
	clock_check(1) ;
	

	return ;	// sucessful return

	no_dam:;	// no success

	if(cfd>=0) close(cfd) ;
	dam_ptr = slv100 = slv160 = slv250 = 0 ;	// to provoke an error if used

}

int dam_c::lpgbt_reset()
{
	int ret = 0 ;
	u_int val ;

	LOG(WARN,"lpGBT: issuing RESET -- expect some ERROR messages...") ;

	for(int i=0;i<10;i++) {

	// reset GTP
	//  RESET_I
	// pulse slv100_o(0)(4)
	reg_wp(0x000,4) ;
	usleep(10000) ;

	// reset lpgbt downlink
	//  CONTROL_I(2)
	// pulse lp_control
	// slv160_o(4)(2)
	reg_wp(0x404,2) ;
	usleep(10000) ;

	// reset lpgbt uplink
	//  CONTROL_I(1)
	// pulse lp_control
	// slv160_o(4)(1) 
	reg_wp(0x404,1) ;
	usleep(10000) ;


	lpgbt_write(0x128,0xC0) ;
	usleep(50000) ;
//	status() ;

	lpgbt_write(0x128,0x0) ;
	usleep(50000) ;
//	status() ;

	lpgbt_write(0x36,0x80) ;	// MUST be 0x80!!!!
	usleep(50000) ;
//	status() ;

	lpgbt_write(0xFB,0x6) ;
	usleep(50000) ;
//	status() ;

	val = lpgbt_read(0x1D7) ;
	usleep(10000) ;
	status() ;
	if(val==0xA6) {
		LOG(INFO,"lpGBT: RESET-to-READY successfull at attempt %d.",i+1) ;
		ret = 1 ;
		break ;
	}
	}

//	clock_check(1) ;

	if(ret==0) {
		LOG(ERR,"lpGBT: RESET failed!") ;
	}

	return ret ;
}

static u_int reverse(u_int in)
{
	u_int out = 0 ;

	for(int i=0;i<32;i+=2) {
		u_int s ;
		u_int n_s ;

		s = (in>>i)&0x3 ;

		n_s = 0 ;

		if(s&1) n_s |= 2 ;
		if(s&2) n_s |= 1 ;

		out |= (n_s<<i) ;
	}

	return out ;
}


void dam_c::i2c_scl(int on)
{
	if(on) {
		reg_ws(0,16) ;
	}
	else {
		reg_wc(0,16) ;
	}
}

void dam_c::i2c_sda(int on) 
{
	if(on) {
		reg_ws(0,17) ;
	}
	else {
		reg_wc(0,17) ;
	}
}

u_char dam_c::i2c_sdz()
{
	u_int ret ;

	reg_ws(0,17) ;	// set SDA to 1

	ret = reg_r(0+16) ;	// read SDA
	
	
	if(ret & (1<<17)) {
		return 1 ;
	}
	else return 0 ;
}

void dam_c::i2c_start()
{
	i2c_scl(1) ;
	i2c_sda(1) ;

	i2c_sda(0) ;
	i2c_scl(0) ;
}

u_char dam_c::i2c_stop()
{
	i2c_sda(0) ;
	i2c_scl(1) ;
	i2c_sda(1) ;

	u_char a = i2c_sdz() ;

	return a ;
}

u_char dam_c::i2c_write(u_char val)
{
	int i ;
	u_char ret ;

	for(i=7;i>=0;i--) {
		int bit = (val&(1<<i))?1:0 ;
		i2c_sda(bit) ;
		i2c_scl(1) ;
		i2c_scl(0) ;
	}

	i2c_sda(1) ;
	ret = i2c_sdz() ;

	i2c_scl(1) ;
	ret = i2c_sdz() ;
	i2c_scl(0) ;

	return ret ;
}

u_short dam_c::i2c_reg_wr(u_char reg, u_char val)
{
	u_char ack = 0 ;

	i2c_start() ;
	ack |= i2c_write((i2c_device<<1)|0) ;
	ack |= i2c_write(reg) ;
	ack |= i2c_write(val) ;

	if(ack) return (0x100|ack) ;
	return 0 ;
}

u_int dam_c::fmc_i2c_scan()
{
	int ret = 0 ;

	LOG(INFO,"Scanning I2C bus. This will take a while.") ;
	for(int i=0;i<128;i++) {
		u_char f ;

		i2c_start() ;
		f = i2c_write((i<<1)) ;
		if(f==0) {
			ret++ ;
			LOG(INFO,"I2C %d (0x%02X): OK",i,i) ;
		}
		i2c_stop() ;
	}
	LOG(INFO,"I2C scan: found %d devices",ret) ;

	return ret ;

}
