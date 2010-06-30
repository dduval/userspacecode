/*
 * Copyright (C) 2009 Dominic Duval, Red Hat Inc.
 *
 * multidump :
 * Receive multicast messages based on command line arguments
 *
 * USAGE: ./multidump
 *
 * EXAMPLE: ./mdump -p0 -Q2 -r200000 224.0.55.55 12965
 *
 * Inspired from 29West.com's msend and mdump
 */



#include <stdio.h>
#include <stdlib.h>

extern int optind;
extern int optreset;
extern char *optarg;
int getopt(int nargc, char * const *nargv, const char *ostr);

#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#define SLEEP_SEC(s) sleep(s)
#define SLEEP_MSEC(s) usleep((s) * 1000)
#define SLEEP_USEC(s) usleep(s)
#define CLOSESOCKET close
#define ERRNO errno
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define TLONGLONG signed long long

#include <sys/time.h>

#include <string.h>
#include <time.h>

#define MAXPDU 65536


/* program name (from argv[0] */
char *prog_name = "xxx";

/* program options */
int o_quiet_lvl;
int o_rcvbuf_size;
int o_pause_ms;
int o_verify;
int o_stop;
int o_log;

/* program positional parameters */
unsigned long int groupaddr;
unsigned short int groupport;
char *bind_if;


char usage_str[] = "[-h] [-q] [-Q Quiet_lvl] [-r rcvbuf_size] [-p pause_ms] [-v] [-s] group port [interface]";

void usage(char *msg)
{
	if (msg != NULL)
		fprintf(stderr, "\n%s\n\n", msg);
	fprintf(stderr, "Usage: %s %s\n\n"
			"(use -h for detailed help)\n",
			prog_name, usage_str);
}  /* usage */


void help(char *msg)
{
	if (msg != NULL)
		fprintf(stderr, "\n%s\n\n", msg);
	fprintf(stderr, "Usage: %s %s\n", prog_name, usage_str);
	fprintf(stderr, "Where:\n"
			"  -h : help\n"
			"  -q : no print per datagram (same as '-Q 2')\n"
			"  -Q Quiet_lvl : set quiet level [0] :\n"
			"                 0 - print full datagram contents\n"
			"                 1 - print datagram summaries\n"
			"                 2 - no print per datagram (same as '-q')\n"
			"  -r rcvbuf_size : size (bytes) of UDP receive buffer (SO_RCVBUF) [4194304]\n"
			"                   (use 0 for system default buff size)\n"
			"  -p pause_ms : milliseconds to pause after each receive [0 : no pause]\n"
			"  -v : verify the sequence numbers\n"
			"  -s : stop execution when status msg received\n"
			"  -l : log packets\n"
			"\n"
			"  group : multicast address to send on\n"
			"  port : destination port\n"
			"  interface : optional IP addr of local interface (for multi-homed hosts) [INADDR_ANY]\n"
	);
}  /* help */


/* faster routine to replace inet_ntoa() (from tcpdump) */
char *intoa(unsigned int addr)
{
	register char *cp;
	register unsigned int byte;
	register int n;
	static char buf[sizeof(".xxx.xxx.xxx.xxx")];

	addr = ntohl(addr);
	// NTOHL(addr);
	cp = &buf[sizeof buf];
	*--cp = '\0';

	n = 4;
	do {
		byte = addr & 0xff;
		*--cp = byte % 10 + '0';
		byte /= 10;
		if (byte > 0) {
			*--cp = byte % 10 + '0';
			byte /= 10;
			if (byte > 0)
				*--cp = byte + '0';
		}
		*--cp = '.';
		addr >>= 8;
	} while (--n > 0);

	return cp + 1;
}  /* intoa */


char *format_time(const struct timeval *tv)
{
	static char buff[sizeof(".xx:xx:xx.xxxxxx")];
	int min;

	unsigned int h = localtime((time_t *)&tv->tv_sec)->tm_hour;
	min = (int)(tv->tv_sec % 86400);
	sprintf(buff,"%02d:%02d:%02d.%06d",h,(min%3600)/60,min%60,tv->tv_usec);
	return buff;
}  /* format_time */


void dump(const char *buffer, int size)
{
	int i,j;
	unsigned char c;
	char textver[20];

	for (i=0;i<(size >> 4);i++) {
		for (j=0;j<16;j++) {
			c = buffer[(i << 4)+j];
			printf("%02x ",c);
			textver[j] = ((c<0x20)||(c>0x7e))?'.':c;
		}
		textver[j] = 0;
		printf("\t%s\n",textver);
	}
	for (i=0;i<size%16;i++) {
		c = buffer[size-size%16+i];
		printf("%02x ",c);
		textver[i] = ((c<0x20)||(c>0x7e))?'.':c;
	}
	for (i=size%16;i<16;i++) {
		printf("   ");
		textver[i] = ' ';
	}
	textver[i] = 0;
	printf("\t%s\n",textver);
	fflush(stdout);
}  /* dump */


void currenttv(struct timeval *tv)
{
	gettimeofday(tv,NULL);
}  /* currenttv */


