#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#include <LOG/rtsLog.h>
#include <ZCU106/zcu106.h>

int rewriteLocalRegisters();

int alex_zcu106(int asic_type, int run_type, int num_events, int mode)
{
	LOG(INFO,"alex_zcu106: mode 0x%X  with ASIC type %d",mode, asic_type) ;

	// do stuff depending on mode&0xFF

	// e.g. open my local .py
	if(asic_type == 0 ){ open_py("alex.py") ; }
    else if(asic_type == 10){ open_py("eicroc0_global_reg_defaults.py") ; }
    else{
        LOG(ERR, "Bad ASIC type used...exiting...");
        return 0;
    }

	int current_reg_DAC_pulser = 0;
	int current_reg_MSB_Vth    = 0;
	

	// apply it
	for(int i=0;i<reg_cou;i++) {
		LOG(INFO, "Looping over Alex Regi assignments...%02X --> %02X", regs[i].reg, regs[i].val);
		i2c_wr(regs[i].reg, regs[i].val) ;
        i2c_rd(regs[i].reg);
		
        if(regs[i].reg == 0x400B){ current_reg_MSB_Vth = regs[i].val; }
		if(regs[i].reg == 0x400C){ current_reg_DAC_pulser = regs[i].val; }

        if(regs[i].reg == 0x20B){ current_reg_MSB_Vth = regs[i].val; }
	    if(regs[i].reg == 0x20C){ current_reg_DAC_pulser = regs[i].val; }
	}

	// apply local defaults
	//i2c_wr(0x0001,0x80) ;	// 7bits disc threshold
	//i2c_wr(0x0002,0x00) ;	// vref 
	//i2c_wr(0x0003,0x94) ;	// on_ctest?
	//i2c_wr(0x0004,0x29) ;	// no idea... EICROC1 was 0x01
	//i2c_wr(0x0005,0x00) ;	// no idea... EICROC1 was 0x20

	//eic_clib.write_asic_indirect_reg(0x400A, 0b00000000) # vth 7:0
	//eic_clib.write_asic_indirect_reg(0x400B, 0b00000001) # bias PA + vth 9:8
	//eic_clib.write_asic_indirect_reg(0x400C, 0b11100000) # refs + charge dacbpulser

    /////add this to zcu mode number

    int vth_start      = 0;
    int vth_step_size  = 5;
    int N_vth_steps    = 120;

    int q_start      = 60;
    int q_step_size  = 2;
    int N_q_steps    = 1;

    int vth_corr_start     = 0;
    int vth_corr_step_size = 1;
    int N_vth_corr_steps   = 126;

    int vref_corr_start     = 0;
    int vref_corr_step_size = 1;
    int N_vref_corr_steps   = 126;

    char outputFolderName[256];
    char asicFolder[256];    
 
    //default folder name in case an option is not specified   
    sprintf(outputFolderName,"./alexData/");


    if( (mode & 0x010) == 0x010){ //threshold global scan
        
        q_start      = 6;
        q_step_size  = 1;   
        N_q_steps    = 1;

        vth_corr_start     = 0;
        vth_corr_step_size = 1;
        N_vth_corr_steps   = 1;

        vref_corr_start     = 65;
        vref_corr_step_size = 1;
        N_vref_corr_steps   = 1;

        if(asic_type ==  0){ sprintf(asicFolder, "output_EICROC0A");}
        if(asic_type == 10){ sprintf(asicFolder, "output_EICROC0");}
        sprintf(outputFolderName,"./%s/thresholdScanData/", asicFolder);
    }
    if( (mode & 0x020) == 0x020){ //charge scan

        vth_start      = 200;
        vth_step_size  = 1;
        N_vth_steps    = 1;

        LOG(INFO, "Running a charge scan from Q = %d to Q = %d with VTh = %d", q_start, q_start+(q_step_size*N_q_steps), vth_start);

        vth_corr_start     = 0;
        vth_corr_step_size = 1;
        N_vth_corr_steps   = 1;

        vref_corr_start     = 90;
        vref_corr_step_size = 1;
        N_vref_corr_steps   = 1;

        if(asic_type ==  0){ sprintf(asicFolder, "output_EICROC0A");}
        if(asic_type == 10){ sprintf(asicFolder, "output_EICROC0");}
        sprintf(outputFolderName,"./%s/chargeScanData/", asicFolder);
    }

    int local_reg;

    if(asic_type == 10){

        LOG(INFO, "Using EICROC0 --> write the local registers...");

         for(u_int column = 0; column < 4; column++){
                for(u_int row = 0; row < 4; row++){

                    //LOG(INFO, "local_reg = 0x%X", (1 << (5+column)));

                    local_reg = (1 << (5+column)) + row*5; //starting local value for column
                    
                    local_reg = 0x0FFF & local_reg;
					
					LOG(INFO, "setting registers for pixel (%d, %d)", column, row);
					
					LOG(INFO, "local_reg = 0x%X", local_reg);
					LOG(INFO, "local_reg = 0x%X", local_reg+1);
					LOG(INFO, "local_reg = 0x%X", local_reg+2);
					LOG(INFO, "local_reg = 0x%X", local_reg+3);
					LOG(INFO, "local_reg = 0x%X", local_reg+4);
					
                    i2c_wr(local_reg,     0x80); //turns on discriminator, set Vth_corr to 0
                    i2c_wr(local_reg + 1, 0x00); //turns on integrator, sets Vref_corr to 0
                    i2c_wr(local_reg + 2, 0x84); //turns on debug outputs (PA, TRGOUT), turns on PA, turns on in_ctest
                    i2c_wr(local_reg + 3, 0x01); //turns on TDC
                    i2c_wr(local_reg + 4, 0x00); //doesn't mask any ADCs

                }
            }



    }   

 

    //default value
	//int Q_inj = 6; //in DACu, needs to be reversed to be applied correctly
    //Q_inj = 63 - Q_inj; //reverse logic for injected charge

    //int threshold = 350;
    

	for(int vth  = vth_start; vth < vth_start+(vth_step_size*N_vth_steps); vth+=vth_step_size){
	    for(int qinj = q_start; qinj < q_start+(q_step_size*N_q_steps); qinj+=q_step_size){
            for(int vthcor = vth_corr_start; vthcor < vth_corr_start+(vth_corr_step_size*N_vth_corr_steps); vthcor+=vth_corr_step_size){
            for(int vrefcor = vref_corr_start; vrefcor < vref_corr_start+(vref_corr_step_size*N_vref_corr_steps); vrefcor+=vref_corr_step_size){            
    
                LOG(INFO, "Setup for VTh = %d, Q = %d, vthcorr = %d, vrefcor = %d", vth, qinj, vthcor, vrefcor);
   
                char fname[256] ;

		        int tmp_LSB_actual_threshold = 0xFF & vth; //get the bottom 8 bits
		        int tmp_MSB_actual_threshold = vth >> 8; //get the top 2 bits
		
		        LOG(INFO, "check Vth math: %d ---> 0x%02X%02X \n", vth, tmp_MSB_actual_threshold, tmp_LSB_actual_threshold);		

		        int tmp_reg_high_vth_bits = current_reg_MSB_Vth & 0xFC; //preserve all bits except the lowest 2
		
		        LOG(INFO, "New reg 0x400B value = %02X \n", tmp_reg_high_vth_bits + tmp_MSB_actual_threshold);

		        //now let's deal with the DAC pulser value
		        int tmp_reg_high_DAC_pulser_bits = current_reg_DAC_pulser & 0xC0; //only the top two bits are not part of the DAC pulser number
		
		        LOG(INFO, "New reg 0x400C value = %02X \n", tmp_reg_high_DAC_pulser_bits + (63-qinj) );

                int localWrite = 0;                

                if(asic_type == 0) { // EICROC0A pre metal fix
		            i2c_wr(0x400A, tmp_LSB_actual_threshold);
		            i2c_wr(0x400B, tmp_reg_high_vth_bits + tmp_MSB_actual_threshold);
		            i2c_wr(0x400C, tmp_reg_high_DAC_pulser_bits + (63-qinj) );
		
		            localWrite = rewriteLocalRegisters();

                    i2c_wr(0x1, 0x80 + vthcor);
                    i2c_wr(0x2, vrefcor);

         
                    if(localWrite == 1){
		                sprintf(fname,"%svth_global_%d_q_inj_%d_vthcor_%d_vrefcor_%d.dta", outputFolderName, vth, qinj, vthcor, vrefcor);
		                FILE *f = fopen(fname,"w") ;
		                if(f==0) {
			                LOG(ERR,"%s: %s",fname,strerror(errno)) ;
			                break ;
		                }

		                LOG(TERR,"Running %d events into %s",num_events,fname) ;

		                run_asic(f, num_events) ;	// readout num_events and store to f
		                fclose(f) ;
                    }

                } //For ASIC EICROC0A pre metal fix

                if(asic_type == 10) {// EICROC0 chip

                    i2c_wr(0x20A, tmp_LSB_actual_threshold);
                    i2c_wr(0x20B, tmp_reg_high_vth_bits + tmp_MSB_actual_threshold);
                    i2c_wr(0x20C, tmp_reg_high_DAC_pulser_bits + (63-qinj) );

                    sprintf(fname,"%svth_global_%d_q_inj_%d_vthcor_%d_vrefcor_%d.dta", outputFolderName, vth, qinj, vthcor, vrefcor);
                    FILE *f = fopen(fname,"w") ;
                    if(f==0) {
                        LOG(ERR,"%s: %s",fname,strerror(errno)) ;
                        break ;
                    }

                    LOG(TERR,"Running %d events into %s",num_events,fname) ;

                    run_asic(f, num_events) ;   // readout num_events and store to f
                    fclose(f) ;
                }// EICROC0 chip


            }  //vrefcor loop
            } //vthcor loop
	    } //qinj loop
    } //vth loop




	return 0 ;
}


int rewriteLocalRegisters(){

    for(int i=0;i<reg_cou;i++) {
        if(regs[i].reg < 0x4000){ 
            LOG(INFO, "Re-writing local register...%02X --> %02X", regs[i].reg, regs[i].val);
			i2c_wr(regs[i].reg,regs[i].val);
		}
    }	

	return 1;
}

