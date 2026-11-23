#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>


#include <LOG/rtsLog.h>
volatile int rtsLogLevel = 2 ;	// WARN and above

#include <AXAU/eicroc_c.h>

extern int alex_zcu106(eicroc_c *eic) ;




/*************** Internal Helper ******************/
#if 0
static int sig_on ;
static void sighandler(int sig)
{
	if(sig==2) {
		LOG(WARN,"Cntrl-C: EXITING when oportune.") ;
	}
	else {
		LOG(ERR,"Signal %d: EXITING.",sig) ;
	}

	sig_on = sig ;
}
#endif

/* Split a line into argv-style tokens (in place).
   Supports 'single quotes', "double quotes" and backslash escapes. */
static char **split_args(char *s, int *argc_out)
{
    size_t cap = 8, n = 1;                 /* slot 0 is reserved for argv[0] */
    char **v = (char **)malloc(cap * sizeof *v);
    if (!v) return NULL;
    v[0] = NULL;

    char *r = s;
    for (;;) {
        while (isspace((unsigned char)*r)) r++;
        if (!*r) break;

        char *start = r, *w = r, quote = 0;
        for (; *r; r++) {
            if (quote) {
                if (*r == quote)                          quote = 0;
                else if (quote == '"' && *r == '\\' && r[1]) *w++ = *++r;
                else                                      *w++ = *r;
            } else if (*r == '\'' || *r == '"')           quote = *r;
            else if (*r == '\\' && r[1])                  *w++ = *++r;
            else if (isspace((unsigned char)*r)) { r++; break; }
            else                                          *w++ = *r;
        }
        *w = '\0';

        if (n + 2 > cap) {
            cap *= 2;
            char **tmp = (char **)realloc(v, cap * sizeof *v);
            if (!tmp) { free(v); return NULL; }
            v = tmp;
        }
        v[n++] = start;
    }
    v[n] = NULL;
    *argc_out = (int)n;
    return v;
}



