#include <stdio.h>
#include <sys/types.h>

const int adc_pedestal = 35 ;

int mux64_to_gpio(int ch)
{
	u_char mux[6] ;
	u_char gpio[6] ;

	for(int i=0;i<6;i++) {
		if(ch & (1<<i)) mux[i] = 1 ;
		else mux[i] = 0 ;
	}

	gpio[0] = mux[4] ;
	gpio[1] = mux[3] ;
	gpio[2] = mux[5] ;
	gpio[3] = mux[2] ;
	gpio[4] = mux[0] ;
	gpio[5] = mux[1] ;

	u_char ret = 0 ;
	for(int i=0;i<6;i++) {
		if(gpio[i]) ret |= (1<<i) ;
	}

	return ret ;
}

int mux64_adc_c(char *dest, int ch, int val)
{
	int vch = -1 ;
	double fv ;


	fv = (double) (val-adc_pedestal) / 1024.0 * 2.0 ;


	if(ch==0) {
		fv *= ((10000.0+510.0)/510.0) ;
		sprintf(dest,"ADC2 = %.3f [%u]",fv,val) ;
	}
	else if(ch==1) {
		sprintf(dest,"ADC5 = %.3f [%u]",fv,val) ;
	}
	else if(ch>=2 && ch<=13) {
		sprintf(dest,"ANOUT%02d = %.3f [%u]",fv,ch-1,val) ;
	}
	else if(ch==14) vch = 2  ;
	else if(ch==15) vch = 6 ;
	else if(ch==16) vch = 10 ;

	else if(ch==60) vch = 8 ;
	else if(ch==59) vch = 5 ;
	else if(ch==58) vch = 7  ;
	else if(ch==57) vch = 4 ;
	else if(ch==56) vch = 1 ;
	else if(ch==55) vch = 3  ;

	else if(ch==36) vch = 11 ;
	else if(ch==35) vch = 9 ;
	else if(ch==34) vch = 12  ;

	else if(ch==31) {
		fv *= ((100000.0+200.0)/200.0) ;
		sprintf(dest,"HVMON = %.2f V [%u]",fv,val) ;
	}
	else if(ch==33) {
		sprintf(dest,"PTAT1 = %.3f [%u]",fv,val) ;
	}
	else if(ch==32) {
		sprintf(dest,"PTAT2 = %.3f [%u]",fv,val) ;
	}
	else if(ch==63) {
		sprintf(dest,"GND = %.3f [%u]",fv,val) ;
	}
	else {
		sprintf(dest,"unknown ch %d",ch) ;
		return -1 ;
	}

	if(vch>=0) {
		sprintf(dest,"VTEMP%02d = %.3f [%u]",fv,vch,val) ;
	}

	return 0 ;
}


int lpgbt_adc_c(char *dest, int ch, int val)
{
	double fval = (double)(val-adc_pedestal)/1024.0 ;

	switch(ch) {
	// external pins
	case 0 :
		sprintf(dest,"VTRX TH1 = %.3f [%u]",fval,val) ;
		break ;
	case 1 :	// MUX64
		fval *= ((10+10)/10) ;
		sprintf(dest,"MUX64 = %.3f [%u]",fval,val) ;
		break ;
	case 2 :
		fval *= ((10000.0+510.0)/510.0) ;
		sprintf(dest,"LVRB = %.3f = [%u]",fval,val) ;
		break ;
	case 3 :
		fval *= ((3+1.5)/1.5) ;
		sprintf(dest,"2V5TX = %.3f [%u]",fval,val) ;
		break ;
	case 4 :
		fval *= ((3+3)/3) ;
		sprintf(dest,"VTRX RSSI = %.3f  [%u]",fval,val) ;
		break ;
	case 5 :
		sprintf(dest,"1V2RA (therm) = %.3f [%u]",fval,val) ;
		break ;
	case 6 :
		fval *= ((3+1.5)/1.5) ;
		sprintf(dest,"2V5RX = %.3f [%u]",fval,val) ;
		break ;
	case 7 :
		sprintf(dest,"VDAC (therm) = %.3f [%u]",fval,val) ;
		break ;

	// internal
	case 8 :
		sprintf(dest,"EOM DAC = %.3f [%u]",fval,val) ;
		break ;
	case 9 :
		sprintf(dest,"VSSA = %.3f [%u]",fval,val) ;
		break ;
	case 10 :
		fval /= 0.42 ;
		sprintf(dest,"VDDTX = %.3f [%u]",fval,val) ;
		break ;
	case 11 :
		fval /= 0.42 ;
		sprintf(dest,"VDDRX = %.3f [%u]",fval,val) ;
		break ;
	case 12 :
		fval /= 0.42 ;
		sprintf(dest,"VDD = %.3f [%u]",fval,val) ;
		break ;
	case 13 :
		fval /= 0.42 ;
		sprintf(dest,"VDDA = %.3f [%u]",fval,val) ;
		break ;
	case 14 :
		sprintf(dest,"Temperature = %.3f [%u]",fval,val) ;
		break ;
	case 15 :
		sprintf(dest,"VREF/2 = %.3f [%u]",fval,val) ;
		break ;
	default :
		sprintf(dest,"CH%02d = [%u]",ch,val) ;
		return -1 ;
	}

	return 0 ;
}
