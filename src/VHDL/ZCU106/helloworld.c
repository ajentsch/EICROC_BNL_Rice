#include <stdio.h>
#include <stdlib.h>

#include "platform.h"
#include <xil_printf.h>
#include "xuartlite_l.h"


extern char inbyte() ;


static char *all_version_c="0xAAAAAAAA" ;
static char *bitfile_version_c="0xBBBBBBBB" ;
static char    *proj_version_c="0xCCCCCCCC" ;
u_int all_version = 0 ;
u_int bitfile_version = 0 ;


static volatile u_int *const gpio_wr = (volatile u_int *) 0x40000008 ;
static volatile u_int *const gpio_rd = (volatile u_int *) 0x40000000 ;


static u_int dta[2048] ;

void wait(int loops)
{
	for(int i=0;i<(loops*10);i++) ;
}


u_int cpu_read(u_int reg)
{
	reg &= 0xFF ;
	reg <<= 16 ;
	reg |= 0x80000000 ;

	*gpio_wr = reg ;

	reg = *gpio_rd ;

	*gpio_wr = 0 ;

	return reg ;
}

void cpu_write(u_int reg, u_short dta)
{
	reg &= 0xFF ;	// only 8 bit address
	reg <<= 16 ;	// ...in the higher 20 bits
	reg |= 0xC0000000 | dta;	// pulse AD cycle

	*gpio_wr = reg ;	// write
	*gpio_wr = 0 ;		// go to dormant
	
	return ;

}

void cpu_set_bit(u_int reg, int bit)
{
	u_int tmp = cpu_read(reg) ;
	tmp |= (1<<bit) ;
	cpu_write(reg,tmp) ;
}

void cpu_clr_bit(u_int reg, int bit)
{
	u_int tmp = cpu_read(reg) ;
	tmp &= ~(1<<bit) ;
	cpu_write(reg,tmp) ;
}

u_char cpu_get_bit(u_int reg, int bit)
{
	u_int tmp = cpu_read(reg) ;
	if(tmp & (1<<bit)) return 1 ;
	return 0 ;
}

void cpu_pulse_bit(u_int reg, int bit) 
{
	cpu_set_bit(reg,bit) ;
	cpu_clr_bit(reg,bit) ;
}


/****************** I2C stuff ************************/

u_char i2c_bus = 0 ;
u_char i2c_device = 0x40 ;	// defaults to EICROC1 Testboard
u_char i2c_scan_r = 0 ;

void i2c_scl(int on)
{
	if(on) {
		cpu_set_bit(0,8) ;
	}
	else {
		cpu_clr_bit(0,8) ;
	}
}

void i2c_sda(int on)
{

	cpu_set_bit(0,10) ;	// drive SDA

	if(on) {
		cpu_set_bit(0,9) ;
	}
	else {
		cpu_clr_bit(0,9) ;
	}


}

u_char i2c_sdz()
{
	//SDA to 1
//	cpu_set_bit(0,9) ;

	cpu_clr_bit(0,10) ;	// SDA to Z

	u_int val = cpu_get_bit(0,9+16) ;	// read SDA

	if(val) {
		return 1 ;
	}

	return 0 ;
}


void i2c_start()
{
	i2c_scl(1) ;
	i2c_sda(1) ;

	i2c_sda(0) ;
	i2c_scl(0) ;
}

u_char i2c_stop()
{
	i2c_sda(0) ;
	i2c_scl(1) ;
	i2c_sda(1) ;

	u_char a = i2c_sdz() ;

//	xil_printf("2S %d\n",a) ;

	return a ;
}

u_char i2c_write(u_char val)
{
	int i ;
	u_char ret0, ret1 ;

	for(i=7;i>=0;i--) {
		int bit = (val&(1<<i))?1:0 ;
		i2c_sda(bit) ;

		i2c_scl(1) ;
		i2c_scl(0) ;
	}

	i2c_sda(1) ;
	ret0 = i2c_sdz() ;


	i2c_scl(1) ;
	ret1 = i2c_sdz() ;

	i2c_scl(0) ;

//	xil_printf("2W 0x%02X: %d %d\n",val,ret0,ret1) ;

	return ret1 ;
}

