/*--------------------------------------------------------------------*/
/* conference server */
//Adrian Guzman

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/select.h>
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <netdb.h>
#include <time.h> 
#include <unistd.h>
#include <errno.h>

extern char *  recvdata(int sd);
extern int     senddata(int sd, char *msg);

extern int     startserver();
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/* main function*/
int main(int argc, char *argv[])
{
  int    serversock;    /* server socket*/

  fd_set liveskset1;   /* set of live client sockets */
  fd_set liveskset2;  /* set of live client sockets used by select method */
  int    liveskmax;   /* maximum socket number */

  /* check usage */
  if (argc != 1) {
    fprintf(stderr, "usage : %s\n", argv[0]);
    exit(1);
  }

  /* get ready to receive requests */
  serversock = startserver();
  if (serversock == -1) {
    exit(1);
  }
   liveskmax = serversock; //set max socket to server socket
  
  /*
    init the live client set 
  */
   FD_ZERO(&liveskset1);
   FD_ZERO(&liveskset2);
   FD_SET(serversock, &liveskset1);

  /* receive and process requests */
  while (1) {
    int    itsock;      /* loop variable */

    /*
      using select() to serve both live and new clients
    */
     liveskset2 = liveskset1; //reinitialize the fd set used by select because it is changed after the select call
     int selection = select(liveskmax + 1, &liveskset2, NULL, NULL, NULL);
    
    /* process messages from clients */
    for (itsock=3; itsock <= liveskmax; itsock++) {
	if ((itsock == serversock) && FD_ISSET(itsock, &liveskset2)) { //if connecting socket is the listen socket

	      /*
		accept a new connection request
	      */
		struct sockaddr_in client;
		socklen_t client_len = sizeof(client);
		int client_fd = accept(serversock, (struct sockaddr *) &client, &client_len);

	        if (client_fd >= 0) {//acception is fine
			char *  clienthost;  /* host name of the client */
			ushort  clientport;  /* port number of the client */

			/*
			  get client's host name and port using gethostbyaddr() 
			*/
			struct sockaddr_storage addr;
			int len = sizeof(addr);
			getpeername(client_fd, (struct sockaddr *)&addr, &len); //gets info about client
			struct sockaddr_in *s = (struct sockaddr_in *)&addr; //gets info regarding client portno
			clientport = ntohs(s->sin_port);

			struct hostent *he;
			he = gethostbyaddr(&client.sin_addr, sizeof(client.sin_addr), AF_INET); //gets info about client host name
			clienthost = he->h_name;
			printf("admin: connect from '%s' at '%hu'\n",
			       clienthost, clientport);

			/*
			  add this client to 'liveskset'
			*/
			FD_SET(client_fd, &liveskset1); //add client to liveskset
			/* Increments liveskmax to represent the max file descriptor in set.
			   conditionals are for when a client is closed and a new client opens and 
			   uses the previous clients fd number */
			if (liveskmax > client_fd) liveskmax = liveskmax + 1;
			else liveskmax = client_fd + 1;
			break;
	        } 
	        else {
			perror("accept");
			exit(0);
	        }
         }
	else if (FD_ISSET(itsock, &liveskset2) && (itsock != serversock)) { //connecting socket is an existing connection sending msg

		char *  clienthost;  /* host name of the client */
		ushort  clientport;  /* port number of the client */
		
		/*
		  obtain client's host name and port
		  using getpeername() and gethostbyaddr()
		*/
		struct sockaddr_storage addr;
		int len = sizeof(addr);
		getpeername(itsock, (struct sockaddr *)&addr, &len); //get info about client
		struct sockaddr_in *s = (struct sockaddr_in *)&addr; //get info about client portno
		clientport = ntohs(s->sin_port);

		struct hostent *he;
		he = gethostbyaddr(&s->sin_addr, sizeof(s->sin_addr), AF_INET); //get info about client hostname
		clienthost = he->h_name;
		
		/* read the message */
		char * msg = recvdata(itsock);
		if (!msg) {
		  /* disconnect from client */
		  printf("admin: disconnect from '%s(%hu)'\n",
			 clienthost, clientport);

		  /*
		    remove this client from 'liveskset'  
		  */
		  FD_CLR(itsock, &liveskset1);
		  close(itsock);
		} 
		else {
			/*
			  send the message to other clients
			*/
			int outsock; //socket to write to
			for (outsock = 3; outsock < liveskmax; outsock++) {//loop through all clients on client list
				if (outsock != serversock && outsock != itsock) {//socket is not server or client that sent message
					int err = senddata(outsock, msg); //send message to specified client
					if (err == -1) printf("Client write failed\n");
				}
			}
			  
			/* print the message */
			printf("%s(%hu): %s", clienthost, clientport, msg);
			free(msg);
		}
	}	
    }
  }
}
/*--------------------------------------------------------------------*/