int main(int argc, char **argv)
{
	int opt;
	int num_parms;
	char equiv_cmd[1024];
	char *buff;
	SOCKET sock;
	socklen_t fromlen = sizeof(struct sockaddr_in);
	int default_rcvbuf_sz, cur_size, sz;
	int num_rcvd;
	struct sockaddr_in name;
	struct sockaddr_in src;
	struct ip_mreq imr;
	struct timeval tv;
	int num_sent;
	float perc_loss;
	int cur_seq;
	struct iovec iov;
	ssize_t iovnr;
	int iovfd;
	char strbuffer[1000];

	prog_name = argv[0];


	if (o_log == 1) {
		iov.iov_base=strbuffer;
		iov.iov_len=0; //To be defined latter.
		iovfd= open ("log.out", O_WRONLY | O_CREAT | O_TRUNC);
		if (iovfd == -1) {
			perror ("open error");
			exit(1);
		}
	}


	buff = malloc(65536 + 1);  /* one extra for trailing null (if needed) */
	if (buff == NULL) { fprintf(stderr, "malloc failed\n"); exit(1); }

	signal(SIGPIPE, SIG_IGN);

	if((sock = socket(PF_INET,SOCK_DGRAM,0)) == INVALID_SOCKET) {
		fprintf(stderr, "ERROR: ");  perror("socket");
		exit(1);
	}
	sz = sizeof(default_rcvbuf_sz);
	if (getsockopt(sock,SOL_SOCKET,SO_RCVBUF,(char *)&default_rcvbuf_sz,
			(unsigned int *)&sz) == SOCKET_ERROR) {
		fprintf(stderr, "ERROR: ");  perror("getsockopt - SO_RCVBUF");
		exit(1);
	}

	/* default values for options */
	o_quiet_lvl = 0;
	o_rcvbuf_size = 0x400000;  /* 4MB */
	o_pause_ms = 0;
	o_verify = 0;
	o_stop = 0;
	o_log = 0;

	/* default values for optional positional params */
	bind_if = NULL;

	while ((opt = getopt(argc, argv, "hqQ:p:r:vsl")) != EOF) {
		switch (opt) {
		  case 'h':
			help(NULL);  exit(0);
			break;
		  case 'q':
			o_quiet_lvl = 2;
			break;
		  case 'Q':
			o_quiet_lvl = atoi(optarg);
			break;
		  case 'p':
			o_pause_ms = atoi(optarg);
			break;
		  case 'r':
			o_rcvbuf_size = atoi(optarg);
			if (o_rcvbuf_size == 0)
				o_rcvbuf_size = default_rcvbuf_sz;
			break;
		  case 'v':
			o_verify = 1;
			break;
		  case 's':
			o_stop = 1;
			break;
		  case 'l':
			o_log = 1;
		  default:
			usage("unrecognized option");
			exit(1);
			break;
		}  /* switch */
	}  /* while opt */

	num_parms = argc - optind;
	
	if (o_log == 1) {
		iov.iov_base=strbuffer;
		iov.iov_len=0; //To be defined latter.
		iovfd= open ("log.out", O_WRONLY | O_CREAT | O_TRUNC);
		if (iovfd == -1) {
			perror ("open error");
			exit(1);
		}
	}


	/* handle positional parameters */
	if (num_parms == 2) {
		groupaddr = inet_addr(argv[optind]);
		groupport = (unsigned short)atoi(argv[optind+1]);
		sprintf(equiv_cmd, "mdump -p%d -Q%d -r%d %s%s%s %s",
				o_pause_ms, o_quiet_lvl, o_rcvbuf_size,
				o_verify ? "-v " : "",
				o_stop ? "-s " : "",
				argv[optind],argv[optind+1]);
		printf("Equiv cmd line: %s\n", equiv_cmd);
		fflush(stdout);
		fprintf(stderr, "Equiv cmd line: %s\n", equiv_cmd);
		fflush(stderr);
	} else if (num_parms == 3) {
		groupaddr = inet_addr(argv[optind]);
		groupport = (unsigned short)atoi(argv[optind+1]);
		bind_if  = argv[optind+2];
		sprintf(equiv_cmd, "mdump -p%d -Q%d -r%d %s%s %s %s",
				o_pause_ms, o_quiet_lvl, o_rcvbuf_size,
				o_verify ? "-v " : "",
				argv[optind],argv[optind+1],argv[optind+2]);
		printf("Equiv cmd line: %s\n", equiv_cmd);
		fflush(stdout);
		fprintf(stderr, "Equiv cmd line: %s\n", equiv_cmd);
		fflush(stderr);
	} else {
		usage("need 2-3 positional parameters");
		exit(1);
	}

	if(setsockopt(sock,SOL_SOCKET,SO_RCVBUF,(const char *)&o_rcvbuf_size,
			sizeof(o_rcvbuf_size)) == SOCKET_ERROR) {
		printf("WARNING: setsockopt - SO_RCVBUF\n");
		fflush(stdout);
		fprintf(stderr, "WARNING: "); perror("setsockopt - SO_RCVBUF");
		fflush(stderr);
	}
	sz = sizeof(cur_size);
	if (getsockopt(sock,SOL_SOCKET,SO_RCVBUF,(char *)&cur_size,
			(unsigned int *)&sz) == SOCKET_ERROR) {
		fprintf(stderr, "ERROR: ");  perror("getsockopt - SO_RCVBUF");
		exit(1);
	}
	if (cur_size < o_rcvbuf_size) {
		printf("WARNING: tried to set SO_RCVBUF to %d, only got %d\n",
				o_rcvbuf_size, cur_size);
		fflush(stdout);
		fprintf(stderr, "WARNING: tried to set SO_RCVBUF to %d, only got %d\n",
				o_rcvbuf_size, cur_size);
		fflush(stderr);
	}

	memset((char *)&imr,0,sizeof(imr));
	imr.imr_multiaddr.s_addr = groupaddr;
	if (bind_if != NULL) {
		imr.imr_interface.s_addr = inet_addr(bind_if);
	} else {
		imr.imr_interface.s_addr = htonl(INADDR_ANY);
	}

	opt = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == SOCKET_ERROR) {
		fprintf(stderr, "ERROR: ");  perror("setsockopt SO_REUSEADDR");
		exit(1);
	}

	memset((char *)&name,0,sizeof(name));
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = groupaddr;
	name.sin_port = htons(groupport);
	if (bind(sock,(struct sockaddr *)&name,sizeof(name)) == SOCKET_ERROR) {
		name.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(sock,(struct sockaddr *)&name, sizeof(name)) == -1) {
			fprintf(stderr, "ERROR: ");  perror("bind");
			exit(1);
		}
	}

	if (setsockopt(sock,IPPROTO_IP,IP_ADD_MEMBERSHIP,
				(char *)&imr,sizeof(struct ip_mreq)) == SOCKET_ERROR ) {
		fprintf(stderr, "ERROR: ");  perror("setsockopt - IP_ADD_MEMBERSHIP");
		exit(1);
	}

	cur_seq = 0;
	num_rcvd = 0;
	for (;;) {
		cur_size = recvfrom(sock,buff,65536,0,
				(struct sockaddr *)&src,&fromlen);
		if (cur_size == SOCKET_ERROR) {
			fprintf(stderr, "ERROR: ");  perror("recv");
			exit(1);
		}

		if (o_log == 1 ) {
			currenttv(&tv);
			sprintf(strbuffer, "%s %s.%d %d bytes:\n",
				format_time(&tv), inet_ntoa(src.sin_addr),
				ntohs(src.sin_port), cur_size);
			
		}

		if (o_quiet_lvl == 0) {  /* non-quiet: print full dump */
			currenttv(&tv);
			printf("%s %s.%d %d bytes:\n",
					format_time(&tv), inet_ntoa(src.sin_addr),
					ntohs(src.sin_port), cur_size);
			dump(buff,cur_size);
		}
		if (o_quiet_lvl == 1) {  /* semi-quiet: print datagram summary */
			currenttv(&tv);
			printf("%s %s.%d %d bytes\n",  /* no colon */
					format_time(&tv), inet_ntoa(src.sin_addr),
					ntohs(src.sin_port), cur_size);
			fflush(stdout);
		}

		if (o_pause_ms > 0) {
			SLEEP_USEC(o_pause_ms);
		}

		if (cur_size > 5 && memcmp(buff, "echo ", 5) == 0) {
			/* echo command */
			buff[cur_size] = '\0';  /* guarantee trailing null */
			if (buff[cur_size - 1] == '\n')
				buff[cur_size - 1] = '\0';  /* strip trailing nl */
			printf("%s\n", buff);
			fflush(stdout);
			fprintf(stderr, "%s\n", buff);
			fflush(stderr);
		}
		else if (cur_size > 5 && memcmp(buff, "stat ", 5) == 0) {
			/* when sender tells us to, calc and print stats */
			buff[cur_size] = '\0';  /* guarantee trailing null */
			/* 'stat' message contains num msgs sent */
			num_sent = atoi(&buff[5]);
			perc_loss = (float)(num_sent - num_rcvd) * 100.0 / (float)num_sent;
			printf("%d msgs sent, %d received (not including 'stat')\n",
					num_sent, num_rcvd);
			printf("%f%% loss\n", perc_loss);
			fflush(stdout);
			fprintf(stderr, "%d msgs sent, %d received (not including 'stat')\n",
					num_sent, num_rcvd);
			fprintf(stderr, "%f%% loss\n", perc_loss);
			fflush(stderr);

			if (o_stop)
				exit(0);

			/* reset stats */
			num_rcvd = 0;
			cur_seq = 0;
		}
		else {  /* not a cmd */
			if (o_verify) {
				buff[cur_size] = '\0';  /* guarantee trailing null */
				if (cur_seq != strtol(&buff[8], NULL, 16)) {
					printf("Expected seq %x (hex), got %s\n", cur_seq, &buff[8]);
					fflush(stdout);
					/* resyncronize sequence numbers in case there is loss */
					cur_seq = strtol(&buff[8], NULL, 16);
				}
			}

			++num_rcvd;
			++cur_seq;
		}
	}  /* for ;; */

	CLOSESOCKET(sock);

	exit(0);
}  /* main */