void i2c_read_m(int cou, u_short *val) 
{
	int i, w ;

	for(w=0;w<cou;w++) {
		val[w] = 0 ;

		i2c_sda(1) ;
		i2c_sdz() ;

		for(i=7;i>=0;i--) {
			i2c_scl(1) ;
			int bit = i2c_sdz() ;
			i2c_scl(0) ;

			val[w] |= (bit<<i) ;
		}

		if(w==(cou-1)) {
			// nack for last read
			i2c_sda(1) ;
			i2c_scl(1) ;
			i2c_scl(0) ;
		}
		else {
			i2c_sda(0) ;
			i2c_scl(1) ;
			i2c_scl(0) ;
		}
	}
}



u_short i2c_reg_wr(u_char reg, u_char val)
{
	u_char ack = 0 ;

	i2c_start() ;
	ack |= i2c_write((i2c_device<<1)|0) ;
	ack |= i2c_write(reg) ;
	ack |= i2c_write(val) ;

	if(ack) return (0x100|ack) ;
	return 0 ;
}

u_short i2c_reg16_wr(u_short reg, u_char val)
{
	u_char ack = 0 ;

	i2c_start() ;
	ack |= i2c_write((i2c_device<<1)|0) ;
	ack |= i2c_write(reg&0xFF) ;
	ack |= i2c_write(reg>>8) ;

	ack |= i2c_write(val) ;

	if(ack) return (0x100|ack) ;
	return 0 ;
}

u_short i2c_reg16_rd4(u_short reg)
{
	u_char ack = 0 ;
	u_short val[3] = { 0,0,0} ;
	u_short err = 0 ;

	// set the register we want to read
	i2c_start() ;
	ack |= i2c_write(i2c_device<<1|0) ;
	ack |= i2c_write(reg&0xFF) ;
	ack |= i2c_write(reg>>8) ;
	i2c_stop() ;

	if(ack) err |= 0x100 ;

	// read the value
	wait(100) ;

	i2c_start() ;
	ack |= i2c_write((i2c_device<<1)|1) ;

	wait(100) ;

	i2c_read_m(3,val) ;
	i2c_stop() ;


	if(ack) err |= 0x200 ;

	xil_printf("E %X: %X %X %X\n",err,val[0],val[1],val[2]) ;

	return val[0] | err ;
}

u_short i2c_reg16_rd(u_short reg)
{
	u_char ack = 0 ;
	u_short val = 0 ;
	u_short err = 0 ;

	// set the register we want to read
	i2c_start() ;
	ack |= i2c_write(i2c_device<<1|0) ;
	ack |= i2c_write(reg&0xFF) ;		//lo for EICROC
	ack |= i2c_write(reg>>8) ;		//hi for EICROC

//	i2c_stop() ;				// no stop for EICROC

	if(ack) err |= 0x100 ;

	// read the value
	i2c_start() ;
	ack |= i2c_write((i2c_device<<1)|1) ;
	i2c_read_m(1,&val) ;
	i2c_stop() ;


	if(ack) err |= 0x200 ;

	return val | err ;
}
	
u_short i2c_reg_rd(u_char reg) 
{
	u_char ack = 0 ;
	u_short val = 0xFF ;

	// set the register we want to read
	i2c_start() ;
	ack |= i2c_write(i2c_device<<1|0) ;
	ack |= i2c_write(reg) ;
	i2c_stop() ;

	if(ack) {
		val |= 0x300 ;
		return val ;
	}

	// read the value
	i2c_start() ;
	ack |= i2c_write((i2c_device<<1)|1) ;
	i2c_read_m(1,&val) ;
	i2c_stop() ;

	if(ack) val |= 0x400 ;
	return val ;
}

u_short eicr_rd(u_short reg)
{
	u_char ack ;
	u_short val = 0xF000 ;
	u_short retval = 0 ;

	i2c_start() ;
	ack = i2c_write(i2c_device<<1|0) ;
	if(ack) retval |= 0x100 ;
	ack = i2c_write(reg&0xFF) ;
	if(ack) retval |= 0x200 ;
	i2c_stop() ;

	i2c_start() ;
	ack = i2c_write((i2c_device+1)<<1|0) ;
	if(ack) retval |= 0x400 ;
	ack = i2c_write(reg>>8) ;
	if(ack) retval |= 0x800 ;
	i2c_stop() ;

	i2c_start() ;
	ack = i2c_write((i2c_device+2)<<1|1) ;
	i2c_read_m(1,&val) ;
	i2c_stop() ;

	return (retval|val) ;
	
	
}

