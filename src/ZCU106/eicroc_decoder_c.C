#include <string.h>

#include <LOG/rtsLog.h>

#include "eicroc_decoder_c.h"

int eicroc_decoder_c::evt_start() 
{
	LOG(NOTE,"Evt start %d: state %d, l_cou %d",evt,state,l_cou) ;

	if(rbv1_fmt) {	// will only get executed after the first event
		l_cou = 0 ;
		return 0 ;
	}

	state = 0 ;
	l_cou = 0 ;
	trl_cou = 0 ;
	
	memset(lane_bits,0,sizeof(lane_bits)) ;
	memset(pixel,0,sizeof(pixel)) ;

	memset(hdr,0xFF,sizeof(hdr)) ;
	memset(trl,0xFF,sizeof(trl)) ;

	memset(lane_had_bits,0,sizeof(lane_had_bits)) ;

	dout_bits = 0 ;

	evt++ ;

	hdr_len = 2 ;	// at least
	rbv1_fmt = 0 ;

	return 0 ;
}

int eicroc_decoder_c::ssc(char *buff, u_int *datum)
{
	int l, e ;

	int ret = sscanf(buff,"Evt %d: %d = 0x%X",&e,&l,datum) ;
	if(ret==3) return 1 ;	// all good: a valid datum
	return 0 ;		// some other line
}


int eicroc_decoder_c::rbv1(u_int datum)
{
	if(rbv1_fmt==0) {	// thus first time this is called
		asic_type = 1 ;
		hdr_len = 16 ;
		fmt_type = 0 ;

		col_max = 32 ;
		row_max = 32 ;

		rbv1_fmt = 1 ;	
	}

	re_state: ;

//	LOG(TERR,"State %d: 0x%08X",state,datum) ;

	switch(state) {
	case 0 :	// tslice header
		hdr1[l_cou] = datum ;
		if(l_cou==15) {
			state = 1 ;
			return 0 ;
		}
		l_cou++ ;
		return 0 ;
		break ;
	case 1 :	// ASIC header
		if(datum!=0x88CD0000) {
			LOG(ERR,"Not ASIC header 0x%08X",datum) ;
			return -1 ;
		}
		LOG(DBG,"ASIC start") ;
		l_cou = 0 ;
		state = 2 ;
		return 0 ;
	case 2 :	// ASIC data
		if(datum&0xF0000000) {
			if(datum==0x44760000) {	// ASIC trailer
				LOG(DBG,"ASIC end") ;
				state = 3 ;
				goto re_state ;
			}
			else {
				LOG(ERR,"Invalid ASIC data 0x%08X",datum) ;
				return -1 ;
			}
		}

		for(int b=0;b<3;b++) {
			int bit ;
			int l_max = 8 ;


			for(int l=0;l<l_max;l++) {
				bit = (datum>>(8*b+l))&1 ;

				//printf("byte %d, lane %d = %d\n",b,l,bit) ;

				if(bit) lane_had_bits[l]=1 ;	// mark as having _any_ data

				if(lane_had_bits[l]) lane[l][lane_bits[l]++] = bit ;

				//printf("Lane %d, bits %d/%d\n",l,lane_bits[l],bit_cou) ;
			}
		}


		l_cou++ ;

		return 0 ;
	case 3 :
		LOG(DBG,"Working on ASIC data") ;

		for(int lx=0;lx<8;lx++) {	// for each lane
		
		int lane_start_ix = 0 ;

		//LOG(TERR,"Lane %d, bits %d",lx,lane_bits[lx]) ;

		if(lane_had_bits[lx]==0) continue ;	// skip lanes wo _any_ data

		LOG(DBG,"Lane %d had bits",lx) ;


		// move to start of lane data...
		for(int i=0;i<1000;i++) {
			if(lane[lx][i]==1) {
				lane_start_ix = i ;
				break ;
			}
		}

		LOG(NOTE,"Lane %d starts at %d",lx,lane_start_ix) ;

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


			for(int i=0;i<100;i++) {
				if(lane[lx][i]==1) break ;
				i_start++ ;
			}

//			// hack to move to start of lane
//			if(lane[lx][0]==1) ;
//			else i_start++ ;

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
									LOG(ERR,"Header error: evt %d: CR %d:%d = 0x%02X",evt,column,row,byte) ;
								}
							}
							else {
								LOG(ERR,"Header error: evt %d: CR %d:%d = 0x%02X",evt,column,row,byte) ;
							}
						}
						else {
							LOG(NOTE,"evt %d: CR %d:%d = 0x%02X",evt,column,row,byte) ;
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


		state = 4 ;
		l_cou = 0 ;
		
		//LOG(TERR,"ASIC data ready") ;

		return 3 ;
	case 4 :
		
		//LOG(TERR,"State %d, datum 0x%08X",state,datum) ;

		if(datum==0x88CD0000) {	// start of ASIC
			evt++ ;

			memset(lane_bits,0,sizeof(lane_bits)) ;
			memset(pixel,0,sizeof(pixel)) ;

			memset(hdr,0xFF,sizeof(hdr)) ;
			memset(trl,0xFF,sizeof(trl)) ;

			memset(lane_had_bits,0,sizeof(lane_had_bits)) ;

			dout_bits = 0 ;

			l_cou = 0 ;
			state = 1 ;
			goto re_state ;
		}
		
		return 0 ;
	}


	return -1 ;
		
	
}

