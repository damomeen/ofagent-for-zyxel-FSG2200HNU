#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strtok, ...
#include <fcntl.h> // open, ...
#include <sys/types.h> // size_t, ...
#include <sys/socket.h>
#include <netdb.h>
#include "libtelnet.h"
#include <errno.h>

static const telnet_telopt_t myTelopts[] = {
    { TELNET_TELOPT_ECHO,      TELNET_WILL, TELNET_DONT },
    { TELNET_TELOPT_TTYPE,     TELNET_WILL, TELNET_DONT },
    { TELNET_TELOPT_COMPRESS2, TELNET_WONT, TELNET_DO   },
    { TELNET_TELOPT_ZMP,       TELNET_WONT, TELNET_DO   },
    { TELNET_TELOPT_MSSP,      TELNET_WONT, TELNET_DO   },
    { TELNET_TELOPT_BINARY,    TELNET_WILL, TELNET_DO   },
    { TELNET_TELOPT_NAWS,      TELNET_WILL, TELNET_DONT },
    { -1, 0, 0 }
  };

//telnet_t *telnet_init(const telnet_telopts_t *telopts, telnet_event_handler_t handler, unsigned char flags, void *user_data);



void telnetHandlerCallback(telnet_t *telnet, telnet_event_t *ev, void *user_data)
{
   char buffer[10000];
   switch(ev->type)
   {
      case TELNET_EV_DATA:
         //m_InBuffer.append(ev->buffer, ev->size);
         memcpy (buffer, ev->data.buffer, ev->data.size);
         buffer[ev->data.size] = 0;
         printf("%s\n", buffer);
         break;
      case TELNET_EV_SEND:
         //writeTo(m_Control, ev->buffer, ev->size);
         break;
      case TELNET_EV_ERROR:
         //Really need to come up with some kind of error reporting/bug function...
         //fatal_error("TELNET error: %s", ev->buffer);
         break;
      default:
         break;
   }
}

int readFrom(int socket_fd, telnet_t *mTelnet)
{
    char readBuffer[4096];
    int size;

    while(1)
    {
          size = read(socket_fd, readBuffer, sizeof(readBuffer));

          if(size > 0) {
             telnet_recv(mTelnet, readBuffer, size);
          }
          else if (size == 0)
             break;
          else if (errno == EAGAIN)
             break;
          else
             return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    //struct hostent *hp;
    struct sockaddr_in srv;
    int socket_fd;
    char userData[1000];

    /* create a connection to the server */
    srv.sin_family = AF_INET; // use the internet addr family
    srv.sin_port = htons(23); // dedicated telnet port = 23
    srv.sin_addr.s_addr = inet_addr("10.0.1.1"); // ptt.cc IP address

    /* create the socket */
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket");
            exit(1);
    }

    printf("socket_fd: %d\n", socket_fd);

    /* connect to the server */
    printf("Connecting to 10.0.1.1 [ptt.cc]...\n");
    if (connect(socket_fd, (struct sockaddr*) &srv, sizeof(srv)) < 0) {
            perror("connect");
            exit(1);
    } else {
            printf("Connection successful.\n\n");
    }

    telnet_t *mTelnet;
    mTelnet = telnet_init(myTelopts, &telnetHandlerCallback, 0, userData);
    telnet_negotiate(mTelnet, TELNET_WILL, TELNET_TELOPT_COMPRESS2 );
    readFrom(socket_fd, mTelnet);
    printf("Po readzie\n");

    //void telnet_recv(telnet_t *telnet, const char *buffer, unsigned int size, void *user_data);


    //while(1){};
    //sleep(30);
    telnet_negotiate(mTelnet, TELNET_WILL, TELNET_TELOPT_COMPRESS2 );
    //close connection
    close(socket_fd);
    telnet_free(mTelnet);
    return 0;
}
