#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#include <LOG/rtsLog.h>
#include <ZCU106/zcu106.h>



int alex_zcu106(int asic_type, int run_type, int num_events, int mode)
{
	LOG(INFO,"alex_zcu106: mode 0x%X",mode) ;

	// do stuff depending on mode&0xFF

	// e.g. open my local .py
	open_py("alex.py") ;

	// apply it
	for(int i=0;i<reg_cou;i++) {
		i2c_wr(regs[i].reg,regs[i].val) ;

	}

	// apply local defaults
	i2c_wr(0x0001,0x80) ;	// 7bits disc threshold
	i2c_wr(0x0002,0x00) ;	// vref 
	i2c_wr(0x0003,0x94) ;	// on_ctest?
	i2c_wr(0x0004,0x29) ;	// no idea... EICROC1 was 0x01
	i2c_wr(0x0005,0x00) ;	// no idea... EICROC1 was 0x20

	// example loops

	for(int vref=0;vref<0x70;vref+=2) {
		char fname[256] ;

		// change the default vref
		i2c_wr(0x0002,vref) ;	// vref 

		sprintf(fname,"vref%03d.dta",vref) ;
		FILE *f = fopen(fname,"w") ;
		if(f==0) {
			LOG(ERR,"%s: %s",fname,strerror(errno)) ;
			break ;
		}

		LOG(TERR,"Running %d events into %s",num_events,fname) ;

		run_asic(f, num_events) ;	// readout num_events and store to f
		fclose(f) ;
	}




	return 0 ;
}
