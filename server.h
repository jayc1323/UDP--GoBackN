#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)
typedef struct packet *Packet;
struct packet{
        char type;
        char seq_no;
        char data[512];
};
/*----------------------------------------------------------------------------------------------*/
SOCKET setup_server_socket(char *port);
void handle_client(char *request,struct sockaddr *client_address,socklen_t client_len,SOCKET server_socket);