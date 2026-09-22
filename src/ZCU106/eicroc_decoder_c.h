#ifndef _EICROC_DECODER_C_H_
#define _EICROC_DECODER_C_H_

#include <sys/types.h>

class eicroc_decoder_c {
public:
	eicroc_decoder_c() { evt = 0 ;} ;
	~eicroc_decoder_c() {;} ;

	int evt_start() ;
	int decode(char *buff) ;
	int decode(u_int datum) ;

	char is_fcmd ;

	u_int hdr[8] ;
	u_int trl[8] ;
	int word_cou ;
	int bit_cou ;
	int fmt_type ;
	int asic_type ;
	int b_use ;

	int col_max ;
	int row_max ;

	int evt ;

	// 8 lanes, each with 4 columns or 128 pixels
	u_char lane[8][128*203] ;	// linarized bit-array
	int lane_bits[8] ;
	u_char lane_had_bits[8] ;


	// for a single EICROC0 DOUT
	u_char dout[1000*32] ;
	int dout_bits ;

	struct pixel_t {	// 8 timebins
		u_short tdc[8] ;
		u_char adc[8] ;	
		u_char discr[8] ;
		u_char hdr ;
	} pixel[32][32] ;	// 32 columns, 32 rows



private:
	int state ;
	int l_cou ;
	int trl_cou ;

	int ssc(char *buff, u_int *datum) ;
} ;


#endif
