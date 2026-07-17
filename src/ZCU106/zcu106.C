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
#include <ZCU106/zcu106_c.h>

static zcu106_c *zcu ;

static u_int dta[1024*1024] ;	

int main(int argc, char *argv[])
{
	int c ;
	int mode = 0 ;
	int num_events = 1 ;
	const char *usb_dev = "UNSET" ;
	int trg_type = 0 ;	// pedestal
	const char *def_fname ;
	int asic_flavor = 0 ;

	while((c=getopt(argc,argv,"m:d:n:t:E:")) != EOF) {
	switch(c) {
	case 'm' :	// execute batch command with argument...
		mode = atoi(optarg) ;
		break ;
	case 'd' :	// change USB device
		usb_dev = optarg ;
		break ;
	case 'n' :
		num_events = atoi(optarg) ;
		break ;
	case 't' :
		trg_type = atoi(optarg) ;
		break ;
	case 'E' :
		asic_flavor = atoi(optarg) ;
		break ;
	default:
		LOG(WARN,"Unknown parameter") ;
		break ;
	}
	}
	
	zcu = new zcu106_c ;	// create an instance


	LOG(INFO,"%s: %s %s: using %s: ASIC %d",argv[0],__DATE__,__TIME__,usb_dev,asic_flavor) ;

	// open the USB port
	if(zcu->dev_open(usb_dev)<0) {
		LOG(ERR,"%s: %s",usb_dev,strerror(errno)) ;
		return -1 ;
	}


	// set board/ASIC flavor
	zcu->set_flavor(asic_flavor) ;	// set EICROC0

	switch(asic_flavor) {
	case 0 :
		def_fname = "asd" ;
		break ;
	case 1 :
		def_fname = "asd" ;
		break ;
	}


		
	switch(mode) {
	case 0 :
		LOG(INFO,"Configuring...") ;
		zcu->set_default_regs(def_fname) ;

		// zcu->i2c_wr() ;

		LOG(INFO,"Configured.") ;
		break ;
	case 1:
	case 2:
		LOG(INFO,"Setting trg_type to %d and taking %d events...",trg_type,num_events) ;
		
		zcu->set_trg_type(trg_type) ;

		for(int i=0;i<num_events;i++) {
			int ret = zcu->get_event(dta) ;
			if(ret<0) {
				LOG(ERR,"Event %d/%d: %d",i,num_events,ret) ;
				break ;
			}

		}

		LOG(INFO,"Done.") ;

		break ;
	default:
		break ;
	}


	return 0 ;
}


	
