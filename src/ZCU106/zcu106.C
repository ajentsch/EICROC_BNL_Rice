#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <sys/file.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

#define ANSI_GREEN      "\033[32m"
#define ANSI_RED        "\033[31m"
#define ANSI_BLUE       "\033[34m"
#define ANSI_YELLOW     "\033[33m"
#define ANSI_MAGENTA    "\033[35m"
#define ANSI_CYAN       "\033[36m"
#define ANSI_BOLD       "\033[1m"
#define ANSI_ITALIC     "\033[3m"
#define ANSI_UNDERLINE  "\033[4m"
#define ANSI_REVERSE    "\033[7m"
#define ANSI_RESET      "\033[0m"

#include <LOG/rtsLog.h>
volatile int rtsLogLevel = 0 ;

static const char *usb_dev = "/dev/ttyUSB3" ;

static int ser_usb ;	// device
static u_char i2c_glo_shadow[255] ;	// to shadow I2C writes to the global registers
static u_int fw_flavor ;
static int asic_type = 1 ;			// EICROC0A, EICROC1...



int ser_open()
{	
//	int baud = 230400 ;
	int baud = 115200 ;
//	int baud = 19200 ;
	int block = 0 ;	// never block in this application...

	if(block==0) {
		ser_usb = open(usb_dev,O_RDWR|O_NOCTTY|O_NONBLOCK|O_NDELAY,0666) ;
	}
	else {
		ser_usb = open(usb_dev,O_RDWR|O_NOCTTY,0666) ;
	}

	if(ser_usb<0) {
		perror(usb_dev) ;
		return -1 ;
	}

	
	struct termios ts ;
	memset(&ts,0,sizeof(ts)) ;
	cfmakeraw(&ts) ;
	cfsetspeed(&ts,baud) ;
	//ts.c_cflag |= CLOCAL | CREAD | CSTOPB ;
	ts.c_cflag |= CLOCAL | CREAD ;
	ts.c_cflag &= ~CSTOPB ;
	tcflush(ser_usb,TCIOFLUSH) ;

	int ret = tcsetattr(ser_usb,TCSANOW,&ts) ;
	if(ret<0) {
		perror("tcsetattr: ") ;
		return -1 ;
	}

	return ser_usb ;

}

int ser_read(u_char *ch)
{
	int ret = read(ser_usb,ch,1) ;
	if(ret<0) {
		if(errno==EAGAIN) {
			return 0 ;
		}
		perror("ser_read") ;
		return -1 ;
	}
	if(ret==0) {
		perror("got 0??") ;
		return -1 ;
	}
	return ret ;
}

int ser_ln_read(char *buff)
{
	char *c = buff ;
	u_char ch ;
	int s_cou = 0 ;
	int sret = 0 ;

	for(;;) {
		int ret = ser_read(&ch) ;
		if(ret<0) {
			perror("ser_ln_read") ;
			return -1 ;
		}
		if(ret==0) {
			usleep(2) ;
			s_cou++ ;
			if(s_cou==500) {
				break ;
			}
			continue ;
		}

		if(ch=='\n') break ;

		*c++ = ch ;
		sret = 1 ;
	}

	*c++ = 0 ;

	return sret ;
}


int ser_write_b(const void *buffer, int bytes)
{
	int ret = write(ser_usb,buffer,bytes) ;

	if(ret != bytes) {
		perror("com_write") ;
	}

	usleep(100) ;

	return ret ;
}

int ser_write(const char *buffer)
{
	int bytes = strlen(buffer) ;
	return ser_write_b(buffer,bytes) ;
}

u_int rd(u_int reg)
{
	char cmd[16] ;
	u_int r ;
	u_int val ;
 
	sprintf(cmd,"r %u\n",reg) ;
	ser_write(cmd) ;

	ser_ln_read(cmd) ;
	int ret = sscanf(cmd,"R %u = 0x%X",&r,&val) ;

	if(ret==2 && r==reg) {
		return val ;
	}

	perror("rd") ;
	return 0 ;
}

u_int wr(u_int reg, u_int val)
{
	char cmd[32] ;
	u_int r ;

	sprintf(cmd,"w %u,0x%X\n",reg,val) ;
	ser_write(cmd) ;

	ser_ln_read(cmd) ;
	int ret = sscanf(cmd,"R %u = 0x%X",&r,&val) ;

	if(ret==2 && r==reg) {
		return val ;
	}

	perror("wr") ;
	return 0 ;
}


