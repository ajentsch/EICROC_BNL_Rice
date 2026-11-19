#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <sys/file.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>

#include <LOG/rtsLog.h>
#include <ZCU106/zcu106lib.h>


static int ser_usb ;
static int i2c_dev ;


#define GPIO_BASE 0xA0000000    /* AXI GPIO base from the Address Editor */
#define OUT (0x0 / 4)           /* channel 1: PS -> PL (ps2pl) */
#define IN  (0x8 / 4)           /* channel 2: PL -> PS (pl2ps) */


static volatile u_int *gpio_reg_wr ;
static volatile u_int *gpio_reg_rd ;

static u_int gpio_wr(u_int reg, u_short val)
{
        reg &= 0xFF ;
        reg <<= 16 ;
        reg |= 0xC0000000 | val ;

        *gpio_reg_wr = reg ;
        *gpio_reg_wr = 0 ;

	return val ;
}

static u_int gpio_rd(u_int reg) 
{
        reg &= 0xFF ;
        reg <<= 16 ;
        reg |= 0x80000000 ;

        *gpio_reg_wr = reg ;

        reg = *gpio_reg_rd ;

        *gpio_reg_wr = 0 ;

	return reg ;
}

static void gpio_set_bit(int reg, int bit)
{
	u_int val = gpio_rd(reg) ;
	gpio_wr(reg,val|(1<<bit)) ;
}

static void gpio_clr_bit(int reg, int bit)
{
	u_int tmp = gpio_rd(reg) ;
	tmp &= ~(1<<bit) ;
	gpio_wr(reg,tmp) ;
}

static u_char gpio_get_bit(int reg, int bit)
{
	u_int tmp = gpio_rd(reg) ;
	if(tmp & (1<<bit)) return 1 ;
	return 0 ;
}

static char *hwicap_version(u_int v)
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

static int gpio_init()
{
	ser_usb = -1 ;	// indicate 
	i2c_dev = 0 ;	// mark as unset

	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0) { 
		LOG(ERR,"/dev/mem [%s]",strerror(errno)) ;
		return -1 ;
	}


	void *map = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, GPIO_BASE);

	if (map == MAP_FAILED) { 
		LOG(ERR,"mmap [%s]",strerror(errno)) ;
		return -1 ;
	}
	
	volatile u_int *gpio = (volatile u_int *)map;  /* cast needed in C++ */


        gpio_reg_wr = gpio ;
        gpio_reg_rd = gpio+2 ;


	u_int version = gpio_rd(8) ;

	LOG(INFO,"ZCU:init: FW version %s[0x%08X]",hwicap_version(version),version) ;

	return 0 ;
}



int zcu_reg_dump()
{
	for(int i=0;i<16;i++) {
		printf("ZCU106 reg %2d = 0x%08X\n",i,gpio_rd(i)) ;
	}

	return 0 ;
}

int zcu_ser_open(const char *usb_dev)
{	
//	int baud = 230400 ;
	int baud = 115200 ;
//	int baud = 19200 ;
	int block = 0 ;	// never block in this application...

	ser_usb = -1 ;
	i2c_dev = 0 ;

	if(usb_dev==0) {
		return gpio_init() ;
	}

	if(block==0) {
		ser_usb = open(usb_dev,O_RDWR|O_NOCTTY|O_NONBLOCK|O_NDELAY,0666) ;
	}
	else {
		ser_usb = open(usb_dev,O_RDWR|O_NOCTTY,0666) ;
	}

	if(ser_usb<0) {
		LOG(ERR,"%s [%s]",usb_dev,strerror(errno)) ;
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
		LOG(ERR,"%s [%s]",usb_dev,strerror(errno)) ;
		close(ser_usb) ;
		ser_usb = -1 ;
		return -1 ;
	}

	LOG(NOTE,"zcu_ser_open(%s) OK",usb_dev) ;

	return ser_usb ;

}

