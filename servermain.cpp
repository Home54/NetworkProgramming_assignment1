#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include  <signal.h>
#include<errno.h>
#include <calcLib.h>
#define MAXSZ 1400
#define COMMENT
// Enable if you want debugging to be printed, see examble below.
// Alternative, pass CFLAGS=-DDEBUG to make, make CFLAGS=-DDEBUG
//#define DEBUG

using namespace std;

int main(int argc, char *argv[]){
  int server_sockfd, client_sockfd;
  int server_len, client_len;
  struct sockaddr_in server_address;
  struct sockaddr_in client_address;
  struct timeval timeToWait;
  fd_set readfds, testfds;
  int result;
  char msg[MAXSZ];

  // Create the server side socket
  server_sockfd = socket(AF_INET, SOCK_STREAM , 0);
  //if(setsockopt(server_sockfd, SOL_SOCKET, SO_RCVTIMEO ,&timeToWait,sizeof(timeToWait))<0) perror("Fail to set the tcp.\n");
  
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  server_address.sin_port = htons(8888);
  server_len = sizeof(server_address);
  bind(server_sockfd, (struct sockaddr * ) & server_address, server_len);
  
  listen(server_sockfd, 5);// Set the maximum number of listening fds to 5
  FD_ZERO( & readfds);// Put the fd of the socket to the fd_set
  FD_SET(server_sockfd, & readfds);
  while (1) {
    int fd;
    // As select will modify the fd_set readfds
    // So we need to copy it to another fd_set testfds
    testfds = readfds;
    printf("Server is waiting \n");
    timeToWait.tv_sec=1000;
    timeToWait.tv_usec=0;
    result = select( FD_SETSIZE, & testfds, NULL , NULL , &timeToWait);
    
    if (result == 0) {// only on the condition when all the client time out, the server will abort all
      	//handle the time out
      	for(fd = 0 ; fd < FD_SETSIZE; fd++ ){
      		if(FD_ISSET(fd,&readfds)!=0&&fd!=server_sockfd){//to disconnect the client
      			send(fd,"EEROR.\n",sizeof("EEROR."),0);
      			FD_CLR(fd, & readfds);
      			close(fd);
      			printf("Client on fd:%d timeout.\n",fd);
      		}
      	}
      	printf("Handled the time out.\n");
      	continue;
    }else if( result < 0 ){
		printf("strerror=%s\n",strerror(errno));
        exit(1);
    }
	
    for (fd = 0; fd < FD_SETSIZE; fd++) {
      if (FD_ISSET(fd, & testfds)) {// handle the changed socket -> eg. listen socket will not be selected until new connection comes either does client socket  
        if (fd == server_sockfd) {
          client_len = sizeof(client_address);
          client_sockfd = accept(server_sockfd,(struct sockaddr * ) & client_address, (socklen_t*)&client_len);
          // Add the client socket to the collection
          // the spcific number of the ESTABLSIHED socket is confined by parameter backlog
          FD_SET(client_sockfd, & readfds);
          printf("Adding the client socket to fd %d\n", client_sockfd);
          //set the timer for the each 
        }
        // If not, it means there is data request from the client socket
        else {
          	memset(msg,0,sizeof(msg));
            int n=recv(fd,msg,MAXSZ,MSG_DONTWAIT);//in this case: the recv will not block -> only when message comes that select will continue  
			
			if(n<=0){
				close(fd);// Remove closed fd (from the unmodified fd_set readfds)
            	FD_CLR(fd, & readfds);
            	printf("Removing client on fd %d\n", fd);
			}
            else{
            	msg[n]='\0';
        		send(fd,msg,n,0);
        		printf("Serving client on fd %d Message:%s\n", fd ,msg); 
            	}
        	//if remove the sleep function: that means it will not cause an exceeding error for the backlog
        	// one assumption:the function will reduce the backlog  
        	// when solve one request (backlog=0)
        	// the program will be blocked in select until the new request come(backlog=1)
        	// then in the time of excuting a few linesten, the program calls accept reduce the backlog 
        }
      }
    }
  }
  return 0;
}
