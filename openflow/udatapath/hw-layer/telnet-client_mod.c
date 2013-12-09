/*
 * Sean Middleditch
 * sean@sourcemud.org
 *
 * The author or authors of this code dedicate any and all copyright interest
 * in this code to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and successors. We
 * intend this dedication to be an overt act of relinquishment in perpetuity of
 * all present and future rights to this code under copyright law.
 */

#if !defined(_POSIX_SOURCE)
#	define _POSIX_SOURCE
#endif
#if !defined(_BSD_SOURCE)
#	define _BSD_SOURCE
#endif

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

#ifdef HAVE_ZLIB
#include "zlib.h"
#endif


//#include "../milegate.h";
#include "libtelnet.h"

static struct termios orig_tios;
static telnet_t *telnet;
static int do_echo;

char *login = "manager";
char *passwd = "";
char *ip_addr = "195.8.104.196";
char *telnet_port = "23";

char port_number[100] = {0};
unsigned short c_tag;
unsigned short s_tag;
unsigned short interfaceID;
unsigned short serviceID;
int com_counter = 0;
int end_session = 0;

static const telnet_telopt_t telopts[] = {
	{ TELNET_TELOPT_ECHO,		TELNET_WONT, TELNET_DO   },
	{ TELNET_TELOPT_TTYPE,		TELNET_WILL, TELNET_DONT },
	{ TELNET_TELOPT_COMPRESS2,	TELNET_WONT, TELNET_DO   },
	{ TELNET_TELOPT_MSSP,		TELNET_WONT, TELNET_DO   },
	{ -1, 0, 0 }
};

static void _cleanup(void) {
	tcsetattr(STDOUT_FILENO, TCSADRAIN, &orig_tios);
}

static void _input(char *buffer, int size) {
	static char crlf[] = { '\r', '\n' };
	int i;

	for (i = 0; i != size; ++i) {
		/* if we got a CR or LF, replace with CRLF
		 * NOTE that usually you'd get a CR in UNIX, but in raw
		 * mode we get LF instead (not sure why)
		 */
		/*if (buffer[i] == '\r' || buffer[i] == '\n') {
			if (do_echo)
				printf("\r\n");
			telnet_send(telnet, crlf, 2);
		} else {*/
			if (do_echo)
				putchar(buffer[i]);
			telnet_send(telnet, buffer + i, 1);
		//}
	}
	fflush(stdout);
}

static void _send(int sock, const char *buffer, size_t size) {
	int rs;

	/* send data */
	while (size > 0) {
		if ((rs = send(sock, buffer, size, 0)) == -1) {
			fprintf(stderr, "send() failed: %s\n", strerror(errno));
			exit(1);
		} else if (rs == 0) {
			fprintf(stderr, "send() unexpectedly returned 0\n");
			exit(1);
		}

		/* update pointer and size to see if we've got more to send */
		buffer += rs;
		size -= rs;
	}
}

void process_input_data1(const char *buffer, size_t size)
{
    //fprintf(stderr, "***Wchodze do process input data.\n");
    char command[1000];
    if (strstr(buffer, "login as:") != NULL){
        _input(login, strlen(login));
        _input("\r\n", 2);
    } else if (strstr(buffer, "password:") != NULL){
        _input(passwd, strlen(passwd));
        _input("\r\n", 2);
    } else if (strstr(buffer, ">") != NULL){
        //fprintf(stderr, "jestem za />\n");
        switch(com_counter) {
            case 0:
                snprintf(command, sizeof command, "cd /%s/cfgm", port_number);
                _input(command, strlen(command));
                _input("\r\n", 2);
                com_counter++;
                break;
            case 1:
                _input("set ClassificationKey Vlan", strlen("set ClassificationKey Vlan"));
                _input("\r\n", 2);
                com_counter++;
                break;
            case 2:
                snprintf(command, sizeof command, "CreateInterface VLAN_C_TAG%d", c_tag);
                _input(command, strlen(command));
                _input("\r\n", 2);
                com_counter++;
                break;
            case 3:
                _input("cd /services/packet/tls/cfgm", strlen("cd /services/packet/tls/cfgm"));
                _input("\r\n", 2);
                com_counter++;
                break;
            case 4:
                snprintf(command, sizeof command, "CreateService /%s/interface-%d %d Add Assigned CoS0 default", port_number, interfaceID, s_tag);
                _input(command, strlen(command));
                _input("\r\n", 2);
                com_counter++;
                break;
            case 5:
                end_session = 1;
                /*_input("exit", strlen("exit"));
                _input("\r\n", 2);
                com_counter++;
                break;*/
        }
    }
}