u_short eicr_wr(u_short reg, u_char val)
{
	u_char ack ;
	u_short retval = 0 ;

	// LO
	i2c_start() ;
	ack = i2c_write(i2c_device<<1|0) ;
	if(ack) retval |= 0x1 ;
	ack = i2c_write(reg&0xFF) ;
	if(ack) retval |= 0x2 ;
	i2c_stop() ;

	// HI
	i2c_start() ;
	ack = i2c_write((i2c_device+1)<<1|0) ;
	if(ack) retval |= 0x4 ;
	ack = i2c_write(reg>>8) ;
	if(ack) retval |= 0x8 ;
	i2c_stop() ;

	// VAL
	i2c_start() ;
	ack = i2c_write((i2c_device+2)<<1|0) ;
	if(ack) retval |= 0x10 ;
	ack = i2c_write(val) ;
	if(ack) retval |= 0x20 ;
	i2c_stop() ;

	return retval ;	// error if non-0
}






/*************** end of I2C *******************/

#include <xparameters.h>


#define USB_ERROR 0xE0 
#define USB_BASE 0x40600000


static volatile int * const usb_rx = (int *)(USB_BASE+0) ;
static volatile int * const usb_tx = (int *)(USB_BASE+4) ;
static volatile int * const usb_stat = (int *)(USB_BASE+8) ;

int usb_write(void *vbuff, int bytes)
{
	int i ;
	int written = 0 ;
	int err = 0 ;
	char *buffer = (char *)vbuff ;

	for(i=0;i<100000;i++) {
//		err = *usb_stat & USB_ERROR ;
//		if(err) {	//error
//			break ;
//		}

		if(*usb_stat & 0x8) {	//FULL ;
			continue ;
		}

		*usb_tx = *buffer++ ;

		written++ ;
		if(bytes == written) break ;
	}

	if(err || (i>90000)) {
		//no point in writing anything...
		return -1 ;
	}
	
	return written ;
}


int usb_read(char *buffer, int bytes, int blocking)
{
	int i ;
	int err = 0 ;
	int b_read = 0 ;

	for(i=0;i<1000000;i++) {
		err = *usb_stat & USB_ERROR ;
		if(err) {	//error
			break ;
		}

		if(*usb_stat & 0x1) {	//FIFO has data
		}
		else {	//empty
			if(!blocking) return b_read ;
			continue ;
		}

		buffer[b_read] = *usb_rx ;
		b_read++ ;
		if(bytes == b_read) break ;
	}

	if(err || (i>900000)) {
		xil_printf("ERROR: read 0x%X, %d %d\n",err,i,b_read) ;
		return -1 ;
	}
	
	return b_read ;
	

}


int usb_string(const char *str)
{
	int bytes = strlen(str) ;

	return usb_write((void *)str,bytes) ;
}

int usb_empty()
{
	if(*usb_stat & 1) return 0 ;

	return 1 ;
}



static char *get_cmd()
{
	static char str[64] ;

	if(XUartLite_IsReceiveEmpty(STDIN_BASEADDRESS)) return 0 ;

	char *p_str = str ;

	for(;;) {
		char c = (char) inbyte() ;

		//*p_str++ = c ;

		// don't save the newline
		if(c==0xA || c==0xD || ((p_str-str)>60)) {
			*p_str++ = 0 ;
			return str ;
		}
		else {
			*p_str++ = c ;
		}
	}
}

static u_int char_to_hex(char *d)
{
	int i ;
	int s = strlen(d) ;
	u_int dta = 0 ;

	int rot = 0 ;
	for(i=s;i>=0;i--) {
		if((d[i]>='0')&&(d[i]<='9')) {
			dta |= (d[i]-'0')<<(rot*4) ;
			rot++ ;
		}
		else if((d[i]>='a')&&(d[i]<='f')) {
			dta |= (d[i]-'a'+10)<<(rot*4) ;
			rot++ ;
		}
		else if((d[i]>='A')&&(d[i]<='F')) {
			dta |= (d[i]-'A'+10)<<(rot*4) ;
			rot++ ;
		}
	}

	if(rot==0) return 0xFFFFFFFF ;

	return dta ;
}

