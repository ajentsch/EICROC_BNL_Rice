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

// custom firmware/hardware testing utility

static const char *usb_dev = "/dev/ttyUSB3" ;

static int ser_usb ;	// device
//static int mode ;	// command mode

////////////////////////////////////////
// SERIAL PORT COMMUNICATION FUNCTIONS//
////////////////////////////////////////

// ser_open: opens and configures serial comm link to usb_dev
// depending on flag, opens in blocking or nonblocking (default) state
// nonblocking: comm path without delays due to other data processing
int ser_open()
{	
	//	int baud = 230400 ;
	// baud rate: speed at which devices communicate in signal changes per second
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
	// sets up raw terminal data transmission with forced baud rate
	cfmakeraw(&ts) ;
	cfsetspeed(&ts,baud) ;
	//ts.c_cflag |= CLOCAL | CREAD | CSTOPB ;
	ts.c_cflag |= CLOCAL | CREAD ;
	ts.c_cflag &= ~CSTOPB ;

	// clears any unread bytes in buffer before committing connection settings
	tcflush(ser_usb,TCIOFLUSH) ;

	int ret = tcsetattr(ser_usb,TCSANOW,&ts) ;
	if(ret<0) {
		perror("tcsetattr: ") ;
		return -1 ;
	}

	return ser_usb ;

}


// ser_read: reads a single byte from serial port and returns value
int ser_read(u_char *ch)
{
	int ret = read(ser_usb,ch,1) ;
	if(ret<0) {
		// return error if no byte to read in buffer
		if(errno==EAGAIN) {
			return 0 ;
		}
		perror("ser_read") ;
		return -1 ;
	}
	// return error if read failure
	if(ret==0) {
		perror("got 0??") ;
		return -1 ;
	}
	return ret ;
}


