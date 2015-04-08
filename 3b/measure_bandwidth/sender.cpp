/*************************************
*this sender is used for lab 4. You can use it to get a 
*more accurate bandwidth of the relayer. This sender keeps on sending
*2016-byte packets to the relayer, The relayer may drop some of them,
*and relay the rest of them. The receiver on the other end will grabs
*all the packets relayed by the relayer in 30s and calculates
*(number of received bits / 30s). This is a more accurate estimation
*of the relayer's bandwidth.
*
*You may need to change some of the values bellow according to the relayer's config.xml
*use "g++ -o sender sender.cpp" to compile
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
#include <sys/time.h>
#include <unistd.h>
using namespace std;

#define PKT_SIZE 1016

/***************************************/
//you may need to change these configurations according to relayer's config.xml
char* host_name = (char*)"linux25";//relayer's host name
short int sender_port = 10000;     //this sender's sending port
short int relayer_port = 50001;    //relayer's listening port
/***************************************/

int main()
{
  int udpsock = socket(AF_INET,SOCK_DGRAM,0);
  if (udpsock<0)
  {
     printf("ERROR: opening UDP socket\n");
    return 0;
  }
  struct sockaddr_in l_s_i;
  l_s_i.sin_family=AF_INET;
  l_s_i.sin_addr.s_addr=INADDR_ANY;
  l_s_i.sin_port=htons(sender_port);
  if (bind(udpsock, (struct sockaddr *)&(l_s_i), sizeof(struct sockaddr_in))<0)
  {
    printf("ERROR: UDP Binding error\n");
    return 0;
  }
  
  struct sockaddr_in r_s_i;
  r_s_i.sin_family=AF_INET;
  struct hostent *address = gethostbyname(host_name);
  if(address)
  {
    memcpy(&r_s_i.sin_addr , address->h_addr, address->h_length);
    r_s_i.sin_port=htons(relayer_port);
  }
  else
  {
    printf("ERROR: UDP INIT error\n");
    return 0;
  }

  char sendbuf[PKT_SIZE];
  while(1)
  {
    if(sendto(udpsock, sendbuf, PKT_SIZE, 0, (struct sockaddr *)&r_s_i, sizeof(struct sockaddr_in))<0)
    {
      printf("Error in loop \n");
      continue;
    }
  }
}