int eicroc_decoder_c::decode(char *buff)
{
	u_int datum ;


	if(ssc(buff,&datum)==0) return -1 ;

	if(rbv1_fmt) return rbv1(datum) ;
	else if(state==0 && l_cou==0 && datum==0xABCDFFFF) return rbv1(datum) ;


	re_state: ;
	
	LOG(DBG,"state %d",state) ;

	switch(state) {

	case 0 :	// wait for header
		hdr[l_cou] = datum ;
		if(l_cou==(hdr_len-1)) {		// end-of-header, just 2 words
			if(hdr[0]!=0x87654321) {
				if(hdr[0]==0xABCDFFFF) {	// RBv1
					LOG(TERR,"RBv1, hdr_len %d",hdr_len) ;
					asic_type = 1 ;		// EICROC1

					fmt_type = 0 ;

					rbv1_fmt = 1 ;

					word_cou = 123 ;
					col_max = 32 ;
					row_max = 32 ;
					bit_cou = 203*128 ;

					hdr_len = 17 ;
					l_cou++ ;
					
					is_fcmd = 1 ;

					if(l_cou==17) state = 1 ;
				}
				else {
					hdr_len = 2 ;

					word_cou = hdr[1]&0xFFFF ;
					fmt_type = (hdr[1]>>16)&0xFF ;
					asic_type = (hdr[1]>>28)&0xF ;
					bit_cou = 0 ;

				

					if(asic_type==0) {	// EICROC0A
						col_max = 1 ;
						row_max = 16 ;
						state = 11 ;	// for EICROC0A
						is_fcmd = 0 ;	// override, if I can...
					}
					else if(asic_type==10) {  // EICROC0
						col_max = 1 ;
						row_max = 16 ;
						state = 11 ;    // for EICROC0 -- FIXME, is it true for EICROC0, or only 0A??
						is_fcmd = 0 ;   // override, if I can...
					}
					else {			// can only be EICROC1 so far
						
						col_max = 32 ;
						row_max = 32 ;
						bit_cou = 203*128 ;
						state = 1 ;
					}

					l_cou = 0 ;
				}
			}
			else {	// OLD FW

				word_cou = 8000 ;
				fmt_type = 0 ;
				asic_type = 1 ;
				bit_cou = 203*128 ;

				col_max = 32 ;
				row_max = 32 ;

				state = 1 ;
				hdr_len = 2 ;

				l_cou = 0 ;
			}

			//l_cou = 0 ;	// zap word count
			return 0 ;
		}
		else {
			l_cou++ ;
			return 0 ;
		}
		break ;
	case 11 :
		for(int b=0;b<32;b++) {
			if(datum&(1<<b)) {
				dout[dout_bits] = 1  ;
			}
			else {
				dout[dout_bits] = 0 ;
			}
			dout_bits++ ;
		}
		l_cou++ ;

		if(l_cou>=word_cou) state = 22 ;
		
		break ;
	case 22 :
		{
		int b_start = -1 ;
		int c_cou = 0 ;
		u_char byte[30] ;

		for(int b=0;b<8;b++) {
			if(dout[b]==1) {
				b_start = b ;
				lane_had_bits[0] = 1 ;
				break ;
			}
		}
			
		b_start += 3 ;		// add a few bits to center around the data bit
		int och_cou = 7 ;
		int och = 0 ;
		int row = 0 ;

		for(int b=b_start;b<dout_bits;b+=8) {
			if(dout[b]==1) {
				och |= (1<<och_cou) ;
			}
			
			//for(int i=0;i<8;i++) {
			//	printf("...%d %d\n",b+i,dout[b+i]) ;
			//}

			if(och_cou==0) {
				byte[c_cou] = och ;

				//printf("%d = 0x%02X\n",c_cou,och) ;
				och_cou= 7 ;
				och = 0 ;
				
				if(c_cou==24) {

					pixel[0][row].hdr = byte[0] ;

					//printf("Row %d, hdr 0x%02X\n", row, byte[0]) ;

					u_int word  = 0 ;
					u_int tb = 0 ;

					for(int i=1;i<=24;i++) {
						int ix = (i-1)%3 ;

						word |= byte[i]<<((2-ix)*8) ;

						if(ix==2) {
							int adc = (word>>13)&0xFF ;
							int tdc = word & 0x3FF ;
							int disc = (word&(1<<12))?1:0 ;
							
							//printf("... tb %d: ADC %d, TDC %d, disc %d\n",
							//       tb,adc,tdc,disc) ;

							pixel[0][row].adc[tb] = adc ;
							pixel[0][row].tdc[tb] = tdc ;
							pixel[0][row].discr[tb] = disc ;

							tb++ ;
							word = 0 ;
						}
					}

					b +=24 ;
					c_cou = 0 ;
					row++ ;
					if(row==16) {
						//LOG(TERR,"to state 2") ;
						state = 2 ;	// grab trailer
						goto re_state ;
					}
				}
				else c_cou++ ;
			}
			else och_cou-- ;

		}

		}


		break ;
	case 1 :

		LOG(DBG,"state1: datum 0x%08X",datum) ;

		int b_max ;
		if(rbv1_fmt) {
			b_max = 3 ;
		}
		else {
			b_max = 4 ;
		}

		for(int b=0;b<b_max;b++) {
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


			//printf("byte %d, lane %d = %d\n",b,l,bit) ;

			if(bit) lane_had_bits[l]=1 ;	// mark as having _any_ data

			if(lane_had_bits[l]) lane[l][lane_bits[l]++] = bit ;

			//printf("Lane %d, bits %d/%d\n",l,lane_bits[l],bit_cou) ;
		}
		}


		l_cou++ ;

		if(bit_cou) {
			for(int i=0;i<8;i++) {
			if(lane_bits[i]>=bit_cou) {
				state = 2 ;	// I read all the bits
				break ;
			}
			}
		}
		else {
			if(l_cou>=word_cou) {
				state = 2 ;
			}
		}

		break ;
	case 2 :	// trailer: wait for the event to end...

		LOG(NOTE,"state2: hdr[0] 0x%08X, datum 0x%08X, trl cou %d",hdr[0],datum,trl_cou) ;
		
		
		if(hdr[0]==0x87654321 && datum==0x0FFFEFFF) {	// start of trailer
			trl_cou = 0 ;
			trl[trl_cou++] = datum ;
		}
		else if(hdr[0]==0xABCDFFFF) {
			state = 3 ;
			goto re_state ;
		}
		else if(datum==0xCDEFEC01) {
			trl_cou = 0 ;
			trl[trl_cou++] = datum ;
		}
		else if(trl_cou) {
			trl[trl_cou++] = datum ;
			if(trl_cou==8) {
				if(trl[3]&0xFF) {
					LOG(ERR,"Evt %d: timeout, no data was sent",evt) ;
				}
				state = 3 ;
				goto re_state ;		// continue to state==3 without a new call
			}
		}

		l_cou++ ;
		
		break  ;
	case 3 :
		// now turn bits to bytes
		LOG(NOTE,"State 3") ;
		//printf("+++ State 3\n") ;

		if(asic_type==0 || asic_type==10) return 3 ; //added EICROC0 explicitly otherwise decoding will fail
		
		for(int lx=0;lx<8;lx++) {	// for each lane
		
		int lane_start_ix = 0 ;

		//LOG(TERR,"Lane %d, bits %d",lx,lane_bits[lx]) ;

		if(lane_had_bits[lx]==0) continue ;	// skip lanes wo _any_ data

		LOG(NOTE,"Lane %d had bits",lx) ;


		// move to start of lane data...
		for(int i=0;i<1000;i++) {
			if(lane[lx][i]==1) {
				lane_start_ix = i ;
				break ;
			}
		}

		LOG(NOTE,"Lane %d starts at %d",lx,lane_start_ix) ;

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


			for(int i=0;i<100;i++) {
				if(lane[lx][i]==1) break ;
				i_start++ ;
			}

//			// hack to move to start of lane
//			if(lane[lx][0]==1) ;
//			else i_start++ ;

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
									LOG(ERR,"Header error: evt %d: CR %d:%d = 0x%02X",evt,column,row,byte) ;
								}
							}
							else {
								LOG(ERR,"Header error: evt %d: CR %d:%d = 0x%02X",evt,column,row,byte) ;
							}
						}
						else {
							LOG(NOTE,"evt %d: CR %d:%d = 0x%02X",evt,column,row,byte) ;
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

