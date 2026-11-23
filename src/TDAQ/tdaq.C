#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>


#include <LOG/rtsLog.h>
volatile int rtsLogLevel = 2 ;

#define DEFAULT_HOST "peta1"
#define DEFAULT_PORT "19000"
#define RCVBUF_SIZE  (4 * 1024 * 1024)   /* requested kernel receive buffer */
#define STDIO_BUF    (256 * 1024)        /* stdio buffer for the read side */

struct conn {
    int   fd;      /* the socket itself, read/write */
    FILE *in;      /* read side  (fdopen of fd)      */
    FILE *out;     /* write side (fdopen of dup(fd)) */
};

/* Split "host:port" into host and port. Missing parts get the defaults.
   Accepts: host, host:port, :port, [v6addr], [v6addr]:port.
   A bare IPv6 literal (several colons, no brackets) is taken as host only. */
static int parse_hostport(const char *arg, char *host, size_t hsz,
                          char *port, size_t psz)
{
    const char *h = DEFAULT_HOST, *p = DEFAULT_PORT;
    size_t hlen = strlen(DEFAULT_HOST), plen = strlen(DEFAULT_PORT);

    if (arg && *arg) {
        const char *colon = NULL;

        if (arg[0] == '[') {
            const char *close = strchr(arg, ']');
            if (!close) return -1;
            h = arg + 1;
            hlen = (size_t)(close - h);
            if (close[1] == ':') colon = close + 1;
            else if (close[1] != '\0') return -1;
        } else {
            const char *first = strchr(arg, ':');
            if (first && first == strrchr(arg, ':'))
                colon = first;                 /* exactly one colon */
            h = arg;
            hlen = colon ? (size_t)(colon - arg) : strlen(arg);
        }

        if (hlen == 0) { h = DEFAULT_HOST; hlen = strlen(DEFAULT_HOST); }
        if (colon && colon[1]) { p = colon + 1; plen = strlen(p); }
    }

    if (hlen >= hsz || plen >= psz) return -1;
    memcpy(host, h, hlen); host[hlen] = '\0';
    memcpy(port, p, plen); port[plen] = '\0';
    return 0;
}

/* Connect to host:port. On success fills *c and returns 0, else -1. */
static int conn_open(struct conn *c, const char *host, const char *port)
{
    struct addrinfo hints, *res, *ai;
    int fd = -1, rc;

    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    rc = getaddrinfo(host, port, &hints, &res);
    if (rc) {
        LOG(ERR, "%s:%s: %s\n", host, port, gai_strerror(rc));
        return -1;
    }

    for (ai = res; ai; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0) continue;

        /* Must be set BEFORE connect(): the TCP window scale factor is
           negotiated in the SYN, so a later setsockopt can't raise the
           advertised window beyond what was agreed. */
        int sz = RCVBUF_SIZE;
        if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &sz, sizeof sz) < 0)
            perror("setsockopt(SO_RCVBUF)");

        if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0)
            break;

        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);

    if (fd < 0) {
        LOG(ERR, "cannot connect to %s:%s: %s",
                host, port, strerror(errno));
        return -1;
    }

    /* Linux silently caps the request at net.core.rmem_max and reports
       double the usable value, so show what we really got. */
    int got = 0;
    socklen_t len = sizeof got;
    if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &got, &len) == 0)
        LOG(NOTE, "receive buffer: requested %d, got %d",
                RCVBUF_SIZE, got);

    /* Two FILE streams over the same socket: one for reading, one for
       writing. This avoids the fflush/fseek dance that a single "r+"
       stream would need between reads and writes. */
    int wfd = dup(fd);
    if (wfd < 0) { perror("dup"); close(fd); return -1; }

    c->fd  = fd;
    c->in  = fdopen(fd,  "r");
    c->out = fdopen(wfd, "w");
    if (!c->in || !c->out) {
        LOG(ERR,"fdopen [%s]",strerror(errno));
        if (c->in)  fclose(c->in);  else close(fd);
        if (c->out) fclose(c->out); else close(wfd);
        return -1;
    }

    setvbuf(c->in, NULL, _IOFBF, STDIO_BUF);   /* big reads, fewer syscalls */
    return 0;
}

static void conn_close(struct conn *c)
{
    fflush(c->out);
    fclose(c->out);    /* closes the dup'ed descriptor */
    fclose(c->in);     /* closes the socket; connection ends here */
    c->in = c->out = NULL;
    c->fd = -1;
}

int main(int argc, char **argv)
{
	char host[256], port[16];
	const char *hp = NULL;
	int opt;

	while ((opt = getopt(argc, argv, "+H:")) != -1) {
	switch (opt) {
        case 'H': 
		hp = optarg; 
		break;
        default:
		LOG(ERR,"usage: %s [-H host[:port]] other flags", argv[0]);
		return 2;
        }
	}

	
	// get the host and port from -H
	if (parse_hostport(hp, host, sizeof host, port, sizeof port) < 0) {
		LOG(ERR,"bad -H argument: %s", hp ? hp : "");
		return 2;
	}

	signal(SIGPIPE, SIG_IGN);   /* get EPIPE from writes instead of dying */

	// connect
	struct conn c;
	if (conn_open(&c, host, port) < 0) {
		LOG(ERR,"connection failed to %s:%s [%s]",host,port,strerror(errno)) ;
		return 1;
	}


	
	LOG(INFO,"connected to %s:%s", host, port);


	// concatenate all the parameters after "--" into 1 string and pass it to the remote eicroc
	char cmd[256] ;
	cmd[0] = 0 ;
	for(int i=optind;i<argc;i++) {
		strcat(cmd,argv[i]) ;
		strcat(cmd," ") ;
	}

	if(cmd[0]==0) {	// oh, well... use defaults
		fprintf(c.out, "-R 1 -A 0 -F 0 -m 1\n");
	}
	else {
		fprintf(c.out, "%s\n",cmd);
	}

	fflush(c.out);	// goes to the remote gost...

	char buf[65536];
	
	// and now start reading...
	while(!feof(c.in)) {
		if(fgets(buf,sizeof(buf),c.in)==0) {
			LOG(NOTE,"Read 0?") ;
			continue ;
		}

		if(strstr(buf," [line ")) {		// signature of the LOG
			fprintf(stderr,"%s",buf) ;
		}
		else if(strstr(buf,"STARTING BINARY")) {	// for later use
			LOG(INFO,"Binary starting") ;
		}
		else {
			printf("%s",buf) ;		// data to stdout
		}
	}

#if 0
	// this will be used for binary data later on...
	size_t n;
	while ((n = fread(buf, 1, sizeof buf, c.in)) > 0) {
		LOG(TERR,"Read %d bytes",n) ;
	}

#endif

	conn_close(&c);

	LOG(INFO,"Done.") ;
	return 0;
}