void process_input_data2(const char *buffer, size_t size)
{
    //fprintf(stderr, "***Wchodze do process input data.\n");
    char command[1000];
    if (strstr(buffer, "login as:") != NULL){
        _input(login, strlen(login));
        _input("\r\n", 2);
    } else if (strstr(buffer, "password:") != NULL){
        _input(passwd, strlen(passwd));
        _input("\r\n", 2);
    } else if (strstr(buffer, ">") != NULL){
        //fprintf(stderr, "jestem za />\n");
        switch(com_counter) {
            case 0:
                _input("cd /services/packet/tls/cfgm", strlen("cd /services/packet/tls/cfgm"));
                _input("\r\n", 2);
                com_counter++;
                break;
            case 1:
                snprintf(command, sizeof command, "DeleteService %d", serviceID);
                _input(command, strlen(command));
                _input("\r\n", 2);
                com_counter++;
                break;
            case 2:
                snprintf(command, sizeof command, "cd /%s/cfgm", port_number);
                _input(command, strlen(command));
                _input("\r\n", 2);
                com_counter++;
                break;
            case 3:
                snprintf(command, sizeof command, "DeleteInterface interface-%d", interfaceID);
                _input(command, strlen(command));
                _input("\r\n", 2);
                com_counter++;
                end_session = 1;
                break;
            case 4:
                end_session = 1;
                /*_input("exit", strlen("exit"));
                _input("\r\n", 2);
                com_counter++;
                break;*/
        }
    }
}

void strrev(char *p)
{
  char *q = p;
  while(q && *q) ++q;
  for(--q; p < q; ++p, --q)
    *p = *p ^ *q,
    *q = *p ^ *q,
    *p = *p ^ *q;
}

void get_interface_id(const char *buffer, size_t size){
    char *aux;
    char dst[512] = {0};
    int diff = 0;
    int i = 0;
    int j = 0;

    if (strstr(buffer, "\\ # Id\r\n") != NULL){
        aux = strstr(buffer, "\\ # Id");
        diff = aux - buffer - 1;
        //printf("aux: %s\n", aux);
        //printf("diff: %d\n", diff);
        while (i <= diff && buffer[diff-i] != '\x0a')
        {
            if (buffer[diff-i] != '\x20') {dst[j] = buffer[diff-i]; j++;}
            i++;
        }
        strrev(dst);
        interfaceID = strtol(dst, NULL, 10);
        //printf("dst: %s\n", dst);
    }
    return;
    //fprintf(stderr, "Test zwyklego id:_%.*s", size, buffer);
    //fprintf(stderr, "***Koniec testu id.\n");
}

void get_service_id(const char *buffer, size_t size){
    char *aux;
    char dst[512] = {0};
    int diff = 0;
    int i = 0;
    int j = 0;

    if (strstr(buffer, "\\ # Id\r\n") != NULL && strstr(buffer, "\\ # SrvcId\r\n") != NULL){
        aux = strstr(buffer, "\\ # Id");
        diff = aux - buffer - 1;
        //printf("aux: %s\n", aux);
        //printf("diff: %d\n", diff);
        while (i <= diff && buffer[diff-i] != '\x0a')
        {
            if (buffer[diff-i] != '\x20') {dst[j] = buffer[diff-i]; j++;}
            i++;
        }
        strrev(dst);
        serviceID = strtol(dst, NULL, 10);
        //printf("dst: %s\n", dst);
    }
    return;
    //fprintf(stderr, "Test zwyklego id:_%.*s", size, buffer);
    //fprintf(stderr, "***Koniec testu id.\n");
}