// ser_ln_read: reads an entire text line from the serial device into a buffer string
int ser_ln_read(char *buff)
{
	char *c = buff ;
	u_char ch ;
	int s_cou = 0 ;
	int sret = 0 ;

	// loops ser_read for each byte until encountering a termination \n character
	for(;;) {
		int ret = ser_read(&ch) ;
		if(ret<0) {
			perror("ser_ln_read") ;
			return -1 ;
		}
		// safely exits after timeout if the incoming serial stream stalls
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

// ser_write_b: write data back out to ASIC via USB interface
// does raw byte_array writing
int ser_write_b(const void *buffer, int bytes)
{
	int ret = write(ser_usb,buffer,bytes) ;

	// return error if unable to write
	if(ret != bytes) {
		perror("com_write") ;
	}

	// sleep avoids oversaturating (?) the downsteam buffer
	usleep(100) ;

	return ret ;
}

// ser_write: write data back out to ASIC via USB interface
// wrapper for ser_write_b which calculates string lengths automatically
int ser_write(const char *buffer)
{
	int bytes = strlen(buffer) ;
	return ser_write_b(buffer,bytes) ;
}

/////////////////////////////////////
// DEVICE REGISTER ACCESS FUNCTIONS//
/////////////////////////////////////

// rd: reads the contents of an on-chip register
u_int rd(u_int reg)
{
	// command
	char cmd[16] ;
	u_int r ;
	u_int val ;
	
	// format of raw string command: "r <reg number>\n"
	sprintf(cmd,"r %u\n",reg) ;
	// command is transmitted to connected to hardware
	ser_write(cmd) ;

	// listening for formatted hardware response in format:
	// "R %u = 0x%X"
	ser_ln_read(cmd) ;
	int ret = sscanf(cmd,"R %u = 0x%X",&r,&val) ;

	// if response detected in target format, hex value is parsed
	if(ret==2 && r==reg) {
		return val ;
	}

	// return error if unable to read response
	perror("rd failed") ;
	return 0 ;
}

// wr: writes a value onto on-chip register
u_int wr(u_int reg, u_int val)
{
	// command
	char cmd[32] ;
	u_int r ;

	// sends command in format below
	sprintf(cmd,"w %u,0x%X\n",reg,val) ;
	ser_write(cmd) ;

	// reads back echoed response to confirm write is successful
	ser_ln_read(cmd) ;
	int ret = sscanf(cmd,"R %u = 0x%X",&r,&val) ;

	if(ret==2 && r==reg) {
		return val ;
	}

	// return error if unable to read response
	perror("wr failed") ;
	return 0 ;
}

////////////////////////////////
// I2C PROTOCOL COMMUNICATION //
////////////////////////////////

// i2c_wr: executes i2c write command targeting specific address peripheral on the board
u_short i2c_wr(u_short reg, u_char val)
{
	// command
	char cmd[32] ;
	u_int r ;
	u_int err ;

	// command format generated and written: reg, val
	sprintf(cmd,"Ew %u,0x%X\n",reg,val) ;
	ser_write(cmd) ;

	// reads back expected format: reg, val, err
	ser_ln_read(cmd) ;
	int ret = sscanf(cmd,"EIC W 0x%X,0x%X: 0x%X",&r,&val,&err) ;

	// checks error log if it exists
	if(ret==3 && r==reg) {
		return err ;
	}

	// return if write failes
	perror("i2c_wr failed") ;
	return 0xFFFF ;

}

// i2c_rd: executes i2c read command
u_short i2c_rd(u_short reg)
{
	// command
	char cmd[32] ;
	u_int r ;
	u_int val ;

	// transmits structured request command
	// register formatted as required
	sprintf(cmd,"Er 0x%X\n",reg) ;
	ser_write(cmd) ;

	// reads returning payload signature
	// extracts register data from expected format
	ser_ln_read(cmd) ;
	int ret = sscanf(cmd,"EIC R 0x%X: 0x%X",&r,&val) ;
	
	// returns register val if found
	if(ret==2 && r==reg) {
		return val ;
	}

	// return if failed
	perror("i2c_rd failed") ;
	return 0xFFFF ;
	
}

// counter for registers
static int reg_cou ;
// struct for registers and values
static struct reg_t {
	u_short reg ;
	u_char val ;
} regs[1034] ;
// initialize internal stacking array, regs

/////////////////////////
// CONFIG FILE PARSING //
/////////////////////////

// open_py: opens and parses python script containing target ASIC config parameters
int open_py(const char *fname)
{
	// opens file, returns error if not found
	FILE *f = fopen(fname,"r") ;

	if(f==0) {
		perror(fname) ;
		return -1 ;
	}

	// scans script line by line looking for direct ASIC write functions
	while(!feof(f)) {
		char buff[256] ;
		char b[256] ;
		u_int reg ;

		if(fgets(buff,sizeof(buff),f)==0) continue ;

		// expected format for ASIC write functions
		int ret = sscanf(buff,"eic_clib.write_asic_indirect_reg(0x%X, %s",&reg,b) ;
		if(ret!=2) continue ;

		u_int val = 0 ;
		int bit = 7 ;

		// parses binary values and converts them into bytes via bitshifting
		// caches them into internal stacking array, regs
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

	// when end of file is reached, stop processing and close
	fclose(f) ;

//	printf("Read %d items\n",reg_cou) ;
//	for(int i=0;i<reg_cou;i++) {
//		printf("I2C %d: reg 0x%04X = 0x%02X\n",i,regs[i].reg,regs[i].val) ;
//	}
	// return register counter
	return reg_cou ;

}

////////////////////////////////////
// TERMINAL INTERACTION UTILITIES //
////////////////////////////////////

// exit_recover: cleanup exit routine
void exit_recover()
{
	// ANSI color reset sequence to fix terminal output color
	fprintf(stderr,"%s\n",ANSI_RESET) ;
	// restore normal in/out echo behavior in case of unexpected exit
	system("/bin/stty sane") ;
}

// khbit: non-blocking keyboard listener
int kbhit()
{
	static int first ;
	u_char a ;

	if(first==0) {	// set stdin to non-blocking once
		fcntl(0,F_SETFL,fcntl(0,F_GETFL)|O_NONBLOCK) ;
		first = 1 ;
	}

	// when called, checks if key has been tapped on the keyboard
	// if yes, return character code; if not returns -1
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


#if 0

static const char *ser_cmd[] = {
	"b",
	"r",

	"2b 0",		// scan bus 0 (2x Si PLLs)
	"2",
	"2b 1",		// scan bus 1 (SFP)
	"2",
	"2b 2",		// scan bus 2 (ETL)
	"2",

	"2b 0",
	"2d 112",	// 5338
	"2r 0",
	"2r 1",
	"2r 2",
	"2r 3",
	"2r 4",
	"2d 104",	// 5345
	"2r 0",
	"2r 1",
	"2r 2",
	"2r 3",
	"2r 4",

	"2b 1",
	"2d 80",
	"2r 0",
	"2d 22",	// provoke error
	"2r 0",
	""
} ;

static u_short adc[36] ;

static int run_exe(int mode)
{
	int cou = 0 ;
	int ret ;

	switch(mode) {
	case 2 :
		for(int i=0;i<10000;i++) {
			char rcmd[128] ;
			const char *cmd = "r 11\n" ;
			ser_write_b(cmd,strlen(cmd)) ;

			ret = ser_ln_read(rcmd) ;
			if(ret) {
				int dummy ;
				unsigned int val ;

				sscanf(rcmd,"Reg %d = 0x%X",&dummy,&val) ;

				int ch = (val >> 16) & 0x3F ;
				
				if(val&0xFFFF) {
					adc[ch] = val & 0xFFFF ;
					printf("%d: %s: 0x%08X: ch %2d, ADC %d\n",i,rcmd,val,ch,val&0xFFFF) ;
				}
			}
		}

		for(int i=0;i<36;i++) {
			printf("Ch %2d = %u\n",i,adc[i]) ;
		}
		return 0 ;
	}

	while(ser_cmd[cou][0]) {
		char cmd[256] ;
		sprintf(cmd,"%s\n",ser_cmd[cou]) ;
		ser_write_b(cmd,strlen(cmd)) ;

		printf("[%2d] >>> %s",cou,cmd) ;

		for(;;) {
			ret = ser_ln_read(cmd) ;

			if(ret) printf("%s%s%s\n",ANSI_BLUE,cmd,ANSI_RESET) ;
			else break ;
		}

		cou++ ;
	}

	return cou ;
}

int run_test(int mode)
{
	int reg = 20  ;
	u_int val = rd(reg) ;
	printf("Reg %d = 0x%08X[%u]\n",reg,val,val) ;

	val = wr(0,0xab) ;
	printf("Reg %d = 0x%X, now 0x%X\n",0,0xab,val) ;

	reg = 0 ;
	val = rd(reg) ;
	printf("Reg %d = 0x%08X[%u]\n",reg,val,val) ;


	return 0 ;
}

void gtp_print()
{
	u_int st = rd(16+2) ;
	int err ;

	if((st&0x61FF)!=0x61DC) err = 1;
	else err = 0 ;

	printf("Control 0x%08X [data sent 0x%08X], status%s 0x%08X [data received 0x%08X]; RX %u, TX %u\n",
	       rd(2),rd(3),err?"*":"",
	       st,rd(16+3),
	       rd(16+5),rd(16+6)) ;


}

void gtp_reset()
{
	u_int control = rd(2) ;

	printf("Before GTP Reset: control 0x%08X, status 0x%08X\n",control,rd(16+2)) ;

#if 1
	control |= (1<<9) ;	// reset ALL
	wr(2,control) ;

	usleep(10000) ;
	
	printf("... ALL 0x%08X (0x%X)\n",rd(16+2),rd(2)) ;
	
	control &= ~(1<<9) ;
	wr(2,control) ;

	for(int i=0;i<10000;i++) {
		u_int st = rd(16+2) & 0x61FF ;
		if(st==0x61DC) {
			printf("Rst ALL after %d: 0x%04X\n",i,st) ;
			break ;
		}
	}
#endif

#if 0
	// Reset RX & TX clocks
	control |= ((1<<4) | (1<<5)) ;
	wr(2,control) ;

	usleep(1000) ;

	control &= ~((1<<4)|(1<<5)) ;
	wr(2,control) ;

	usleep(1000) ;
#endif

#if 0	// reset TX
	control |= ((1<<10)) ;
	wr(2,control) ;

	usleep(1000) ;

	printf("...TX 0x%08X\n",rd(16+2)) ;

	control &= ~((1<<10)) ;
	wr(2,control) ;


	for(int i=0;i<100000;i++) {
		u_int st = rd(16+2) ;
		if(st & (1<<3)) {
			printf("Rst TX after %d: 0x%08X\n",i,st) ;
			break ;
		}
	}


	usleep(1000) ;
#endif 

#if 0
	// reset RX
	control |= ((1<<13)) ;
	wr(2,control) ;

	usleep(1000) ;

	printf("...RX 0x%08X\n",rd(16+2)) ;

	control &= ~((1<<13)) ;
	wr(2,control) ;

	for(int i=0;i<100000;i++) {
		u_int st = rd(16+2) ;
		if(st & (1<<4)) {
			printf("Rst RX after %d: 0x%08X\n",i,st) ;
			break ;
		}
	}

	usleep(1000) ;

	
#endif


#if 0
	// Reset RX & TX clocks: RX is 5, TX is 4
	control |= ((1<<4) | (1<<5)) ;
	wr(2,control) ;

	usleep(1000) ;

	control &= ~((1<<4)|(1<<5)) ;
	wr(2,control) ;

	usleep(1000) ;
#endif


}

#if 0	
	
void gtp_run()
{
	u_int freq_rx, freq_tx ;
	u_int st ;

	// prepare data
	wr(3,0x1234563C) ;	// data


//	wr(2,0x10000001) ;	// LSB comma+loopback=1 -- WORKS
//	wr(2,0x10000002) ;	// LSB comma+loopback=2 -- WORKS  

	// seems that all modes from here don't work, hmmmmm..... Perhaps the farend stuff won't work...
//	wr(2,0x10000004) ;	// LSB comma+loopback=4 -- NO 
//	wr(2,0x10000006) ;	// LSB comma+loopback=6 -- NO 
	wr(2,0x10000000) ;	// LSB comma+loopback=0 (normal) -- NO

	gtp_print() ;
	gtp_reset() ;
	gtp_print() ;
	usleep(1000) ;
	gtp_print() ;
	usleep(100000) ;
	gtp_print() ;
	usleep(100000) ;
	gtp_print() ;
}

#endif

#endif

///////////////////////
// MAIN PROGRAM LOOP //
///////////////////////

// user command arguments:
// -m flag: operation mode
//// -m0 = ASIC initialization
//// -m1 and -m2: DAQ loops, sends test pulse to clear on-board FIFOs,
//// then streams until terminator marker (0x8FFFFFFF) reached or timeout
// -n flag: number of target event loops
// default mode: converts user terminal text into direct serial link interface
// blue text represents data returned by hardware
int main(int argc, char *argv[])
{
	int ret ;
	int c ;
	int mode = -1 ;
	int sel_fcmd = 1 ;	// default is lpGBT mode
	int num_events = 1 ;
	time_t now ;

	while((c=getopt(argc,argv,"m:d:En:")) != EOF) {
	switch(c) {
	case 'm' :	// execute batch command with argument...
		mode = atoi(optarg) ;
		break ;
	case 'E' :
		sel_fcmd = 0 ;
		break ;
	case 'd' :	// change USB device
		usb_dev = optarg ;
		break ;
	case 'n' :
		num_events = atoi(optarg) ;
		break ;
	}
	}

	// Enter interactive terminal mode...
	printf("%s**** %s Version %s %s: %s\n",ANSI_RESET,argv[0],__DATE__,__TIME__,usb_dev) ;



	if(ser_open()<0) exit(-1) ;


	// grab them from the canonical location
	open_py("/home/epic/tonko/registers_values.py") ;

	
	switch(mode) {
	int column, row ;
	u_int addr ;
	case 0 :
		// reset
		wr(2,0) ;	// reset last run

		wr(0,0) ;	// reset
		wr(0,(sel_fcmd<<2)) ;	// set SEL_FCMD=1 is (1<<2)
		usleep(1000) ;		// wait a bit...
		wr(0,(sel_fcmd<<2)|3) ;	// enable ASIC
		usleep(10000) ;		// wait a bit more

		// load default register values
		for(int i=0;i<reg_cou;i++) {
			u_int reg = regs[i].reg ;

			//if(reg<0x4000) continue ;

			ret = i2c_wr(reg,regs[i].val) ;

			printf("I2C %d: write 0x%04X = 0x%02X\n",i,regs[i].reg,regs[i].val) ;
		}

		// load specific register values
		//i2c_wr(0x4000,0x63) ;
		//i2c_wr(0x4012,0x78) ;

		i2c_wr(0x4013,0xF) ;

//		i2c_wr(0x4013,0xFF) ;	// enable lanes 0..7
//		i2c_wr(0x4014,0xF0) ;
//		i2c_wr(0x4015,0x00) ;	
//		i2c_wr(0x4016,0x00) ;	


		//per pixel writes scheme for bad EICROC1
		// FIRST
		//	Write the registers for the pixel you want!
		//	But using addresses 0x1..0x5 (note NO higher order bits)
		// SECOND
		//	Use the pixel's address e.g. 0x20F9... and write the
		//	values for ALL the other pixels!

		// FIRST: values I want for a particular pixel
		i2c_wr(0x0001,0x80) ;
		i2c_wr(0x0002,0x6C) ;	// for pix
		i2c_wr(0x0003,0x04) ;
		i2c_wr(0x0004,0x01) ;
		i2c_wr(0x0005,0x20) ;

		column = 0 ;		// 0..31; but only use 0..3 for my tests
		row = 31 ;		// 0..31

		// SECOND
		// use the correct pixel but set the values for all
		addr = 0x2000 | (column<<16) | (row<<3) ;

		i2c_wr(addr|1,0x80) ;
		i2c_wr(addr|2,0x00) ;
		i2c_wr(addr|3,0x00) ;
		i2c_wr(addr|4,0x01) ;
		i2c_wr(addr|5,0x20) ;


		// read those values back...
		for(int i=0;i<=0x1B;i++) {
			int reg = 0x4000+i ;

			ret = i2c_rd(reg) ;
			printf("I2C read 0x%04X = 0x%02X\n",reg,ret) ;
		}


		// set the length of the EN_ACQ pulse as well as the delay to CMDPULSE
		//wr(3,12<<8|4) ;	// lo 8bits: len of EN_ACK _after_ CMDPULSE, hi 8bits: delay from EN_ACQ to CMDPULSE


		{
		int cmd_mode = 4 ;		//4: DON'T issue CMDPULSE, 0: issue CMDPULSE
		int en_ack_to_cmd = 12 ;	// any length longer than at least 8
		int cmd_to_end_ack = 4 ;	// keep at 4 normally

		wr(3,(en_ack_to_cmd<<8) | (cmd_to_end_ack)) ;
		wr(2,cmd_mode<<1) ;
		}

		return 0 ;

	case 1 :
		for(int i=0;i<num_events;i++) {
			int cmd_mode = 4 ;	// bit 2: disable CMD

			printf("Fire %d/%d, mode %d...\n",i,num_events,cmd_mode) ;

			wr(1,1) ;			// clear FIFO with a pulse
			wr(1,0) ;


			wr(2,(cmd_mode)<<1|1) ;			// fire 1 readout; but always show the old trace
			usleep(10) ;			
			wr(2,(cmd_mode<<1)) ;			// clear fire

			int w_cou = 0 ;
			for(;;) {
				u_int val ;

				val = rd(6) ;
				printf("Evt %d: %d = 0x%08X\n",i,w_cou,val) ;
				w_cou++ ;

				if(val==0x8FFFFFFF) break ;
				if(w_cou==10000) {
					fprintf(stderr,"Timeout in event %d\n",i) ;
					break ;
				}
			}

			
//			sleep(20) ;		// wait between events
		}

		printf("Done with %d events.\n",num_events) ;
		
		//wr(0,0) ; Leaving EICROC1 in RESET.\n") ;
		
		return 0 ;
	case 2 :
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
			if(strcmp(buff,"8FFFFFFF")==0) break ;
		}
		fflush(stdout) ;
		fprintf(stderr,"Done evt %d, %d words after %d secs\n",e,w_cou,time(0)-now) ;
		}
		return 0 ;
		break ;
	case 3 :
		{
		int tot_bytes = 0 ;
		ser_write("T\n") ;
		now = time(0) ;
		for(;;) {
			u_int buff[1024] ;
			int ret = read(ser_usb,buff,sizeof(buff)) ;
			if(ret < 0) {
				if(errno==EAGAIN) continue ;
				else break ;
			}
			
			tot_bytes += ret ;

//			for(int i=0;i<ret/4;i++) {
//				printf("%d 0x%08X\n",i,buff[i]) ;
//			}

//			printf("Read %d, tot %d, time %u\n",ret,tot_bytes,time(0)-now) ;
			if(ret == 0) break ;
			if(tot_bytes>=26112) break ;
		}
		printf("Read tot %d, time %u\n",tot_bytes,time(0)-now) ;
		}
		return 0 ;

	default :
		break ;
	}


	printf("Entering interfactive mode:\n") ;

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