u_int ctoi(char *d)
{
	if(d[0]=='0' && (d[1]=='x' || d[1]=='X')) return char_to_hex(d) ;
	return atoi(d) ;
}


int parser(char *c, char *ptr[], u_int args[2]) 
{
        int arg_cou = 0 ;
	int len = strlen(c) ;
	int found = 0 ;

	args[0] = args[1] = 0 ;

	for(int i=0;i<len;i++) {
		if(c[i]==' ') {
			found = 0 ;
			c[i] = 0 ;
			continue ;
		}

                if(c[i]==',') {
			found = 0 ;
			c[i] = 0 ;
			continue ;
		}

                if(c[i]=='\n') {
			found = 0 ;
			c[i] = 0 ;
                        continue ;
		}

                if(c[i]=='\r') {
			found = 0 ;
			c[i] = 0 ;
                        continue ;
		}

                if(!found && arg_cou<3) {
			ptr[arg_cou] = c+i ;
			arg_cou++ ;
			found = 1 ;
		}
	}

        for(int i=1;i<arg_cou;i++) {
			args[i-1] = ctoi(ptr[i]) ;
	}

        return arg_cou ;

}

void hwicap_version(u_int v)
{
	int s = v & 0x3F ;
	int m = (v>>6)&0x3F ;
	int h = (v>>12)&0x1F ;
	int y = ((v>>17)&0x3F) ;
	int mo = (v>>23)&0xF ;
	int d = (v>>27)&0x1F ;

	xil_printf("%02d-%02d-%02d %02d:%02d:%02d",
			mo,d,y,h,m,s) ;
}

static void boot_print()
{


	xil_printf("%s: SW %s %s\n",proj_version_c,__DATE__,__TIME__) ;

	xil_printf("%s: V-all 0x%08X [",proj_version_c,all_version) ;
	hwicap_version(all_version) ;
	xil_printf("]\n") ;

	xil_printf("%s: V-bit 0x%08X [",proj_version_c,bitfile_version) ;
	hwicap_version(bitfile_version) ;
	xil_printf("]\n") ;


}