int main(int argc, char *argv[])
{
	int c ;
	int ret ;
	int inetd = 0 ;

	rtsLogLevel = 2 ;	// default; 

//	signal(2,sighandler) ;	// TBD

	eicroc_c *eic = new eicroc_c ;	// get one instance...

	// defaults:

	eic->start_acq_to_cmd = 150 ;	//need to make it big to move away from the pedestal ring
	eic->cmd_to_end_acq = 4 ;	// reasonable number

	eic->config_fname = 0 ;		// will be set in asic_configure



	eic->run_type = 0 ;	// plain pedestal
	eic->lane_mask = 0x01 ;	// just the first lane



	eic->glo_threshold = 500 ;	// ~half range
	eic->pulser_0 = 63 ;		// maximum
	
	eic->vref = 0 ;			// per-pixel defaults
	eic->vthr = 0 ;			// per-pixel defaults

	eic->mode = 0x1 ;		// configure the ASICs
	eic->rdo_mode = 0x1 ;	// configure the board, yup

	eic->usb_dev = 0 ;	// direct access

	// my defaults
	eic->rdo_type = RDO_ZCU106 ;	
	eic->feb_type = FEB_OMEGA ;
	eic->asic_type = ASIC_EICROC1 ;

	eic->num_events = 1 ;

//	setvbuf(stdout, 0, _IOLBF, BUFSIZ) ;
//	setvbuf(stderr, 0, _IOLBF, BUFSIZ) ;

	LOG(INFO,"%s: compiled on %s %s",argv[0],__TIME__,__DATE__) ;	


	
	int use_argc = 1 ;
	char **use_argv ;

	if(argc==2 && strcmp(argv[1],"-s")==0) {
		LOG(INFO,"Enter parameters: ") ;
		fflush(stderr) ;

		char *line = 0 ;
		size_t cap = 0 ;
		if(getline(&line,&cap,stdin)<0) {
			LOG(ERR,"No arguments: exiting") ;
			return -1 ;
		}

		line[strlen(line)-1]=0 ;

		LOG(INFO,"Entered \"%s\"",line) ;
		fflush(stderr) ;

		int n ;
		char **v = split_args(line,&n) ;
		v[0] = argv[0] ;

		use_argc = n ;
		use_argv = v ;

		// this leaves some memory dangling but who cares
	}
	else {
		use_argc = argc ;
		use_argv = argv ;
	}


	while((c=getopt(use_argc, use_argv,":D:d:m:n:W:w:M:C:A:t:l:G:v:p:R:f:T:F:h?is")) != EOF) {
	switch(c) {
	case 'D' :
		rtsLogLevel = atoi(optarg) ;
		break ;
	case 'd' :
		eic->usb_dev = optarg ;
		break ;
	case 'm' :
		if(sscanf(optarg,"0x%X",&eic->mode)<=0) {
			eic->mode = atoi(optarg) ;
		}
		break ;
	case 'n' :
		eic->num_events = atoi(optarg) ;
		break ;
	case 'M' :
		if((ret=sscanf(optarg,"0x%X",&eic->rdo_mode))<=0) {
			eic->rdo_mode = atoi(optarg) ;
		}
		break ;
	case 'W' :
		eic->start_acq_to_cmd = atoi(optarg) ;
		break ;
	case 'w' :
		eic->cmd_to_end_acq = atoi(optarg) ;
		break ;
	case 'C' :
		eic->config_fname = optarg ;
		break ;
	case 'A' :
		eic->asic_type = atoi(optarg) ;
		break ;
	case 't' :
		eic->run_type = atoi(optarg) ;
		break ;
	case 'l' :
		if(sscanf(optarg,"0x%X",&eic->lane_mask)<=0) {
			eic->lane_mask = atoi(optarg) ;
		}
		break ;
	case 'G' :
		eic->glo_threshold = atoi(optarg) ;
		break ;
	case 'v' :	// vref per pixel; for all
		eic->vref = atoi(optarg) ;
		break ;
	case 'p' :	// 6 bit pulser
		eic->pulser_0 = atoi(optarg) ;
		break ;
	case 'R' :
		eic->rdo_type = atoi(optarg) ;
		break ;
	case 'F' :
		eic->feb_type = atoi(optarg) ;
		break ;
	case 'f' :
		if(sscanf(optarg,"0x%X",&eic->fmt_type)<=0) {
			eic->fmt_type = atoi(optarg) ;
		}
		break ;
	case 'T' :	// threshold per pixel; common to all
		eic->vthr = atoi(optarg) ;
		break ;
	case 's' :
		inetd = 1 ;
		LOG(WARN,"Running under inetd -- not implemented") ;
		printf("And this to stdout %d\n",inetd) ;
		return -1 ;
	case 'h' :
		LOG(ERR,"There is no help available. RTFM. Goodbye.") ;
		return -1 ;
	case '?' :
		LOG(ERR,"Option -%c is unknown. RTFM. Goodbye.",optopt) ;
		return -1 ;
	case ':' :
		LOG(ERR,"Option -%c requires and argument. Goodbye.",optopt) ;
		return -1 ;
	default :
		LOG(ERR,"Unknown parameter flag %c",c) ;
		break ;
	}
	}





	ret = eic->init() ;			// ALWAYS first
	
	ret = eic->rdo_status() ;		// shows status of something, if it knows


	// board config using rdo_mode bits
	// NOTE:
	//	rdo_mode & 4 -- run reset FIRST
	//	rdo_mode & 1 -- run full bd configure
	//	rdo_mode & 8 -- print board registers or other dbg info

	if(eic->rdo_mode & 8) {
		eic->rdo_reg_dump() ;	// debugging
	}

	if(eic->rdo_mode & 4) {
		eic->rdo_reset() ;	// e.g. soft reset lpGBT
	}

	if(eic->rdo_mode & 1) {		// RDO's own registers setup
		eic->rdo_configure() ;
	}
	else {
		LOG(WARN,"You are skipping RDO %s configuration. NOT a good idea.",eic->rdo2c()) ;
	}

	// Up to this line I configured the RDO
	// Now I configure the ASIC itself via I2C
	
	// EICROC specific modes below
	// NOTE:	mode & 4	-- reset ASIC FIRST
	//		mode & 1	-- full reconfigure
	//		mode & 8	-- I2C reg dump, debuging

	if(eic->mode & 8) {
		eic->asic_reg_dump() ;
	}

	if(eic->mode & 4) {
		eic->asic_reset() ;	// e.g. pulse RESETB
	}

	if(eic->mode & 1) {
		eic->asic_configure() ;
	}
	else {
		LOG(WARN,"You are skipping ASIC %s configuration. Running with old values.",eic->asic2c()) ;
	}

	// user's overrides come last but before run-start
	if((eic->mode & 0x100)==0x100) {	// Tonko's tests
		LOG(WARN,"Calling test code") ;

		// nothing...
	}

	if((eic->mode & 0x200)==0x200) {	// Alex
		LOG(WARN,"Calling Alex code -- not yet implemented") ;
//		if(alex_zcu106(eic)<0) {
//			LOG(INFO,"alex_zcu requests termination. Bye.") ;
//			return 0 ;
//		}
	}


	// And now I start a run, if requested
	if(eic->mode & 2) {
		FILE *f = stdout ;	// for now
		ret = eic->run_start(f,eic->num_events) ;
	}

	LOG(INFO,"Done. Exiting.") ;


	return 0;
}

