#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "calcLib.h"
#include "time.h"
#include <math.h>
#define MAXSZ 1400
#define WAIT_TIME_SEC 5
#define WAIT_TIME_USEC 0
#define MAX_CLIENT 1
// Enable if you want debugging to be printed, see examble below.
// Alternative, pass CFLAGS=-DDEBUG to make, make CFLAGS=-DDEBUG
//#define DEBUG

char ques[MAXSZ];
char* ans[MAX_CLIENT];

void genQues(char * ques , char ** ans , int pos ){
	double fresult;
        int iresult;
        const char *arith[] = {"add", "div", "mul", "sub", "fadd", "fdiv", "fmul", "fsub"};
        srand(time(NULL));
                    /* Figure out HOW many entries there are in the list.
                       First we get the total size that the array of pointers use, sizeof(arith). Then we divide with
                       the size of a pointer (sizeof(char*)), this gives us the number of pointers in the list.
                    */
        int Listitems = sizeof(arith) / (sizeof(char *));
        int itemPos = rand() % Listitems;
        char itemPosStr[10];
        char resultStr[MAXSZ]; // asume that the length of answer is less than 20 char  
        sprintf(itemPosStr, "%d\n", itemPos);

                    /* As we know the number of items, we can just draw a random number and modulo it with the number
                       of items in the list, then we will get a random number between 0 and the number of items in the list

                       Using that information, we just return the string found at that position arith[itemPos];
                    */

        const char *ptr;
        ptr = arith[itemPos]; // Get a random arithemtic operator.

        int i1, i2;
        double f1, f2;
        i1 = rand() % 100;
        i2 = rand() % 100;
        f1 = (double) rand() / (double) (RAND_MAX / 100.0);
        f2 = (double) rand() / (double) (RAND_MAX / 100.0);
        if (ptr != NULL && strlen(ptr) > 0) {
        if (ptr[0] == 'f') {
            sprintf( ques, "%s %.5f %.5f\n", ptr, f1, f2);
                            if (strcmp(ptr, "fadd") == 0) {
                                fresult = f1 + f2;
                            } else if (strcmp(ptr, "fsub") == 0) {
                                fresult = f1 - f2;
                            } else if (strcmp(ptr, "fmul") == 0) {
                                fresult = f1 * f2;
                            } else if (strcmp(ptr, "fdiv") == 0) {
                                fresult = f1 / f2;
                            }
// 将浮点数 fresult 转换为字符串
                            sprintf(resultStr, "%f\n", fresult);

// 发送结果字符串给客户端
                        } else {
                            sprintf( ques, "%s %d %d\n", ptr, i1, i2);
                            if (strcmp(ptr, "add") == 0) {
                                iresult = i1 + i2;
                            } else if (strcmp(ptr, "sub") == 0) {
                                iresult = i1 - i2;
                            } else if (strcmp(ptr, "mul") == 0) {
                                iresult = i1 * i2;
                            } else if (strcmp(ptr, "div") == 0) {
                                iresult = i1 / i2;
                            } else {
                                printf("No match\n");
                            }
                            sprintf(resultStr, "%d\n", iresult);

                        }
                        //end the job
                        //set the timer for the each
                    }
                    *(ans+pos) = strdup(resultStr);
}

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
  fd_max=0;

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
    for (fd = server_sockfd1; fd <= server_sockfd2+MAX_CLIENT; fd++) {
      if (FD_ISSET(fd, & testfds)) {
// handle the changed socket -> eg. listen socket will not be selected until new connection comes either does client socket
        if (fd == server_sockfd1 || fd == server_sockfd2 ) {
            client_len = sizeof(client_address);
            client_sockfd = accept(fd, (struct sockaddr *) &client_address, (socklen_t *) &client_len);
            if (fd_max + 1 > MAX_CLIENT) {
                send(client_sockfd, "Disconnecting...(current client is MAX_CLIENT).\n",
                     sizeof("Disconnnecting...(current client is MAX_CLIENT)."), 0);
                close(client_sockfd);
                printf("The serving client is up to MAX_CLIENT\n");
            } else {
                // Add the client socket to the collection
                // the spcific number of the ESTABLSIHED socket is confined byans parameter backlog
                fd_max++;
                hash[fd] = 0.0;
                FD_SET(client_sockfd, &readfds);  
               send(client_sockfd, "Hello Client.\n" , sizeof("Hello Client.") , 0);
            }
        }
        // If not, it means there is data request from the client socket
        else {
          	memset(msg,0,sizeof(msg));
            int n=recv(fd,msg,MAXSZ,0);//in this case: the recv will not block -> only when message comes that select will continue
			
		if(n<=0){
			close(fd);// Remove closed fd (from the unmodified fd_set readfds)
			fd_max--;
			hash[fd]=-1.0;
            		FD_CLR(fd, & readfds);
            		printf("Removing client on fd %d\n", fd);
            		ans[fd-server_sockfd2-1]=NULL;
			}
            else{
            	//based on the hash to check the answer
            	
               	const double precision = 0.001;
            if(strncmp(msg,"OK",2)==0){
            	hash[fd]=0.0;//reset the timer
                // a new question the same as that wrote above
               	 memset(ques,0,sizeof(ques));
               	 genQues(&ques[0],ans,client_sockfd-server_sockfd2-1);
                	//send the questions to the client
               	send(fd, ques , sizeof(ques), 0);
               	continue;
            }else if(ans[fd-server_sockfd2-1]==NULL){
            	send(fd,"Bye.",sizeof("Bye."),0);
            	fd_max--;
            	close(fd);
            	FD_CLR(fd, & readfds);
            	continue;
            }
            
			if(fabs(atof(msg) - atof(ans[fd-server_sockfd2-1])) >= precision){
            	send(fd,"WRONG ANSWER.\n\n",sizeof("WRONG ANSWER.\n"),0);
            	printf("The client on fd %d gets wrong answer:%s\n",fd,msg);
            	free(ans[fd-server_sockfd2-1]);
            }else{//do good
        		send(fd,"You got a correct answer.\n\n",sizeof("You got a correct answer.\n"),0);
        		printf("The client on fd %d gets correct.\n", fd);
        		free(ans[fd-server_sockfd2-1]);
            }		
                hash[fd]=0.0;//reset the timer
                // a new question the same as that wrote above
               	 memset(ques,0,sizeof(ques));
               	 genQues(&ques[0],ans,client_sockfd-server_sockfd2-1);
                	//send the questions to the client
               	 send(fd, ques , sizeof(ques), 0);
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
}

//generate the question:
//void genQues(char ** resQus , char ** resAns ) //int * bufflen
//char * qus ; send to the client
//char * ans ; the accept answer
//void genQues(&qus,&ans)

