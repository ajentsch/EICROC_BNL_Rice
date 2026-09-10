#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <LOG/rtsLog.h>

#include "eicroc_decoder_c.h"


int main(int argc, char *argv[])
{
	int c;

	eicroc_decoder_c decoder ;
	
	// setup
	decoder.is_fcmd = 1 ;	// default is from SDOUT

	while((c=getopt(argc,argv,"E")) != EOF) {
	switch(c) {
	case 'E' :
		decoder.is_fcmd = 0 ;	// use legacy DOUT, not SDOUT
		break ;
	}
	}

	// start a new event...
	decoder.evt_start() ;

	LOG(INFO,"decoder: assuming %s",decoder.is_fcmd?"SDOUT":"DOUT") ;


	while(!feof(stdin)) {	// read from stdin

	char buff[256] ;

	if(fgets(buff,sizeof(buff),stdin)==0) continue ;


	int ret = decoder.decode(buff) ;


	if(ret==3) {

		// dump header
		for(int i=0;i<8;i++) {
			printf("H 0x%08X, T 0x%08X\n",decoder.hdr[i],decoder.trl[i]) ;
		}

		
		int c_max ;

		// limit columns for DOUT
		if(decoder.is_fcmd) c_max = 32 ;
		else c_max = 4 ;

		for(int c=0;c<c_max;c++) {

		if(decoder.lane_had_bits[c/4]) ;	// skip lanes WO data
		else continue ;

		for(int r=0;r<32;r++) {
			printf("Col %2d, row %2d: hdr 0x%02X\n",c,r,decoder.pixel[c][r].hdr) ;

			for(int t=0;t<8;t++) {
				printf("  %d: %3d %3d %d\n",t,
				       decoder.pixel[c][r].adc[t],
				       decoder.pixel[c][r].tdc[t],
				       decoder.pixel[c][r].discr[t]) ;
			}
		}}

		// Restart a new event
		decoder.evt_start() ;
	}


	}	// fgets...

	return 0 ;
}
