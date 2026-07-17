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

#include <LOG/rtsLog.h>


#include "zcu106_c.h" 

int zcu106_c::get_event(u_int *buff)
{

	return -1 ;
}


int zcu106_c::open_py(const char *fname)
{
	regs_cou = 0 ;

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

		//printf("Reg %d: 0x%04X = [%s] -- 0x%02X\n",regs_cou,reg,b,val) ;

		regs[regs_cou].reg = reg ;
		regs[regs_cou].val = val ;
		regs_cou++ ;
	}

	fclose(f) ;

//	printf("Read %d items\n",reg_cou) ;
//	for(int i=0;i<reg_cou;i++) {
//		printf("I2C %d: reg 0x%04X = 0x%02X\n",i,regs[i].reg,regs[i].val) ;
//	}

	return regs_cou ;

}

int zcu106_c::open_eicroc0_regs(const char *fname)
{
	FILE *f = fopen(fname,"r") ;

	if(f==0) {
		LOG(ERR,"%s: %s",fname,strerror(errno)) ;
		return -1 ;
	}

	regs_cou = 0 ;

	while(!feof(f)) {
		char buff[256] ;
		int reg, val ;

		if(fgets(buff,sizeof(buff),f)==0) continue ;

		int ret = sscanf(buff,"REG 0x%X = 0x%X",&reg,&val) ;

		if(ret!=2) continue ;

		regs[regs_cou].reg = reg ;
		regs[regs_cou].val = val ;
		regs_cou++ ;
	}

	return regs_cou ;
}

int zcu106_c::set_default_regs(const char *fname)
{
	switch(asic_flavor) {
	case 0 :
		return open_eicroc0_regs(fname) ;
		break ;
	case 1 :
		return open_py(fname) ;
		break ;
	default :
		return -1 ;
	}

	return 0 ;
}

zcu106_c::zcu106_c()
{
	ser_usb = -1 ;
	regs_cou = 0 ;
}

zcu106_c::~zcu106_c()
{
	if(ser_usb>=0) close(ser_usb) ;
}

int zcu106_c::set_flavor(int flavor)
{
	//wr to regx

	asic_flavor = flavor ;

	return 0 ;
}

int zcu106_c::set_trg_type(int arg)
{
	//wr to regx

	trg_type = arg ;

	return 0 ;
}

int zcu106_c::use_i2c_addr(u_char arg)
{
	//wr to regx



	return 0 ;
}


int zcu106_c::dev_open(const char *usb_dev)
{
//	int baud = 230400 ;
//	int baud = 19200 ;

	int baud = 115200 ;
	int block = 0 ;	// never block in this application...

	if(block==0) {
		ser_usb = open(usb_dev,O_RDWR|O_NOCTTY|O_NONBLOCK|O_NDELAY,0666) ;
	}
	else {
		ser_usb = open(usb_dev,O_RDWR|O_NOCTTY,0666) ;
	}

	if(ser_usb<0) {
//		perror(usb_dev) ;
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
//		perror("tcsetattr: ") ;
		return -1 ;
	}

	return ser_usb ;
}

int zcu106_c::ser_read(u_char *ch)
{
	int ret = read(ser_usb,ch,1) ;
	if(ret<0) {
		if(errno==EAGAIN) {
			return 0 ;
		}
		LOG(ERR,"ser_read: %s",strerror(errno)) ;
		return -1 ;
	}
	if(ret==0) {
		LOG(ERR,"ser_read: got 0??") ;
		return -1 ;
	}
	return ret ;
}

int zcu106_c::ser_ln_read(char *buff)
{
	char *c = buff ;
	u_char ch ;
	int s_cou = 0 ;
	int sret = 0 ;

	for(;;) {
		int ret = ser_read(&ch) ;
		if(ret<0) {
			LOG(ERR,"ser_ln_read: %s",strerror(errno)) ;
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


int zcu106_c::ser_write_b(const void *buffer, int bytes)
{
	int ret = write(ser_usb,buffer,bytes) ;

	if(ret != bytes) {
		LOG(ERR,"ser_write: %s",strerror(errno)) ;
	}

	usleep(100) ;	// not sure I need this!?

	return ret ;
}

int zcu106_c::ser_write(const char *buffer)
{
	int bytes = strlen(buffer) ;
	return ser_write_b(buffer,bytes) ;
}

u_int zcu106_c::rd(int reg)
{
	char cmd[16] ;
	u_int r ;
	u_int val ;
 
	sprintf(cmd,"r %u\n",reg) ;
	ser_write(cmd) ;

	ser_ln_read(cmd) ;
	int ret = sscanf(cmd,"R %u = 0x%X",&r,&val) ;

	if(ret==2 && r==(u_int)reg) {
		return val ;
	}

	LOG(ERR,"rd(%d): %s",reg,strerror(errno)) ;
	return -1 ;
}

u_int zcu106_c::wr(int reg, u_int val)
{
	char cmd[32] ;
	u_int r ;

	sprintf(cmd,"w %u,0x%X\n",reg,val) ;
	ser_write(cmd) ;

	ser_ln_read(cmd) ;
	int ret = sscanf(cmd,"R %u = 0x%X",&r,&val) ;

//printf("wr: ret %d: 0x%X,%d: [%s]\n",ret,r,val,cmd) ;

	if(ret==2 && r==(u_int)reg) {
		return val ;
	}

	LOG(ERR,"wr(%d,0x%X): %s",reg,val,strerror(errno)) ;
	return 0xFFFFFFFF ;
}


u_short zcu106_c::i2c_wr(u_short reg, u_char val)
{

	char cmd[32] ;
	u_int r ;
	u_int err ;

	sprintf(cmd,"Ew %u,0x%X\n",reg,val) ;
	ser_write(cmd) ;

	ser_ln_read(cmd) ;
	int ret = sscanf(cmd,"EIC W 0x%X,0x%X: 0x%X",&r,&val,&err) ;

	if(ret==3 && r==reg) {
		return err ;
	}

	LOG(ERR,"i2c_wr(0x%X,0x%X)",reg,val) ;
	return 0xFFFF ;

}

u_short zcu106_c::i2c_rd(u_short reg)
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

	LOG(ERR,"i2c_rd(0x%X)",reg) ;
	return 0xFFFF ;	
}

