#include <stdio.h>
#include <sys/types.h>
#include <string.h>

// initializes int array for data and int for data counter
static u_int dta[10000] ;
static u_int dta_cou ;

// initializes raw byte array for data and int for byte counter
static u_char cdta[40000] ;
static u_int cdta_cou ;

// takes raw text log stream of hardware events, parses it, unpacks
// custom bit stream, rearranges the byte order (endianness), and extracts
// detector values and formats them into a table

int main()
{
	if (freopen("output.csv", "w", stdout) == NULL) {
        perror("freopen failed");
        return 1;
    }

	for(int evt=0;evt<10000000;evt++) {	// loop over events

	dta_cou = 0 ;
	cdta_cou = 0 ;

	// loop to process all input events from stdin
	while(!feof(stdin)) {
		char buff[1024] ;
		u_int d ;
		int w_cou ;
		int e ;

		// checks for lines in std event format (from zcu106.C)
		if(fgets(buff,sizeof(buff),stdin)==0) continue ;
		// extracts data word d, stores in dta array
		int ret = sscanf(buff,"Evt %d: %d = 0x%X",&e,&w_cou,&d) ;

		if(ret != 3) continue ;

		dta[dta_cou] = d ;
		dta_cou++ ;

		// stops reading when end of packet marker reached
		if(d==0x8FFFFFFF) break ;

	}

	if(dta_cou==0) {
		
		// printf("Done with %d events\n",evt) ;
		break ;
	}
	// first line indicates no. events and no. words
	// printf("Event %d: got %d words.\n",evt,dta_cou) ;

	// indicates header, skips first 2 words
	// printf("Hdr 0x%08X, 0x%08X\n",dta[0],dta[1]) ;

	// indicates trailer, skips last 8 words
	// for(u_int i=(dta_cou-8);i<dta_cou;i++) {
	//	printf("%d Trl 0x%08X\n",i,dta[i]) ;
	// }

	// indicates format of data

	printf("event,column,row,timebin,adc,tdc,hit\n");

	int last = dta_cou-8 ;
//	printf("last 0x%08X\n",dta[last]) ;

//	for(int i=0;i<last;i++) {
//		printf("dta32 %d 0x%08X\n",i,dta[i]) ;
//	}

	// for all remaining words: breaks each 32 bit int into 4 bytes using bit masks and bit shifts
	// saved sequentially in raw byte array cdta
	for(int i=2;i<last;i++) {
//		printf("dta32 0x%08X\n",dta[i]) ;

		cdta[cdta_cou++] = dta[i]&0xFF ;
		cdta[cdta_cou++] = (dta[i]>>8)&0xFF ;
		cdta[cdta_cou++] = (dta[i]>>16)&0xFF ;
		cdta[cdta_cou++] = (dta[i]>>24)&0xFF ;
	}


	int state = 0 ;
	u_int word = 0 ;
	int cou = 31 ;
	int bcou = 0 ;
	u_int latch = 0 ;

	memset(dta,0,sizeof(dta)) ;
	dta_cou = 0 ;

	// iterates through raw bytes, extracting bit 0 of each byte
	// striips out padding bits inserted by hardware
	// reconstructs the extracted bit into a new 32-bit register
	// flushes the result (latch) back into the dta array
	for(u_int i=0;i<cdta_cou;i++) {
		u_int dta8 = cdta[i] ;

//		printf("bits: %d: bcou %d: 0x%02X\n",i,bcou,dta8) ;

		switch(state) {
		case 0 : // VHDL???
#if 0
			word(cou) <= dta8(0) ;	-- bit 0 only
			if(cou=0) then
				latch <= dta8(0) & word(30 downto 0) ;
				wr_en <= '1' ;
				word <= (others=>'0') ;
				cou <= 31 ;
			else
				cou <= cou - 1 ;
			end if ;



#endif
			// pulls out bit 0
			int bit = dta8 & 1 ;	// dta8(0) 

			// removes padding bits, if present
			bcou++ ;
			if(bcou>199) {
				if(bcou==203) {
					bcou = 0 ;
				}
				else {
					//printf("...skip\n") ;
					continue ;
				}
			}
			
			// shifts extracted bit into new 32-bit register
			word |= bit<<cou ;	// word(cou) <= bit

			// once 32 valid bits are collected, the result is flushed back into dta
			// using latch
			// why is it called latch????
			if(cou==0) {
				latch = (word|bit) ;
//				printf("%d 0x%08X\n",i,latch) ;
				dta[dta_cou++] = latch ;

				cou = 31 ;
				word = 0 ;
			}
			else {	
				cou = cou - 1 ;
			}
		}

	}


	int frame_cou = 0 ;
	int pix_cou = 0 ;

	memset(cdta,0,sizeof(cdta)) ;
	cdta_cou = 0 ;

	// endianness swapping/reversing bytes
	// clears cdta and inserts a reversed byte order word
	// little endian -> big endian
	for(u_int i=0;i<dta_cou;i++) {
//		printf("F %d 0x%08X\n",i,dta[i]) ;

		cdta[cdta_cou++] = (dta[i]>>24) & 0xFF ;
		cdta[cdta_cou++] = (dta[i]>>16) & 0xFF ;
		cdta[cdta_cou++] = (dta[i]>>8) & 0xFF ;
		cdta[cdta_cou++] = (dta[i]>>0) & 0xFF ;
	}


	u_int datum = 0 ;
	int pixel = 0 ;

	// decodes final data stream into table format
	// every 25 bytes represents a complete dataframe
	for(u_int i=0;i<cdta_cou;i++) {
		// skips header of each 25 byte block
		if(pix_cou==0) {
			pix_cou++ ;
			pixel = 0 ;


			continue ;
		}
		
//		printf("D %2d 0x%02X\n",pix_cou,cdta[i]) ;

		// remaining 24 bits are processed in groups of 3
		// datum is resulting 24 bit payload
		int ix = (pix_cou-1) % 3 ;

		pix_cou++ ;



		datum |= cdta[i]<<((2-ix)*8) ;

		// for 3 byte payload, extracts:
		// ADC val, TDC val, hit flag
		if(ix==2) {
			int adc = (datum>>13)&0xFF ;
			int tdc = datum & 0x3FF ;
			int hit = (datum & (1<<12))?1:0 ;

			//printf("STROBE %d %d 0x%06X:: %3d %3d %d\n",frame_cou,pixel,datum,
			//       adc,tdc,hit) ;

			if(frame_cou<128) {	// skip debugging junk
			// evt column row timebin adc tdc hit
			printf("%d,%d,%d,%d,%d,%d,%d\n",evt,frame_cou/32,frame_cou%32,pixel,
			       adc,tdc,hit) ;
			}


			datum = 0 ;
			pixel++ ;
		}

		if(pix_cou==25) {
			pix_cou = 0 ;
			frame_cou++ ;
		}
		
	}



	} // of the for(events)... loop

	return 0 ;
}