static void _event_handler1(telnet_t *telnet, telnet_event_t *ev,
		void *user_data) {
	int sock = *(int*)user_data;

	switch (ev->type) {
	/* data received */
	case TELNET_EV_DATA:
		printf("%.*s", (int)ev->data.size, ev->data.buffer);
		process_input_data1(ev->data.buffer, ev->data.size);
		get_interface_id(ev->data.buffer, ev->data.size);
		get_service_id(ev->data.buffer, ev->data.size);
		fflush(stdout);
		break;
	/* data must be sent */
	case TELNET_EV_SEND:
		_send(sock, ev->data.buffer, ev->data.size);
		break;
	/* request to enable remote feature (or receipt) */
	case TELNET_EV_WILL:
		/* we'll agree to turn off our echo if server wants us to stop */
		if (ev->neg.telopt == TELNET_TELOPT_ECHO)
			do_echo = 0;
		break;
	/* notification of disabling remote feature (or receipt) */
	case TELNET_EV_WONT:
		if (ev->neg.telopt == TELNET_TELOPT_ECHO)
			do_echo = 1;
		break;
	/* request to enable local feature (or receipt) */
	case TELNET_EV_DO:
		break;
	/* demand to disable local feature (or receipt) */
	case TELNET_EV_DONT:
		break;
	/* respond to TTYPE commands */
	case TELNET_EV_TTYPE:
		/* respond with our terminal type, if requested */
		if (ev->ttype.cmd == TELNET_TTYPE_SEND) {
			telnet_ttype_is(telnet, getenv("TERM"));
		}
		break;
	/* respond to particular subnegotiations */
	case TELNET_EV_SUBNEGOTIATION:
		break;
	/* error */
	case TELNET_EV_ERROR:
		fprintf(stderr, "ERROR: %s\n", ev->error.msg);
		exit(1);
	default:
		/* ignore */
		break;
	}
}

static void _event_handler2(telnet_t *telnet, telnet_event_t *ev,
		void *user_data) {
	int sock = *(int*)user_data;

	switch (ev->type) {
	/* data received */
	case TELNET_EV_DATA:
		printf("%.*s", (int)ev->data.size, ev->data.buffer);
		process_input_data2(ev->data.buffer, ev->data.size);
		//get_interface_id(ev->data.buffer, ev->data.size);
		//get_service_id(ev->data.buffer, ev->data.size);
		fflush(stdout);
		break;
	/* data must be sent */
	case TELNET_EV_SEND:
		_send(sock, ev->data.buffer, ev->data.size);
		break;
	/* request to enable remote feature (or receipt) */
	case TELNET_EV_WILL:
		/* we'll agree to turn off our echo if server wants us to stop */
		if (ev->neg.telopt == TELNET_TELOPT_ECHO)
			do_echo = 0;
		break;
	/* notification of disabling remote feature (or receipt) */
	case TELNET_EV_WONT:
		if (ev->neg.telopt == TELNET_TELOPT_ECHO)
			do_echo = 1;
		break;
	/* request to enable local feature (or receipt) */
	case TELNET_EV_DO:
		break;
	/* demand to disable local feature (or receipt) */
	case TELNET_EV_DONT:
		break;
	/* respond to TTYPE commands */
	case TELNET_EV_TTYPE:
		/* respond with our terminal type, if requested */
		if (ev->ttype.cmd == TELNET_TTYPE_SEND) {
			telnet_ttype_is(telnet, getenv("TERM"));
		}
		break;
	/* respond to particular subnegotiations */
	case TELNET_EV_SUBNEGOTIATION:
		break;
	/* error */
	case TELNET_EV_ERROR:
		fprintf(stderr, "ERROR: %s\n", ev->error.msg);
		exit(1);
	default:
		/* ignore */
		break;
	}
}

int sock;
int rs;

int connect_to_server(char *ip_addr, char *port_no){
    struct sockaddr_in addr;
    struct addrinfo *ai;
    struct addrinfo hints;

    /* check usage
	if (argc != 3) {
		fprintf(stderr, "Usage:\n ./telnet-client <host> <port>\n");
		return 1;
	}*/

	/* look up server host */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((rs = getaddrinfo(ip_addr, port_no, &hints, &ai)) != 0) {
		fprintf(stderr, "getaddrinfo() failed for %s: %s\n", ip_addr,
				gai_strerror(rs));
		return 1;
	}

	/* create server socket */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "socket() failed: %s\n", strerror(errno));
		return 1;
	}

	/* bind server socket */
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		fprintf(stderr, "bind() failed: %s\n", strerror(errno));
		return 1;
	}

	/* connect */
	if (connect(sock, ai->ai_addr, ai->ai_addrlen) == -1) {
		fprintf(stderr, "server() failed: %s\n", strerror(errno));
		return 1;
	}

	/* free address lookup info */
	freeaddrinfo(ai);
}