int main()
{
	init_platform();

	xil_printf("%s: powerup\n",proj_version_c) ;


	all_version = char_to_hex(all_version_c) ;
	bitfile_version = char_to_hex(bitfile_version_c) ;

	
	boot_print() ;

	for(;;) {
		u_int arg[2] ;
		char *ptr[3] ;
		int ptr_cou ;
		char *in ;
		u_int ret ;
		u_int val ;

		in = get_cmd() ;

		if(in==0) continue ;

		ptr_cou = parser(in,ptr,arg) ;

		ret = 0 ;

		switch(in[0]) {
		case 'b' :
			boot_print() ;
			break ;
		case 'p' :
			xil_printf("Args:cou %d: 0x%X 0x%X\n",ptr_cou,arg[0],arg[1]) ;
			break ;
		case 'r' :
			if(ptr_cou==1) {
				for(int i=0;i<16;i++) {
					xil_printf("R %d = 0x%X\n",i,cpu_read(i)) ;
				}
			}
			else {
					xil_printf("R %d = 0x%X\n",arg[0],cpu_read(arg[0])) ;
			}
			break ;
		case 'T' :
			for(int i=0;i<204;i++) {	// 204 shots of 32 ints
				for(int j=0;j<32;j++) {
					dta[j] = i*32+j ;
				}
				usb_write(dta,128) ;
			}
			break ;
		case 'w' :
			if(in[1]=='s') {	// set bit
				cpu_set_bit(arg[0],arg[1]) ;
			}
			else if(in[1]=='c') {
				cpu_clr_bit(arg[0],arg[1]) ;
			}
			else if(in[1]=='p') {
				cpu_pulse_bit(arg[0],arg[1]) ;
			}
			else {
				cpu_write(arg[0],arg[1]) ;
			}
			
			xil_printf("R %d = 0x%X\n",arg[0],cpu_read(arg[0])) ;
				 
			break ;
		case 'R' :
			val = cpu_read(2) & 0xFFFE ;	// remove bit 0

			for(int i=0;i<arg[0];i++) {
				cpu_write(1,1) ;	// reset FIFO
				cpu_write(1,0) ;

				cpu_write(2,val|1) ;	// fire
				
				// wait for FIFO not empty
				for(;;) {
					if((cpu_read(1)>>16)&2) continue ;	// FIFO empty...
					break ;				// FIFO not-empty
				}

				int w_cou = 0 ;
				for(;;) {
					u_int val ;
					
					
					val = cpu_read(6) ;

					//dta[w_cou] = val ;

					//if(w_cou<10) {
						xil_printf("%X\n",val) ;
					//}

					w_cou++ ;
					if(val==0x8FFFFFFF) {
						xil_printf("End event %d after %d\n",i,w_cou) ;
						break ;
					}
					if(w_cou==10000) {	// I expect 6520 strobes...
						xil_printf("Timeout event %d\n",i) ;
						break ;
					}

				}
				cpu_write(2,val) ;	// clear fire

				//xil_printf("dta 0x%08X 0x%08X 0x%08X\n",dta[0],dta[1],dta[w_cou-1]) ;

			}
			break ;
		case 'E' :
			if(in[1]=='w') {
				ret = eicr_wr(arg[0],arg[1]) ;
				xil_printf("EIC W 0x%04X,0x%02X: 0x%02X\n",arg[0],arg[1],ret) ;
			}
			else {
				ret = eicr_rd(arg[0]) ;
				xil_printf("EIC R 0x%04X: 0x%03X\n",arg[0],ret) ;
			}

			break ;
		case '2' :	// i2c
			if(in[1]=='b') {	// set the bus for the following commands
				i2c_bus = arg[0] ;
				xil_printf("I2C R 0xB: 0x%X\n",i2c_bus) ;	// to get something...
			}
			else if(in[1]=='d') {	// set the device for the following commands
				i2c_device = arg[0] ;
				xil_printf("I2C R 0xD: 0x%X\n",i2c_device) ;	// to get something...
			}
			else if(in[1]=='m') {	// set scan mode: read or write
				i2c_scan_r = arg[0]?1:0 ;
				xil_printf("I2C R 0x1: 0x%X\n",i2c_scan_r) ;	// to get something...
			}
			else if(in[1]=='r') {	/// read register from Si
				ret = i2c_reg_rd(arg[0]) ;
				xil_printf("I2C R 0x%02X: 0x%03X\n",arg[0],ret) ;
			}
			else if(in[1]=='R') {
				ret = i2c_reg16_rd(arg[0]) ;
				xil_printf("I2C R 0x%04X: 0x%03X\n",arg[0],ret) ;
			}
			else if(in[1]=='Q') {
				ret = i2c_reg16_rd4(arg[0]) ;
				xil_printf("I2C R4 0x%04X: 0x%03X\n",arg[0],ret) ;
			}
			else if(in[1]=='W') {
				ret = i2c_reg16_wr(arg[0],arg[1]) ;
				xil_printf("I2C W 0x%04X,0x%02X: %d\n",arg[0],arg[1],ret) ;
			}
			else if(in[1]=='w') {	// write register of Si
				ret = i2c_reg_wr(arg[0],arg[1]) ;
				xil_printf("I2C W 0x%02X,0x%02X: %d\n",arg[0],arg[1],ret) ;
			}
			else {			// scan the bus
				ret = 0 ;
				for(int i=0;i<128;i++) {
					u_char f ;

					i2c_start() ;
					f = i2c_write((i<<1)|i2c_scan_r) ;
					if(f==0) {
						ret++ ;
						xil_printf("I2C %d: 0x%02X OK\n",i2c_bus,i) ;
					}
					i2c_stop();
				}
				xil_printf("I2C %d: scan %d: found %d devices\n",i2c_bus,i2c_scan_r,ret) ;
			}
			break ;

		default:
			break ;
		}
	}

	cleanup_platform();
	return 0;
}