u_short i2c_wr(u_short reg, u_char val)
{

	char cmd[32] ;
	u_int r ;
	u_int err ;


	if((reg&0xFF00)==0x4000) {	// global register
		i2c_glo_shadow[reg&0xFF] = val ;
	}

	sprintf(cmd,"Ew %u,0x%X\n",reg,val) ;
	ser_write(cmd) ;

	ser_ln_read(cmd) ;
	int ret = sscanf(cmd,"EIC W 0x%X,0x%X: 0x%X",&r,&val,&err) ;

	if(ret==3 && r==reg) {
		return err ;
	}

	perror("i2c_wr") ;
	return 0xFFFF ;

}

u_short i2c_rd(u_short reg)
{
	char cmd[32] ;
	u_int r ;
	u_int val ;

	sprintf(cmd,"Er 0x%X\n",reg) ;
	ser_write(cmd) ;

	ser_ln_read(cmd) ;
	int ret = sscanf(cmd,"EIC R 0x%X: 0x%X",&r,&val) ;
	
	if(ret==2 && r==reg) {
		return val ;
	}

	perror("i2c_rd") ;
	return 0xFFFF ;
	
}


static int reg_cou ;
static struct reg_t {
	u_short reg ;
	u_char val ;
} regs[1034] ;

int open_py(const char *fname)
{
	FILE *f = fopen(fname,"r") ;

	if(f==0) {
		LOG(ERR,"%s: %s",fname,strerror(errno)) ;
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


void exit_recover()
{
	fprintf(stderr,"%s\n",ANSI_RESET) ;
	system("/bin/stty sane") ;
}

int kbhit()
{
	static int first ;
	u_char a ;

	if(first==0) {	// set stdin to non-blocking once
		fcntl(0,F_SETFL,fcntl(0,F_GETFL)|O_NONBLOCK) ;
		first = 1 ;
	}

	int ret = read(0,&a,1) ;
	if(ret==1) {
#if 0
		switch(a) {
		case 0x1B :
			system("/bin/stty sane") ;
			fprintf(stderr,"**** Exiting\n") ;
			exit(0) ;
		}
#endif
		return a ;
	}

	return -1 ;
}




int main(int argc, char *argv[])
{
	int ret ;
	int c ;
	int mode = -1 ;
	int sel_fcmd = 1 ;	// default is lpGBT mode
	int num_events = 1 ;
	int run_type = 1 ;	// default is pedestal
	u_char lane_mask = 1 ;
	const char *reg_values = 0 ;
	const char *c_asic  ;
	char use_dout0 = 0 ;

	time_t now ;

	rtsLogLevel = 2 ;	// set to WARN and anove

	while((c=getopt(argc,argv,"m:d:En:t:l:C:A:0D:")) != EOF) {
	switch(c) {
	case 'm' :	// execute batch command with argument...
		mode = atoi(optarg) ;
		break ;
	case 'E' :		// use legacy, non-FCMD, mode
		sel_fcmd = 0 ;
		break ;
	case 'd' :	// change USB device
		usb_dev = optarg ;
		break ;
	case 'n' :
		num_events = atoi(optarg) ;
		break ;
	case 't' :		// 1: pedestal, 2: pulser
		run_type = atoi(optarg) ;
		break ;
	case 'l' :	// lane mask
		if(sscanf(optarg,"0x%X",&lane_mask)==1) ;
		else lane_mask = atoi(optarg) ;
		break ;
	case 'C' :	// default register values file, usually .py
		reg_values = optarg ;
		break ;
	case 'A' :	
		asic_type = atoi(optarg) ;
		break ;
	case '0' :
		use_dout0 = 1 ;
		break ;
	case 'D' :
		rtsLogLevel = atoi(optarg) ;
		break ;
	}
	}

	// Enter interactive terminal mode...
	LOG(INFO,"%s Version %s %s: %s",argv[0],__DATE__,__TIME__,usb_dev) ;


	if(ser_open()<0) exit(-1) ;


	if(mode==-1) goto do_interactive ;


	if(mode==0 || (mode&1)) {	// SEND_CONFIG configuration phase; compatible with old style...
		char buff[128] ;
		int i2c_last ;

		fw_flavor = rd(7) ;
		LOG(INFO,"FW flavor: 0x%08X",fw_flavor) ;

		switch(asic_type) {
		case 0 :
			c_asic = "EICROC0A" ;
			use_dout0 = 1 ;	// automatic override!
			sel_fcmd = 0 ;	// automatic override
			lane_mask = 1 ;	// automatic override

			i2c_last = 0x4012 ;

			if(reg_values==0) reg_values = "eicroc0_defaults.py" ;

			if(fw_flavor==0xDEADC0DE) {
				LOG(ERR,"Old FW can't work with EICROC0A") ;
				return -1 ;
			}

			break ;
		case 1 :
		default :
			c_asic = "EICROC1" ;

			i2c_last = 0x401B ;

			if(reg_values==0) reg_values = "orig_reg_values.py" ;								
			break ;
		}


		LOG(INFO,"Configuring ASIC %s: use FCMD mode %c; from %s",c_asic,
		    sel_fcmd?'Y':'N',
		    use_dout0?"DOUT0":"SDOUT") ;



		LOG(INFO,"Using register values file \"%s\"",reg_values) ;
		open_py(reg_values) ;


		// get the FW version form ZCU106
		for(int i=0;i<10;i++) {		// flush junk
			ser_ln_read(buff) ;
		}
		ser_write("\n\n") ;		// a few ENTERs
		usleep(1000) ;
		ser_write("b\n") ;		// send the version comman
		for(int i=0;i<3;i++) {		// read the 3 version strings
			ser_ln_read(buff) ;
			LOG(INFO,"%s",buff) ;
		}
	

		// reset
		wr(2,0) ;	// reset last run, just in case

		wr(0,0) ;	// reset the ASIC


		// setup ZCU registers while the ASIC is in reset
		{
		int cmd_mode = 4 ;		// 4: DON'T issue CMDPULSE, 0: issue CMDPULSE
		int en_ack_to_cmd = 12 ;	// any length longer than at least 8
		int cmd_to_end_ack = 4 ;	// keep at 4 normally


		if(run_type==2) cmd_mode = 0 ;	// issue CMDPULSE
		else cmd_mode = 4 ;		// no CMDPULSE

		LOG(INFO,"Run_type %d, cmd_mode %s",run_type,cmd_mode==4?"PEDESTAL":"PULSER") ;

		wr(3,(en_ack_to_cmd<<8) | (cmd_to_end_ack)) ;
		wr(2,cmd_mode<<1) ;
		}


		// new FW features
		if(fw_flavor != 0xDEADC0DE) {
			u_int v ;

			LOG(WARN,"New FW 0x%08X: EXPERIMENTAL",fw_flavor) ;

			switch(asic_type) {
			case 1 :
			default :
				wr(4,6510) ;	// word count
				break ;
			case 0 :
				wr(4,820) ;	// word count
				break ;
			}



			// set either to use DOUT0 or SDOUT
			v = rd(0) ;

			if(use_dout0) v |= (1<<6) ;
			else v &= ~(1<<6) ;

			wr(0,v) ;
			

			v = 0 ;
			v |= (0<<1) ;		// delay from CLK40 to start of data; typically 1 or 0
			v |= (asic_type<<4) ;	// ROC tyoe

			switch(run_type) {
			default :
			case 1:
				v |= (0<<8) ;		// pedestal=0
				break ;
			case 2 :
				v |= (1<<8) ;		// CMDPULSE=1,
				break ;
			}


			wr(1,v) ;		// first write the values

			wr(1,v|(1<<11)) ;	// and then reset delay counter for the values to take...
			usleep(1000) ;
			wr(1,v) ;		// clear reset delay 

		}


		// and now other ASIC-dependent setups
		switch(asic_type) {

		char buff[128] ;

		case 0:	// EICROC0A

			wr(0,(1<<4)|(1<<5)|(1<<6)) ;		// set appropriate bits; leave in reset
			usleep(1000) ;
			wr(0,(1<<4)|(1<<5)|(1<<6)|0x3) ;	// out of reset, keep bits

			ser_write("2d 0x08\n") ;	// new: I2C address is different than EICROC1's 0x40
			ser_ln_read(buff) ;		// read but ignore the return string...

			usleep(100000) ;		// wait a bit after reset

			// dump global regs
			for(int i=0x4000;i<=i2c_last;i++) {	// 4012
				u_short v = i2c_rd(i) ;

				LOG(DBG,"EICROC0A defaults reg 0x%02X = 0x%02X",i,v) ;

			}

			for(int i=0;i<reg_cou;i++) {
				u_int reg = regs[i].reg ;

				if(reg<0x4000) continue ;	// skip writes to single pixel!

				ret = i2c_wr(reg,regs[i].val) ;

				LOG(NOTE,"I2C %d: write 0x%04X = 0x%02X",i,regs[i].reg,regs[i].val) ;
			}

			break ;

		case 1 :	// EICROC1

			if(sel_fcmd) sel_fcmd = (1<<3) | (1<<2) ;

			wr(0,sel_fcmd) ;	// set SEL_FCMD while keeping the ASIC in reset
			usleep(1000) ;		// wait a bit...
			wr(0,sel_fcmd|3) ;	// enable ASIC 
			usleep(100000) ;		// wait a bit more



			// load default register values
			for(int i=0;i<reg_cou;i++) {
					u_int reg = regs[i].reg ;

					if(reg<0x4000) continue ;	// skip writes to single pixel!

					ret = i2c_wr(reg,regs[i].val) ;

					LOG(NOTE,"I2C %d: write 0x%04X = 0x%02X",i,regs[i].reg,regs[i].val) ;
			}



			//		i2c_wr(0x400C,0) ;	// pulser

			// setup EICROC1 lanes
			u_char msk ;

			msk = 0 ;

			if(lane_mask&1) {
				msk |= 0x0F ;
			}
			if(lane_mask&2) {
				msk |= 0xF0 ;
			}

			i2c_wr(0x4013,msk) ;	// enable lanes aka columns 0..7

			msk = 0 ;

			if(lane_mask&4) {
				msk |= 0x0F ;
			}
			if(lane_mask&8) {
				msk |= 0xF0 ;
			}

			i2c_wr(0x4014,msk) ;	// enable lanes aka columns 0..7

			msk = 0 ;

			if(lane_mask&0x10) {
				msk |= 0x0F ;
			}
			if(lane_mask&0x20) {
				msk |= 0xF0 ;
			}

			i2c_wr(0x4015,msk) ;	// enable lanes aka columns 0..7

			msk = 0 ;

			if(lane_mask&0x40) {
				msk |= 0x0F ;
			}
			if(lane_mask&0x80) {
				msk |= 0xF0 ;
			}

			i2c_wr(0x4016,msk) ;	// enable lanes aka columns 0..7


			LOG(INFO,"Enabling lanes 0x%02X",lane_mask) ;

			// defaults
			u_char vth_corr = 0 ;
			u_char vref = 0 ;

		
			// sanity protection
			vth_corr &= 0x7F ;
	

			// DEFAULT for ALL pixels: MUST be done last due to metal-layer I2C bug
			LOG(INFO,"Setting per-pixel defaults: Vref %d, Vth %d",vref,vth_corr) ;

			i2c_wr(0x0001,0x80|vth_corr) ;	// 0x80 | vth_corr
			i2c_wr(0x0002,vref) ;		// vref

			if(run_type==2) {	// pulser
				i2c_wr(0x0003,0x04) ;	// on_ctest if 0x04 aka use pulser
			}
			else {
				i2c_wr(0x0003,0x00) ;	// on_ctest is off for other modes
			}
			i2c_wr(0x0004,0x01) ;	// from Kinaan
			i2c_wr(0x0005,0x20) ;	// from Kinaan



			break ;
		}


		// common to all ASICs
		// read those values back...
		for(int i=0x4000;i<=i2c_last;i++) {
			u_char shd = i2c_glo_shadow[i-0x4000] ;

			ret = i2c_rd(i) ;
			if(ret != shd) {
				LOG(WARN,"I2C read 0x%04X = should 0x%02X, is 0x%02X",i,shd,ret) ;
			}
			else {
				LOG(NOTE,"I2C read 0x%04X = should 0x%02X, is 0x%02X",i,shd,ret) ;
			}
		}



	} // if(mode...)
	

	if(mode&4) {	// special post-configuration thing...
		// generally do something on a per-pixel basis...
		u_short addr ;
		int column, row ;

		// FIRST: values I want for a particular pixel
		i2c_wr(0x0001,0x80) ;	// 0x80 | vth_corr
		i2c_wr(0x0002,0x00) ;	// vref

		if(run_type==2) {	// pulser
			i2c_wr(0x0003,0x04) ;	// on_ctest if 0x04 aka use pulser
		}
		else {
			i2c_wr(0x0003,0x00) ;	// on_ctest is off for other modes
		}

		i2c_wr(0x0004,0x01) ;
		i2c_wr(0x0005,0x20) ;



		column = 0 ;		// 0..31;5 bits; but only use 0..3 for my tests
		row = 31 ;		// 0..31;5 bits; sometimes called "line"

		// SECOND
		// use the correct pixel but set the values for ALL, typically disable on_ctest
		addr = 0x2000 | (column<<8) | (row<<3) ;	//had a bug: column was shifted 16

		i2c_wr(addr|1,0x80) ;	// vth for discriminator
		i2c_wr(addr|2,0x00) ;	// vref for ADC
		if(run_type==2) {	// pulser
			i2c_wr(addr|3,0x04) ;	// on_ctest if 0x04 aka use pulser
		}
		else {
			i2c_wr(addr|3,0x00) ;	// on_ctest is off for other modes
		}
		
		//i2c_wr(addr|3,0x00) ;	// on_ctest if 0x04
		i2c_wr(addr|4,0x01) ;	// leave as-is at 0x01
		i2c_wr(addr|5,0x20) ;	// leave as-is at 0x20


	}

	// VERY LAST
	if(mode&2) {
		// RUN_START

		fw_flavor = rd(7) ;

		LOG(INFO,"Readout: %d events, FW flavor 0x%08X",num_events,fw_flavor) ;

		for(int e=0;e<num_events;e++) {
		
		int w_cou = 0 ;
		now = time(0) ;
		ser_write("R 1\n") ;
		for(int i=0;i<10000;i++) {
			char buff[128] ;
			ser_ln_read(buff) ;

			if(strstr(buff,"End")) continue ;

			w_cou++ ;
			printf("Evt %d: %d = 0x%s\n",e,i,buff) ;

			// look for end of trailer
			if(fw_flavor==0xDEADC0DE) {	// old Jun 24, 2026 FW
				if(strcmp(buff,"8FFFFFFF")==0) break ;
			}
			else {	// if(fw_flavor==0x09112026) {
				if(strcmp(buff,"EEEEEC01")==0) break ;
			}
		}
		fflush(stdout) ;

		LOG(NOTE,"Done evt %d, %d words after %d secs...",e,w_cou,time(0)-now) ;
		}
	}

	if(mode != -1) return 0 ;

	do_interactive: ;

	printf("Entering interactive mode:\n") ;

	atexit(exit_recover) ;
	system("/bin/stty -icanon -echo") ;

	char line[256] ;
	int line_cou = 0 ;

	for(;;) {
		u_char ch ;
		int ret ;

		ret = ser_read(&ch) ;
		if(ret<0) {
			perror("ser_read") ;
			break ;
		}

		if(ret==0) {	// nothing to read
			int c = kbhit() ;	// check keyboard
			if(c>=0) {
				printf("%c",c) ;
				//fprintf(of,"%c",c) ;
				fflush(stdout) ;

				if(c!='\n') {
					line[line_cou] = c ;
					line_cou++ ;
				}
				else {
					line[line_cou] = c ;
					line_cou++ ;
					line[line_cou] = 0 ;

					//printf(">>> %s",line) ;
					for(u_int i=0;i<strlen(line);i++) {
						ser_write_b(line+i,1) ;
					}

					line_cou=0 ;
				}


			}

			continue ;
		}
		
		printf("%s%c%s",ANSI_BLUE,ch,ANSI_RESET) ;
		//printf("\n--- 0x%02X\n",ch) ;		

		//fprintf(of,"%c",ch) ;
		//fflush(of) ;
		//printf("--> 0x%02X [%c]\n",ch,ch) ;
	}

	return 0 ;
}
