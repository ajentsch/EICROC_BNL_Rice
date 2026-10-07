#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include <LOG/rtsLog.h>
volatile int rtsLogLevel = 0 ;

#include "eicroc_decoder_c.h"

static eicroc_decoder_c decoder ;

static int evts  ;
static int run_type ;

static int ped_add() ;
static int ped_calc() ;

static double mean[32][32][8] ;
static double sigma[32][32][8] ;
static double p_mean[32][32] ;
static double p_sigma[32][32] ;

static double tdc_mean[32][32][8] ;
static double tdc_sigma[32][32][8] ;

static int d_mean[32][32][8] ;

static int ped_add() 
{
	for(int c=0;c<decoder.col_max;c++) {
	for(int r=0;r<decoder.row_max;r++) {
		u_int flag = 0 ;
		int tdc_cou = 0 ;
		int discr_cou = 0 ;

		for(int t=0;t<8;t++) {
			u_int adc = decoder.pixel[c][r].adc[t] ;
			u_int tdc = decoder.pixel[c][r].tdc[t] ;
			int discr = decoder.pixel[c][r].discr[t] ;

			switch(run_type) {
			case 0 :	// pedestal
				if(tdc) flag |= 1 ;
				if(discr) flag |= 2 ;
				if(adc<10) flag |= 4 ;
				if(adc>250) flag |= 8 ;

				break ;
			case 2 :
				if(adc<3) flag |= 4 ;
				if(adc>250) flag |= 8 ;

				if(tdc) tdc_cou++ ;
				if(discr) {
					discr_cou++ ;
					d_mean[c][r][t]++ ;
				}
				break ;
			}


			tdc_mean[c][r][t] += tdc ;
			tdc_sigma[c][r][t] += tdc*tdc ;

			mean[c][r][t] += adc ;
			sigma[c][r][t] += adc*adc ;

			p_mean[c][r] += adc ;
			p_sigma[c][r] += adc*adc ;
		}

		if(run_type==2) {
			if(tdc_cou != 1 || discr_cou != 1) flag |= 0x10 ;
		}

		if(flag) {
			LOG(WARN,"Evt %d: pixel %d:%d flag 0x%X",evts,c,r,flag) ;
		}
	}}

	evts++ ;

	return 0 ;
}


static int ped_calc()
{
	LOG(INFO,"Calculating with %d events",evts) ;
	for(int c=0;c<decoder.col_max;c++) {
	for(int r=0;r<decoder.row_max;r++) {
		double pp_mean ;
		double pp_sigma ;
		double tt_mean ;
		double tt_sigma ;

		pp_mean = p_mean[c][r]/evts/8 ;
		pp_sigma = sqrt(p_sigma[c][r]/evts/8 - pp_mean*pp_mean) ;
		

		printf("C %2d, R %2d: mean %.3f, sigma %.3f\n",c,r,pp_mean,pp_sigma) ;

		for(int t=0;t<8;t++) {
			int dd ;

			pp_mean = mean[c][r][t]/evts ;
			pp_sigma = sqrt(sigma[c][r][t]/evts - pp_mean*pp_mean) ;

			tt_mean = tdc_mean[c][r][t]/evts ;
			tt_sigma = sqrt(tdc_sigma[c][r][t]/evts - tt_mean*tt_mean) ;


			if(d_mean[c][r][t]==evts) dd = 1 ;
			else if(d_mean[c][r][t]==0) dd = 0 ;
			else dd = 2 ;

			printf("C %2d, R %2d, TB %d: ADC %.3f +- %.3f; TDC %.3f +- %.3f, Discr %d\n",c,r,t,
			       pp_mean,pp_sigma,
			       tt_mean, tt_sigma,dd) ;
		}
	}}


	return 0 ;
}


int main(int argc, char *argv[])
{
	int c;
	int first = 1 ;

	// setup

	while((c=getopt(argc,argv,"Et:")) != EOF) {
	switch(c) {
	case 'E' :
		decoder.is_fcmd = 0 ;	// use legacy DOUT, not SDOUT
		break ;
	case 't' :
		run_type = atoi(optarg) ;
		break ;
	}
	}

	// start a new event...
	decoder.evt_start() ;




	while(!feof(stdin)) {	// read from stdin

	char buff[256] ;

	if(fgets(buff,sizeof(buff),stdin)==0) continue ;

	int ret = decoder.decode(buff) ;

	if(ret==3) {	// flags end of event, let's do something with the event data
		if(first) {
			LOG(INFO,"ASIC type %d, words %d",decoder.asic_type,decoder.word_cou) ;
			first = 0 ;
		}
		ped_add() ;

		// Restart a new event: REQUIRED!
		decoder.evt_start() ;
	}


	}	// fgets...


	ped_calc() ;

	return 0 ;
}
