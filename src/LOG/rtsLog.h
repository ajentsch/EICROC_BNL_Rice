#ifndef _RTS_LOG_H_
#define _RTS_LOG_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* colored output, :-) */
#define ANSI_GREEN      "\033[32m"
#define ANSI_RED        "\033[31m"
#define ANSI_BLUE       "\033[34m"
#define ANSI_YELLOW     "\033[33m"
#define ANSI_MAGENTA    "\033[35m"
#define ANSI_CYAN       "\033[36m"
#define ANSI_BOLD       "\033[1m"
#define ANSI_ITALIC     "\033[3m"
#define ANSI_UNDERLINE  "\033[4m"
#define ANSI_REVERSE    "\033[7m"
#define ANSI_RESET      "\033[0m"

#define CRIT    "CRITICAL"      /* unmasked (5) */
#define OPER    "OPERATOR"      /* 4 */
#define ERR     "ERROR"         /* 3 */
#define WARN    "WARNING"       /* 2 */
#define NOTE    "NOTICE"        /* 1 */
#define DBG     "DEBUG"         /* 0 */

#define TERR	"Tonko"
#define INFO	"INFO"

extern volatile int tonkoLogLevel ;

#define LOG(SEV,STRING,ARGS...) \
        do { \
                const char *const yada = SEV ; \
                if((*yada == 'E')) { \
                        fprintf(stderr,"" ANSI_RED "RTS_" SEV ": " __FILE__ " [line %d]: " STRING "" ANSI_RESET "\n" , __LINE__ , ##ARGS) ;\
		} \
                else if((*yada == 'C')) { \
                        fprintf(stderr,"" ANSI_RED "" ANSI_REVERSE "RTS_" SEV ": " __FILE__ " [line %d]: " STRING "" ANSI_RESET "\n" , __LINE__ , ##ARGS) ;\
		} \
                else if((*yada == 'I')) { \
                        fprintf(stderr,"" ANSI_BLUE "RTS_" SEV ": " __FILE__ " [line %d]: " STRING "" ANSI_RESET "\n" , __LINE__ , ##ARGS) ;\
		} \
                else if((*yada == 'T')) { \
                        fprintf(stderr,"" ANSI_GREEN "RTS_" SEV ": " __FILE__ " [line %d]: " STRING "" ANSI_RESET "\n" , __LINE__ , ##ARGS) ;\
		} \
                else if((*yada == 'W')) { \
                        fprintf(stderr,"" ANSI_CYAN "RTS_" SEV ": " __FILE__ " [line %d]: " STRING "" ANSI_RESET "\n" , __LINE__ , ##ARGS) ;\
		} \
                else if((*yada == 'O')) { \
                        fprintf(stderr,"" ANSI_BLUE "" ANSI_REVERSE "RTS_" SEV ": " __FILE__ " [line %d]: " STRING "" ANSI_RESET "\n" , __LINE__ , ##ARGS) ;\
		} \
                else if((*yada == 'N') && (tonkoLogLevel<2)) { \
                        fprintf(stderr,"" ANSI_ITALIC "RTS_" SEV ": " __FILE__ " [line %d]: " STRING "" ANSI_RESET "\n" , __LINE__ , ##ARGS) ;\
		} \
                else if((*yada == 'D') && (tonkoLogLevel<1)) { \
                        fprintf(stderr,"" ANSI_ITALIC "RTS_" SEV ": " __FILE__ " [line %d]: " STRING "" ANSI_RESET "\n" , __LINE__ , ##ARGS) ;\
		} \
	} while(0) \

#ifdef __cplusplus
}
#endif

#endif  /* _RTS_LOG_H */
