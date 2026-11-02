#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct ped_t {
	float mean ;
	float rms ;
	int flag ;
} pedestal[32][32] ;

int main()
{
	char buff[128] ;

#if 0
	// pedestals for all 8 lanes separtely
	for(int l=1;l<=8;l++) {
		int t = 0 ;
		int W = 150 ;
		int w = 4 ;

		int l_mask = (1<<(l-1)) ;

		sprintf(buff,"eicroc -m 3 -n 10000 -w %d -W %d -t %d -l %d | ../ZCU106/ana_dta >m_w%d_W%d_t%d_l%d",
			w,W,t,l_mask,w,W,t,l) ;

		fprintf(stderr,"%s\n",buff) ;
		
		system(buff) ;
	}
#endif

#if 0
	// readin pedestal files and turn into .csv
	FILE *f_ped = fopen("ped.csv","w") ;
	fprintf(f_ped,"Column,Row,ADC,RMS,flag\n") ;

	for(int l=1;l<=8;l++) {
		char fname[128] ;

		int t = 0 ;
		int W = 150 ;
		int w = 4 ;



		sprintf(fname,"m_w%d_W%d_t%d_l%d",
			w,W,t,l) ;

		fprintf(stderr,"%s\n",fname) ;
		

		FILE *f = fopen(fname,"r") ;




		while(!feof(f)) {
			int col, row ;
			float adc, rms ;
			int flag ;

			if(fgets(buff,sizeof(buff),f)==0) continue ;

			if(strstr(buff,"mean")) ;
			else continue ;

			int ret = sscanf(buff,"C %d, R %d: mean %f, sigma %f, flag 0x%X ",
					 &col,&row,&adc,&rms,&flag) ;


			if(ret != 5) continue ;
			
			col %= 4 ;	// 0..3

			col = (l-1)*4+col ;	

			fprintf(f_ped,"%d,%d,%f,%f,%d\n",col,row,adc,rms,flag) ;
			
		}

		fclose(f) ;
	}
	fclose(f_ped) ;
#endif

	// and now load pedesals
	// readin pedestal files and turn into .csv

	fprintf(stderr,"Loading pedestals from ped.csv\n") ;

	FILE *f_ped = fopen("ped.csv","r") ;

	while(!feof(f_ped)) {
			int col, row ;
			float adc, rms ;
			int flag ;

			if(fgets(buff,sizeof(buff),f_ped)==0) continue ;


			int ret = sscanf(buff,"%d,%d,%f,%f,%d",
					 &col,&row,&adc,&rms,&flag) ;


			if(ret != 5) continue ;
			

			pedestal[col][row].mean = adc ;
			pedestal[col][row].rms = adc ;
			pedestal[col][row].flag = flag ;
	}
	fclose(f_ped) ;
	


#if 0
	fprintf(stderr,"Scanning pedestals...\n") ;
	for(int l=1;l<=8;l++) {
		u_int l_mask = 1<<(l-1) ;

		fprintf(stderr,"...lane %d\n",l) ;

	for(int W=12;W<255;W+=8) {
		int t = 0 ;
		int w = 4 ;

		sprintf(buff,"eicroc -m 3 -n 1000 -w %d -W %d -t %d -l %d | ../ZCU106/ana_dta >ped_w%d_W%d_t%d_l%d",
			w,W,t,l_mask,
			w,W,t,l) ;

		fprintf(stderr,"%s\n",buff) ;
		
		system(buff) ;
	}

	}

#endif

#if 0 
	fprintf(stderr,"Scanning pulsers...\n") ;

	for(int l=1;l<=8;l++) {
		int l_mask = (1<<(l-1)) ;

		fprintf(stderr,"...lane %d\n",l) ;

	for(int w=4;w<255;w+=8) {
		int t = 2 ;
		int W = 150 ;

		sprintf(buff,"eicroc -m 3 -n 1000 -w %d -W %d -t %d -l %d | ../ZCU106/ana_dta >puls_w%d_W%d_t%d_l%d",
			w,W,t,l_mask,
			w,W,t,l) ;

		fprintf(stderr,"%s\n",buff) ;
		
		system(buff) ;
	}
	}
#endif

#if 0
	fprintf(stderr,"Doing pedestal stiching...\n") ;

	printf("Column,Row,timebin,ADC,RMS\n") ;

	for(int l=1;l<=8;l++) {
		int l_mask = 1<<(l-1) ;

	for(int W=12;W<255;W+=8) {
		int t = 0 ;
		int w = 4 ;

		sprintf(buff,"ped_w%d_W%d_t%d_l%d",w,W,t,l) ;

		fprintf(stderr,"%s\n",buff) ;

		int t_start = (W+w)-8 ;
		
		FILE *f = fopen(buff,"r") ;
		if(f==0) {
			fprintf(stderr,"FILE ERROR\n") ;
			continue ;
		}

		while(!feof(f)) {
			if(fgets(buff,sizeof(buff),f)==0) continue ;

//			if(strstr(buff,"C  8, R 10")) ;
//			else continue ;

			if(strstr(buff,"mean")) continue ;
			

			int c, r, tb ;
			float adc, rms ;

			int ret = sscanf(buff,"C  %d, R   %d, TB %d: ADC %f +- %f",&c,&r,&tb,&adc,&rms) ;
			if(ret != 5) continue ;



			int tt = t_start+(7-tb) ;

			int col = (c%4)+(l-1)*4 ;

			adc -= pedestal[col][r].mean ;

			if(col==0) printf("%d %d %d %f %f\n",col,r,tt,adc,rms) ;

		}


		fclose(f) ;
	}
	}


#endif

#if 0
	fprintf(stderr,"Doing pulser stitching...\n") ;

	printf("Column,Row,timebin,ADC,RMS\n") ;

	for(int l=1;l<=8;l++) {
	for(int w=4;w<255;w+=8) {
		int t = 2 ;
		int W = 150 ;

		sprintf(buff,"puls_w%d_W%d_t%d_l%d",w,W,t,l) ;

		fprintf(stderr,"%s\n",buff) ;
		
		FILE *f = fopen(buff,"r") ;
		if(f==0) continue ;


		while(!feof(f)) {
			if(fgets(buff,sizeof(buff),f)==0) continue ;


			if(strstr(buff,"mean")) continue ;
			

			int c, r, tb ;
			float adc, rms ;

			int ret = sscanf(buff,"C  %d, R   %d, TB %d: ADC %f +- %f",&c,&r,&tb,&adc,&rms) ;
			if(ret != 5) continue ;



			int tt = W+w+(7-tb) ;


			int col = (c%4)+(l-1)*4 ;

			adc -= pedestal[col][r].mean ;

			printf("%d %d %d %f %f\n",col,r,tt,adc,rms) ;

		}


		fclose(f) ;
	}
	}
#endif

#if 1

	fprintf(stderr,"Scanning threshold...\n") ;

	for(int l=1;l<=8;l++) {
		int l_mask = (1<<(l-1)) ;

		for(int G=490;G<=520;G+=1) {
			int t = 0 ;
			int W = 150 ;
			int w = 4 ;

			sprintf(buff,"eicroc -m 3 -n 1000 -w %d -W %d -t %d -l %d -G %d | ../ZCU106/ana_dta >thr_w%d_W%d_t%d_l%d_G%d",
				w,W,t,l_mask,G,
				w,W,t,l,G) ;

			fprintf(stderr,"%s\n",buff) ;
		
			system(buff) ;
		}
	}
#endif


#if 0

	fprintf(stderr,"Calculating thresholds...\n") ;


	u_int thr_hi[32][32] ;
	u_int thr_lo[32][32] ;

	memset(&thr_hi,0,sizeof(thr_hi)) ;
	memset(&thr_lo,0xFF,sizeof(thr_lo)) ;

	printf("Column,Row,tb,ADC,RMS,TDC,TDC_RMS,Discr,Thr\n") ;

	for(int l=1;l<=8;l++) {
		for(int G=490;G<=520;G+=1) {
			int t = 0 ;
			int W = 150 ;
			int w = 4 ;

			sprintf(buff,"thr_w%d_W%d_t%d_l%d_G%d",
				w,W,t,l,G) ;

			fprintf(stderr,"%s\n",buff) ;
		
			FILE *f = fopen(buff,"r") ;

			if(f==0) {
				perror(buff) ;
				continue ;
			}

			while(!feof(f)) {
				if(fgets(buff,sizeof(buff),f)==0) continue ;




				if(strstr(buff,"mean")) continue ;
			
				
				int c, r, tb ;
				float adc, rms ;
				float tdc, tdc_rms ;
				int discr ;

				int ret = sscanf(buff,"C  %d, R   %d, TB %d: ADC %f +- %f; TDC %f +- %f, Discr %d",&c,&r,&tb,&adc,&rms,
						 &tdc,&tdc_rms,&discr) ;
				if(ret != 8) continue ;



				int tt = W+w+(7-tb) ;

				
				int col = (c%4)+(l-1)*4 ;

				adc -= pedestal[col][r].mean ;

				if(tdc || discr) {
					printf("%d %d %d %f %f %f %f %d %d\n",col,r,tt,adc,rms,tdc,tdc_rms,discr,G) ;
					if(thr_hi[col][r] < G) {
						thr_hi[col][r] = G ;
					}
					if(G < thr_lo[col][r]) {
						thr_lo[col][r] = G ;
					}
				}

				

			}
		}
	}

	for(int c=0;c<32;c++) {
	for(int r=0;r<32;r++) {
		if(thr_lo[c][r]>10000) thr_lo[c][r] = 1023 ;
		printf("%d %d %d %d\n",c,r,thr_lo[c][r],thr_hi[c][r]) ;
	}}

#endif

	return 0 ;
}
