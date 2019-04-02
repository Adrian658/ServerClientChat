/*--------------------------------------------------------------------*/
/* functions to connect clients and server */
//Adrian Guzman

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h> 
#include <sys/unistd.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <netdb.h>
#include <time.h> 
#include <errno.h>

#define MAXNAMELEN 256
/*--------------------------------------------------------------------*/


/*----------------------------------------------------------------*/
int startserver()
{
  int     sd;        /* socket */

  char *  serverhost;  /* hostname */
  ushort  serverport;  /* server port */

  /*
    create a TCP socket
  */
   sd = socket(AF_INET, SOCK_STREAM, 0);

  /*
    bind the socket to some random port, chosen by the system 
  */
   struct sockaddr_in server, my_addr; //structs to find portno's
   server.sin_family = AF_INET;
   server.sin_port = htons(0); //portno
   server.sin_addr.s_addr = INADDR_ANY; //IP address
   int err = bind(sd, (struct sockaddr *) &server, sizeof(server)); //bind socket to host port to establish connection
   if (err < 0) printf("Could not listen on socket\n");

  /* ready to receive connections */
  listen(sd, 5);

  /*
    obtain the full local host name (serverhost)
    use gethostname() and gethostbyname()
  */
   char hostname[128];
   gethostname(hostname, sizeof(hostname));
   struct hostent *he;
   he = gethostbyname(hostname);
   serverhost = he->h_name;

  /*
    get the port assigned to this server (serverport)
    use getsockname()
  */
   int len = sizeof(my_addr);
   getsockname(sd, (struct sockaddr *) &my_addr, &len);
   serverport = ntohs(my_addr.sin_port);

  /* ready to accept requests */
  printf("admin: started server on '%s' at '%hu'\n",
	 serverhost, serverport);
  return(sd);
}
/*----------------------------------------------------------------*/

/*----------------------------------------------------------------*/
/*
  establishes connection with the server
*/
int connecttoserver(char *serverhost, ushort serverport)
{
  int     sd;          /* socket */

  ushort  clientport;  /* port assigned to this client */

  /*
    create a TCP socket 
  */
   sd = socket(AF_INET, SOCK_STREAM, 0);
   if (sd < 0) (printf("Could not create socket for client"));

  /*
    connect to the server on 'serverhost' at 'serverport'
    use gethostbyname() and connect()
  */

   struct hostent *he; //struct for gethostbyname method
   struct in_addr **addr_list; //struct to store server address accessed using gethostbyname method
   struct sockaddr_in server_addr, my_addr; //structs to store final address's
   he = (gethostbyname(serverhost));
   if (he == NULL) printf("hostbyname error");
   addr_list = (struct in_addr **)he->h_addr_list; //list storing IP address of host

   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr = *addr_list[0]; //set IP address to host IP address
   server_addr.sin_port = htons(serverport);
   
   int err = connect(sd, (struct sockaddr*)&server_addr, sizeof(server_addr));
   if (err < 0) printf("Connect Error: %d\n", err);
  
  /*
    get the port assigned to this client
    use getsockname()
  */
   int len = sizeof(my_addr);
   getsockname(sd, (struct sockaddr *) &my_addr, &len);
   clientport = ntohs(my_addr.sin_port);

  /* succesful. return socket */
  printf("admin: connected to server on '%s' at '%hu' thru '%hu'\n",
	 serverhost, serverport, clientport);
  return(sd);
}
/*----------------------------------------------------------------*/


/*----------------------------------------------------------------*/
int readn(int sd, char *buf, int n)
{
  int     toberead;
  char *  ptr;

  toberead = n;
  ptr = buf;
  while (toberead > 0) {
    int byteread;

    byteread = read(sd, ptr, toberead);
    if (byteread <= 0) {
      if (byteread == -1)
	perror("read");
      return(0);
    }
    
    toberead -= byteread;
    ptr += byteread;
  }
  return(1);
}

char *recvdata(int sd)
{
  char *msg;
  long  len;
  
  /* get the message length */
  if (!readn(sd, (char *) &len, sizeof(len))) {
    return(NULL);
  }
  len = ntohl(len);

  /* allocate memory for message */
  msg = NULL;
  if (len > 0) {
    msg = (char *) malloc(len);
    if (!msg) {
      fprintf(stderr, "error : unable to malloc\n");
      return(NULL);
    }

    /* read the message */
    if (!readn(sd, msg, len)) {
      free(msg);
      return(NULL);
    }
  }
  
  return(msg);
}

int senddata(int sd, char *msg)
{
  long len;

  /* write lent */
  len = (msg ? strlen(msg) + 1 : 0);
  len = htonl(len);
  write(sd, (char *) &len, sizeof(len));

  /* write message data */
  len = ntohl(len);
  if (len > 0)
    write(sd, msg, len);
  return(1);
}
/*----------------------------------------------------------------*/
