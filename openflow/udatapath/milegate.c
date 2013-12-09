 /*
 * Copyright (c) 2013 PSNC
 * Autors: Szymon Kuc, Damian Parniewicz
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include "milegate.h"

#include "vlog.h"
#define THIS_MODULE VLM_datapath

static struct termios orig_tios;
static telnet_t *telnet;
static int do_echo;

static const telnet_telopt_t telopts[] = {
	{ TELNET_TELOPT_ECHO,		    TELNET_WONT, TELNET_DO   },
	{ TELNET_TELOPT_TTYPE,		TELNET_WILL, TELNET_DONT },
	{ TELNET_TELOPT_COMPRESS2,	TELNET_WONT, TELNET_DO   },
	{ TELNET_TELOPT_MSSP,		    TELNET_WONT, TELNET_DO   },
	{ -1, 0, 0 }
};

struct milegate_properties milegate_props;

char port_number[100] = {0};
unsigned short c_tag = 0;
unsigned short interfaceID = 0;
unsigned short serviceID = 0;
int com_counter = 0;
char print_buffor[4086];
char* print_buffor_iterator;


extern const char ports_names[MAX_PORTS_NAMES][MAX_PORT_NAME_LEN];
extern char *hardware_ip;
extern char *hardware_user;
extern char *hardware_password;

extern char mfr_desc[DESC_STR_LEN];
extern char hw_desc[DESC_STR_LEN];
extern char sw_desc[DESC_STR_LEN];
extern char dp_desc[DESC_STR_LEN];
extern char serial_num[SERIAL_NUM_LEN];

// CODE could not be published