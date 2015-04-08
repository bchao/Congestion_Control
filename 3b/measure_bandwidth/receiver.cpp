/*************************************
*this receiver is used for lab 4. You can use it to get a 
*more accurate bandwidth of the relayer. This receiver grabs
*all the packets relayed by the relayer in 30s, and calculate
*(number of bits received / 30s). This is a more accurate estimation
*of the relayer's bandwidth.
*
*You may need to change the udpport according to the relayer's config.xml
*use "g++ -o receiver receiver.cpp" to compile
*To run: 
*(1)start the relayer first
*./relayer config.xml
*(2)run the sender
*./sender
*(3)run the receiver
*./receiver
*after 30s, you'll get the relayer's bandwidth
*************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
using namespace std;

#define PKT_SIZE 1016
/*****************************************/
//this is the receiver's listening port, you may need to change it before running
unsigned short int udpport = 20000;
/*****************************************/

int udpsock;
int listen_init(unsigned short int port)
{
    int length;
    struct sockaddr_in server;   	
    
    /* Create a socket for receiving feedback */
    udpsock=socket(AF_INET,SOCK_DGRAM,0);
    if (udpsock<0)
    {
 	 printf("ERROR: opening stream socket!\n");
   	exit(1);
    }

    server.sin_family=AF_INET;
    server.sin_addr.s_addr=INADDR_ANY;
    server.sin_port=htons(port);
    
    /*Bind the socket to the listening port */
    if (bind(udpsock,(struct sockaddr *)&server,sizeof server)<0)
    {
    	printf("UDP Binding error, maybe port number occupied by others. Change another port");
   	exit(1);
    };
    length=sizeof server;
    if (getsockname(udpsock,(struct sockaddr *)&server,(socklen_t *)&length)<0)
    {
	 printf("ERROR: getting socket name\n");
   	exit(1);
    }
    printf("UDP Socket port # %d\n",ntohs(server.sin_port)); 
    return 0;
};


int main()
{
  if (listen_init(udpport)!=0) { printf("UDP Socket establish failure!\n"); return 1;};
  double num = 0.0;
  struct timeval start;
  struct timeval current;
  gettimeofday (&start, NULL);

  char buf[PKT_SIZE];
  fd_set setForSelect;
  while (1)
  { 
    FD_ZERO(&setForSelect);
    FD_SET(udpsock,&setForSelect);   	 
    select(udpsock+1,&setForSelect,0,0,NULL);
    if (FD_ISSET(udpsock,&setForSelect))
    {
      struct sockaddr incoming_addr;
      memset(buf,0,PKT_SIZE);   
      socklen_t incoming_len = sizeof(incoming_addr);
      int rval = recvfrom(udpsock,buf,PKT_SIZE,0,&incoming_addr, &incoming_len);   
      if(rval == PKT_SIZE)
      {
          num = num + 1.0;
          printf("%f\n", num);
      }
    }
    gettimeofday (&current, NULL);
    if(current.tv_sec - start.tv_sec >= 30)
      break;
  }
  printf("!!! relayer's bandwidth is %f kb/s\n", (((8.0 * PKT_SIZE) / (current.tv_sec - start.tv_sec)) * num) / 1000.0);
}