static int ser_read(u_char *ch)
{
	if(ser_usb<0) {
		LOG(WARN,"USB not present") ;
		*ch = '?' ;
		return 1 ;
	}

	int ret = read(ser_usb,ch,1) ;
	if(ret<0) {
		if(errno==EAGAIN) {
			return 0 ;
		}
		LOG(ERR,"ser_read") ;
		return -1 ;
	}
	if(ret==0) {
		LOG(ERR,"ser_read: got 0") ;
		return -1 ;
	}
	return ret ;
}

static int ser_ln_read(char *buff)
{
	char *c = buff ;
	u_char ch ;
	int s_cou = 0 ;
	int sret = 0 ;


	if(ser_usb<0) {
		LOG(WARN,"USB not present") ;
		return 0 ;
	}

	for(;;) {
		int ret = ser_read(&ch) ;
		if(ret<0) {
			LOG(ERR,"ser_ln_read") ;
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


static int ser_write_b(const void *buffer, int bytes)
{
	if(ser_usb<0) {
		LOG(WARN,"USB not present") ;
		return 0 ;
	}

	int ret = write(ser_usb,buffer,bytes) ;

	if(ret != bytes) {
		LOG(ERR,"ser_write_b") ;
	}

//	usleep(100) ;

	return ret ;
}

static int ser_write(const char *buffer)
{
	int bytes = strlen(buffer) ;
	return ser_write_b(buffer,bytes) ;
}


u_int zcu_rd(u_int reg)
{
	if(ser_usb<0) return gpio_rd(reg) ;

	char cmd[128] ;
	u_int r ;
	u_int val ;
 
	sprintf(cmd,"r %u\n",reg) ;
	ser_write(cmd) ;

	ser_ln_read(cmd) ;
	int ret = sscanf(cmd,"R %u = 0x%X",&r,&val) ;

	if(ret==2 && r==reg) {
		return val ;
	}

	LOG(ERR,"zcu_rd %d",reg) ;
	return 0 ;
}

u_int zcu_wr(u_int reg, u_int val)
{
	if(ser_usb<0) return gpio_wr(reg,val) ;

	char cmd[128] ;
	u_int r ;

	sprintf(cmd,"w %u,0x%X\n",reg,val) ;
	ser_write(cmd) ;

	ser_ln_read(cmd) ;
	int ret = sscanf(cmd,"R %u = 0x%X",&r,&val) ;

	if(ret==2 && r==reg) {
		return val ;
	}

	LOG(ERR,"zcu_wr %d 0x%08X",reg,val) ;
	return 0 ;
}

static void i2c_scl(int on)
{
        if(on) {
                gpio_set_bit(0,8) ;
	}
        else {
                gpio_clr_bit(0,8) ;
	}
}

static void i2c_sda(int on)
{

        gpio_set_bit(0,10) ;     // drive SDA

        if(on) {
                gpio_set_bit(0,9) ;
	}
        else {
                gpio_clr_bit(0,9) ;
	}


}

static u_char i2c_sdz()
{
        //SDA to 1
//      gpio_set_bit(0,9) ;

        gpio_clr_bit(0,10) ;     // SDA to Z

        u_int val = gpio_get_bit(0,9+16) ;       // read SDA

        if(val) {
                return 1 ;
	}

        return 0 ;
}


static void i2c_start()
{
        i2c_scl(1) ;
        i2c_sda(1) ;

        i2c_sda(0) ;
        i2c_scl(0) ;
}

static u_char i2c_stop()
{
        i2c_sda(0) ;
        i2c_scl(1) ;
        i2c_sda(1) ;

        u_char a = i2c_sdz() ;

//      xil_printf("2S %d\n",a) ;

        return a ;
}

static u_char i2c_write(u_char val)
{
        int i ;
        u_char ret0 ;
	u_char ret1 ;

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

//      xil_printf("2W 0x%02X: %d %d\n",val,ret0,ret1) ;

        return ret1 ;
}

static void i2c_read_m(int cou, u_short *val)
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


static u_short eicr_rd(u_short reg)
{
        u_char ack ;
        u_short val = 0xF000 ;
        u_short retval = 0 ;

        i2c_start() ;
        ack = i2c_write(i2c_dev<<1|0) ;
        if(ack) retval |= 0x100 ;
        ack = i2c_write(reg&0xFF) ;
        if(ack) retval |= 0x200 ;
        i2c_stop() ;

        i2c_start() ;
        ack = i2c_write((i2c_dev+1)<<1|0) ;
        if(ack) retval |= 0x400 ;
        ack = i2c_write(reg>>8) ;
        if(ack) retval |= 0x800 ;
        i2c_stop() ;

        i2c_start() ;
        ack = i2c_write((i2c_dev+2)<<1|1) ;
        i2c_read_m(1,&val) ;
        i2c_stop() ;

        return (retval|val) ;


}

static u_short eicr_wr(u_short reg, u_char val)
{
        u_char ack ;
        u_short retval = 0 ;

        // LO
        i2c_start() ;
        ack = i2c_write(i2c_dev<<1|0) ;
        if(ack) retval |= 0x1 ;
        ack = i2c_write(reg&0xFF) ;
        if(ack) retval |= 0x2 ;
        i2c_stop() ;

        // HI
        i2c_start() ;
        ack = i2c_write((i2c_dev+1)<<1|0) ;
        if(ack) retval |= 0x4 ;
        ack = i2c_write(reg>>8) ;
        if(ack) retval |= 0x8 ;
        i2c_stop() ;

        // VAL
        i2c_start() ;
        ack = i2c_write((i2c_dev+2)<<1|0) ;
        if(ack) retval |= 0x10 ;
        ack = i2c_write(val) ;
        if(ack) retval |= 0x20 ;
        i2c_stop() ;

        return retval ; // error if non-0
}

u_short zcu_i2c_wr(u_short reg, u_char val)
{

	char cmd[32] ;
	u_int r ;
	u_int err ;

	if(ser_usb<0) {
		return eicr_wr(reg,val) ;
		LOG(ERR,"not yet done") ;
		return 0xFFFF ;
	}

	

//	if( (reg&0xFF00)==0x4000 || (reg&0xF00)==0x200 ) {	// global register -- added || is for EICROC0 addresses
//		i2c_glo_shadow[reg&0xFF] = val ;
//	}

	sprintf(cmd,"Ew %u,0x%X\n",reg,val) ;
	ser_write(cmd) ;

	ser_ln_read(cmd) ;
	int ret = sscanf(cmd,"EIC W 0x%X,0x%X: 0x%X",&r,&val,&err) ;

	if(ret==3 && r==reg) {
		return err ;
	}

	LOG(ERR,"i2c_wr 0x%04X 0x%02X",reg,val) ;
	return 0xFFFF ;

}

u_short zcu_i2c_rd(u_short reg)
{
	char cmd[32] ;
	u_int r ;
	u_int val ;

	if(ser_usb<0) {
		return eicr_rd(reg) ;
		LOG(ERR,"not yet done") ;
		return 0xFFFF ;
	}

	sprintf(cmd,"Er 0x%X\n",reg) ;
	ser_write(cmd) ;

	ser_ln_read(cmd) ;
	int ret = sscanf(cmd,"EIC R 0x%X: 0x%X",&r,&val) ;
	
	if(ret==2 && r==reg) {
		return val ;
	}

	LOG(ERR,"i2c_rd 0x%04X",reg) ;
	return 0xFFFF ;
	
}

u_short zcu_i2c_set_dev(u_char dev)
{
	i2c_dev = dev ;

	if(ser_usb<0) {
		return 0 ;
	}

	char buff[128] ;

	sprintf(buff,"2d 0x%X\n",dev) ;
	ser_write(buff) ;

	while(ser_ln_read(buff)>0) ;	// get rid of junk

	return dev ;
}

