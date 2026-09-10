#include <string.h>

#include <LOG/rtsLog.h>

#include "eicroc_decoder_c.h"

int eicroc_decoder_c::evt_start() 
{
	state = 0 ;
	l_cou = 0 ;
	trl_cou = 0 ;

	memset(lane_bits,0,sizeof(lane_bits)) ;
	memset(pixel,0,sizeof(pixel)) ;

	memset(hdr,0xFF,sizeof(hdr)) ;
	memset(trl,0xFF,sizeof(trl)) ;

	memset(lane_had_bits,0,sizeof(lane_had_bits)) ;

	evt++ ;

	return 0 ;
}

int eicroc_decoder_c::ssc(char *buff, u_int *datum)
{
	int l, e ;

	int ret = sscanf(buff,"Evt %d: %d = 0x%X",&e,&l,datum) ;
	if(ret==3) return 1 ;	// all good: a valid datum
	return 0 ;		// some other line
}


int eicroc_decoder_c::decode(char *buff)
{
	u_int datum ;

	if(ssc(buff,&datum)==0) return -1 ;

	re_state: ;
	
//	printf("state %d...\n",state) ;

	switch(state) {

	case 0 :	// wait for header
		hdr[l_cou] = datum ;
		if(l_cou==1) {
			state = 1 ;
			l_cou = 0 ;
			return 0 ;
		}
		else {
			l_cou++ ;
			return 0 ;
		}
		break ;
	case 1 :	// data
		for(int b=0;b<4;b++) {
		int bit ;
		int l_max ;

		if(is_fcmd) l_max=8 ;
		else l_max = 1 ;

		for(int l=0;l<l_max;l++) {
			if(is_fcmd) {
				bit = (datum>>(8*b+l))&1 ;
			}
			else {
				if((datum>>(8*b))&0xFF) bit = 1 ;
				else bit = 0 ;
			}


//			printf("Bit %d, lane %d = %d\n",b,l,bit) ;

			if(bit) lane_had_bits[l]=1 ;	// mark as having _any_ data

			lane[l][lane_bits[l]++] = bit ;
		}
		}


		l_cou++ ;

		if(lane_bits[0]>=203*128) {
			state = 2 ;	// I read all the bits
		}

		break ;
	case 2 :	// wait for the event to end...
		//printf("Trls %d:  0x%08X\n",l_cou,datum) ;
		//LOG(TERR,"state2: datum 0x%08X, trl cou %d",datum,trl_cou) ;
		
		if(datum==0x0FFFEFFF) {	// start of trailer
			trl_cou = 0 ;
			trl[trl_cou++] = datum ;
		}
		else if(trl_cou) {
			trl[trl_cou++] = datum ;
			if(trl_cou==8) {
				if(trl[1]&1) {
					LOG(ERR,"Timeout") ;
				}
				state = 3 ;
				goto re_state ;		// continue to state==3 without a new call
			}
		}

		l_cou++ ;
		
		break  ;
	case 3 :
		// now turn bits to bytes
		//LOG(TERR,"State 3") ;
		//printf("+++ State 3\n") ;

		
		for(int lx=0;lx<8;lx++) {	// for each lane

		//LOG(TERR,"Lane %d, bits %d",lx,lane_bits[lx]) ;

		if(lane_had_bits[lx]==0) continue ;	// skip lanes wo _any_ data

		for(int pix=0;pix<128;pix++) {	// for each pixel of the 4 columns
			int i_start = pix*203 ;
			int i_stop ;
			int i_cou = 0 ;

			int b_cou = 0 ;
			int byte = 0 ;
			int ic = 7 ;
			u_int word = 0 ;

			// right 32x32 indices
			int row = pix % 32 ;
			int column = lx*4 + (pix/32) ;

			int tb_cou = 0 ;


			// hack to move to start of lane
			if(lane[lx][0]==1) ;
			else i_start++ ;

			i_stop = i_start + 202 ;	// each pixel has 203 bits...

			for(int i=i_start;i<=i_stop;i++) {
				//printf("Lane 0: pix %d, bit %d/%d = %d\n",pix,i_cou,i,lane[lx][i]) ;
				i_cou++ ;


				byte |= (lane[lx][i])<<ic ;

				if(ic==0) {	// end of byte
					if(b_cou==0) {	// check header byte if it's 0xAC
						pixel[column][row].hdr = byte ;
						if(byte != 0xAC) {
							if(is_fcmd==0) {	// skip other columns for SDOUT
								if(column<4) {
									LOG(ERR,"Header error: CR %d:%d = 0x%02X",column,row,byte) ;
								}
							}
							else {
								LOG(ERR,"Header error: CR %d:%d = 0x%02X",column,row,byte) ;
							}
						}
					}
					else {
						int ix = (b_cou-1) % 3 ;

						word |= byte<<((2-ix)*8) ;

						if(ix==2) {
							int adc = (word>>13)&0xFF ;
							int tdc = word & 0x3FF ;
							int disc = (word&(1<<12))?1:0 ;

							//printf("CR %d:%d: tb %d: ADC %d %d %d\n",column,row,tb_cou,adc,tdc,disc) ;

							pixel[column][row].adc[tb_cou] = adc ;
							pixel[column][row].tdc[tb_cou] = tdc ;
							pixel[column][row].discr[tb_cou] = disc ;

							
							tb_cou++ ;
							word = 0 ;
						}
					}

					//printf("    Col %2d, row %2d: byte %d = 0x%02X\n",column,row,b_cou,byte) ;

					byte = 0 ;
					ic = 7 ;
					b_cou++ ;
				}
				else {
					ic-- ;
				}
			}
		}	// for pixels
		}	// for lanes

//		goto re_state ;
		state = 4 ;

		return 3 ;

		break ;
	case 4 :
		return 100 ;	// DONE!
		break ;
	}

	return state ;	// not done yet...
}

