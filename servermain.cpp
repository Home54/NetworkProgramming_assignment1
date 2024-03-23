#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include  <signal.h>
#include <errno.h>
#include <calcLib.h>
#include <map>
#define MAXSZ 1400
#define WAIT_TIME_SEC 500
#define WAIT_TIME_USEC 0
// Enable if you want debugging to be printed, see examble below.
// Alternative, pass CFLAGS=-DDEBUG to make, make CFLAGS=-DDEBUG
//#define DEBUG

using namespace std;
#ifdef DEBUG
void  INThandler(int sig)//handle the key interuption ^C 
{
  signal(SIGINT, SIG_IGN);          /* ignore this signal       */
  for(int fd = 0 ; fd < FD_SETSIZE ; fd ++ ) close(fd);
  signal(SIGINT, INThandler);     /* reinstall the handler    */
  printf("The program is shutdown by key interuption.\n");
  exit(0);
}
#endif 

int main(int argc, char *argv[]){
  int server_sockfd1, server_sockfd2 ,client_sockfd;
  int server_len, client_len;
  struct sockaddr_in server_address1;
  struct sockaddr_in6 server_address2;
  struct sockaddr_in client_address;
  struct timeval timeToWait;
  fd_set readfds, testfds;
  char msg[MAXSZ];
  int fd_max;
  float hash[FD_SETSIZE];//record the time to the latest server
  memset(hash,-1,sizeof(hash));
  //signal(SIGINT, INThandler); 
	
  // Create the server side socket
  server_sockfd1 = socket(AF_INET, SOCK_STREAM , 0);
  server_sockfd2 = socket(AF_INET6, SOCK_STREAM , IPPROTO_TCP);
  
  server_address1.sin_family = AF_INET;
  server_address1.sin_addr.s_addr = htonl(INADDR_ANY);
  server_address1.sin_port = htons(8888);
  server_len = sizeof(server_address1);
  bind(server_sockfd1, (struct sockaddr * ) & server_address1, server_len);
  
  server_address2.sin6_family = AF_INET6;
  inet_pton(AF_INET6, "::1", &server_address2.sin6_addr);
  server_address2.sin6_port = htons(8889);
  server_len = sizeof(server_address2);
  bind(server_sockfd2, (struct sockaddr * ) & server_address2, server_len);
  
  listen(server_sockfd1, 5);// Set the maximum number of listening fds to 5
  listen(server_sockfd2, 5);
  FD_ZERO( & readfds);// Put the fd of the socket to the fd_set
  FD_SET(server_sockfd1, & readfds);
  FD_SET(server_sockfd2, & readfds);
  fd_max=server_sockfd2;
  
  while (1) {
    int fd;
    // As select will modify the fd_set readfds
    // So we need to copy it to another fd_set testfds
    testfds = readfds;
    printf("Server is waiting \n");
    timeToWait.tv_sec=WAIT_TIME_SEC;
    timeToWait.tv_usec=WAIT_TIME_USEC;
    int result = select( FD_SETSIZE, & testfds, NULL , NULL , &timeToWait);
    
    if( result < 0 ){
		printf("strerror=%s\n",strerror(errno));
        exit(1);
    }
	//printf("wait for sec(s):%f\n",(WAIT_TIME_SEC-(int)(timeToWait.tv_sec))*1.0+(WAIT_TIME_USEC-(int)(timeToWait.tv_usec))/1000000.0);
    for (fd = server_sockfd1; fd <= fd_max; fd++) {
      if (FD_ISSET(fd, & testfds)) {// handle the changed socket -> eg. listen socket will not be selected until new connection comes either does client socket  
        if (fd == server_sockfd1 || fd == server_sockfd2 ) {
        	client_len = sizeof(client_address);
          	client_sockfd = accept(fd,(struct sockaddr * ) & client_address, (socklen_t*)&client_len);
        	if(fd_max-server_sockfd2-1>5){
        		send(client_sockfd,"Disconnecting...(current client is 5).\n",sizeof("Disconnnecting...(current client is 5)."),0);
        		close(client_sockfd);
        		printf("The serving client is up to 5\n");
        	}else{
          // Add the client socket to the collection
          // the spcific number of the ESTABLSIHED socket is confined by parameter backlog
          		fd_max++;
          		hash[fd]=0.0;
          		FD_SET(client_sockfd, & readfds);
          		send(client_sockfd,"Hello Client.\n",sizeof("Hello Client."),0);
          		printf("Adding the client socket to fd %d\n", client_sockfd);
          //set the timer for the each 
        	}
        }
        // If not, it means there is data request from the client socket
        else {
          	memset(msg,0,sizeof(msg));
            int n=recv(fd,msg,MAXSZ,MSG_DONTWAIT);//in this case: the recv will not block -> only when message comes that select will continue  
			
			if(n<=0){
				close(fd);// Remove closed fd (from the unmodified fd_set readfds)
				fd_max--;
				hash[fd]=-1.0;
            	FD_CLR(fd, & readfds);
            	printf("Removing client on fd %d\n", fd);
			}
            else{
            	if(strcmp(msg,"Hello Client.\n")!=0){
            		send(fd,"WRONG ANSWER.\n\n",sizeof("WRONG ANSWER.\n"),0);
            		printf("The client on fd %d gets wrong answer:%s\n",fd,msg);
            	}else{//do good
            		msg[n]='\0';
        			send(fd,"You got a correct answer.\n\n",sizeof("You got a correct answer.\n"),0);
        			printf("The client on fd %d gets correct.\n", fd); 
            	}
                hash[fd]=0.0;//reset the timer
                // a new question
                send(client_sockfd,"Hello Client.\n",sizeof("Hello Client."),0);
            }
        	//if remove the sleep function: that means it will not cause an exceeding error for the backlog
        	// one assumption:the function will reduce the backlog  
        	// when solve one request (backlog=0)
        	// the program will be blocked in select until the new request come(backlog=1)
        	// then in the time of excuting a few linesten, the program calls accept reduce the backlog 
        }
      }else if(FD_ISSET(fd, & readfds)!=0&&fd!=server_sockfd1&&fd!=server_sockfd2){//the unselected ones
      		hash[fd]+=((WAIT_TIME_SEC-(int)(timeToWait.tv_sec))*1.0+(WAIT_TIME_USEC-(int)(timeToWait.tv_usec))/1000000.0);//ignore the time spent on loop all the fd
      		if(hash[fd]>=WAIT_TIME_SEC*1.0+WAIT_TIME_USEC/1000000.0){
      			send(fd,"EEROR.\n",sizeof("EEROR."),0);
      			fd_max--;
      			hash[fd]=-1.0;
      			FD_CLR(fd, & readfds);
      			close(fd);
      			printf("Client on fd:%d timeout.\n",fd);
      		}
      }
    }
  }
  return 0;
}