void init1()
{
    com_counter = 0;
    end_session = 0;
    char buffer[512];
	struct pollfd pfd[2];
	struct termios tios;

    /* get current terminal settings, set raw mode, make sure we
	 * register atexit handler to restore terminal settings
	 */
	tcgetattr(STDOUT_FILENO, &orig_tios);
	atexit(_cleanup);
	tios = orig_tios;
	cfmakeraw(&tios);
	tcsetattr(STDOUT_FILENO, TCSADRAIN, &tios);

	/* set input echoing on by default */
	do_echo = 1;

	/* initialize telnet box */
	telnet = telnet_init(telopts, _event_handler1, 0, &sock);

	/* initialize poll descriptors */
	memset(pfd, 0, sizeof(pfd));
	pfd[0].fd = STDIN_FILENO;
	pfd[0].events = POLLIN;
	pfd[1].fd = sock;
	pfd[1].events = POLLIN;

	/* loop while both connections are open */
	while (poll(pfd, 2, -1) != -1) {
		/* read from stdin
		if (pfd[0].revents & POLLIN) {
			if ((rs = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
				_input(buffer, rs);
			} else if (rs == 0) {
				break;
			} else {
				fprintf(stderr, "recv(server) failed: %s\n",
						strerror(errno));
				exit(1);
			}
		}*/

        if (end_session == 1) break;
		/* read from client */
		if (pfd[1].revents & POLLIN) {
		    memset(buffer, 0, sizeof(buffer));
			if ((rs = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
				telnet_recv(telnet, buffer, rs);
			} else if ((rs == 0) || (end_session == 1)){
				break;
			} else {
				fprintf(stderr, "recv(client) failed: %s\n",
						strerror(errno));
				//exit(1);
			}
		}
	}
}

void init2()
{
    com_counter = 0;
    end_session = 0;
    char buffer[512];
	struct pollfd pfd[2];
	struct termios tios;

    /* get current terminal settings, set raw mode, make sure we
	 * register atexit handler to restore terminal settings
	 */
	tcgetattr(STDOUT_FILENO, &orig_tios);
	atexit(_cleanup);
	tios = orig_tios;
	cfmakeraw(&tios);
	tcsetattr(STDOUT_FILENO, TCSADRAIN, &tios);

	/* set input echoing on by default */
	do_echo = 1;

	/* initialize telnet box */
	telnet = telnet_init(telopts, _event_handler2, 0, &sock);

	/* initialize poll descriptors */
	memset(pfd, 0, sizeof(pfd));
	pfd[0].fd = STDIN_FILENO;
	pfd[0].events = POLLIN;
	pfd[1].fd = sock;
	pfd[1].events = POLLIN;

	/* loop while both connections are open */
	while (poll(pfd, 2, -1) != -1) {
		/* read from stdin
		if (pfd[0].revents & POLLIN) {
			if ((rs = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
				_input(buffer, rs);
			} else if (rs == 0) {
				break;
			} else {
				fprintf(stderr, "recv(server) failed: %s\n",
						strerror(errno));
				exit(1);
			}
		}*/
        if (end_session == 1) break;
		/* read from client */
		if (pfd[1].revents & POLLIN) {
		    memset(buffer, 0, sizeof(buffer));
			if ((rs = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
				telnet_recv(telnet, buffer, rs);
			} else if ((rs == 0) || (end_session == 1)){
				break;
			} else {
				fprintf(stderr, "recv(client) failed: %s\n",
						strerror(errno));
				//exit(1);
			}
		}
	}
}

void configure_service(char *_port_number, short _c_tag, short _s_tag)
{
    strcpy(port_number, _port_number);
    c_tag = _c_tag;
    s_tag = _s_tag;

    connect_to_server(ip_addr, telnet_port);
    init1();
	/* clean up */
	telnet_free(telnet);
	printf("Zamykam socket...\n");
	close(sock);
}

void unconfigure_service(short _interfaceID, short _serviceID)
{
    interfaceID = _interfaceID;
    serviceID = _serviceID;

    connect_to_server(ip_addr, telnet_port);
    init2();
	/* clean up */
	telnet_free(telnet);
	printf("Zamykam socket...\n");
	close(sock);
}


int main(int argc, char **argv) {

    configure_service("unit-7/port-10", 110, 300);
    unconfigure_service(interfaceID, serviceID);
	return 0;
}
