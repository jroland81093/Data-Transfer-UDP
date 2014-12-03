#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <string.h>
#include <time.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}

/* PACKET DEFINITION */
//Total Packet Size = 1000 Bytes
//Packet Header Size = 100 Bytes

#define BADFILE -1
#define FILEREQ 1
#define DATA 2
#define ACK 3
#define LOSTDATA 4
#define LOSTACK 5
#define CORRDATA 6
#define CORRACK 7

#define TIMEOUT 3 //3 Seconds to retry.
#define WINDOWSIZE 3

#define NAMESTART 16
#define NAMESIZE 84
#define LOADSIZE 100

#define P_LOSS .10  //Probability of packet loss
#define P_CORR .10  //Probability of packet corruption

struct Packet {

  int type; 
  //0 for Data (Sender sends this), 1 for ACK (Receiver sends this.

  int seqSize; 
  //Denotes the total number of packets the file should be.

  int seqNumber; 
  //Sequence number of ACK or Data. 0 = Newfile.

  int fileSize;
  //Total size of the file in bytes

  char name [NAMESIZE]; 
  //Name of file being requested or transmitted.

  char load [LOADSIZE];
};

/* END PACKET DEFINITION */

/* OUTPUT FUNCTIONS */

void printPacket(struct Packet *packet)
{
  if (packet->type == FILEREQ)
  {
    printf("Request for %s\n\n", packet->name);
    return;
  }
  else if (packet->type == DATA || packet->type == CORRDATA)
  {
    if (packet->seqNumber == packet->seqSize)
    {
      printf("[%s DATA %d/%d] \n", packet->name, packet->fileSize, packet->fileSize);
    }
    else
    {
      printf("[%s DATA %d/%d] \n", packet->name, packet->seqNumber * LOADSIZE, packet->fileSize);
    }
    printf("%s\n\n", packet->load);
  }
  else if (packet->type == ACK || packet->type == CORRACK)
  {
    if (packet->seqNumber == packet->seqSize)
    {
      printf("[%s ACK %d/%d] \n\n", packet->name, packet->fileSize, packet->fileSize);
    }
    else
    {
      printf("[%s ACK %d/%d] \n\n", packet->name, packet->seqNumber * LOADSIZE, packet->fileSize);
    }
  }
  else
  {
    printf("****** FATAL ERROR ****** Type is %d\n", packet->type);
  }

}

void printSendPacket(struct Packet *packet)
{
  if (packet->type == FILEREQ || packet->type == DATA || packet->type == ACK)
  {
    struct timeval tv; 
    struct tm* ptm; 
    char time_string[40]; 
    long milliseconds; 
   
    gettimeofday (&tv, NULL); 
    ptm = localtime (&tv.tv_sec); 
    strftime (time_string, sizeof (time_string), "%H:%M:%S", ptm); 
    milliseconds = tv.tv_usec / 1000; 

    printf ("(SEND @ %s.%03ld)\n", time_string, milliseconds); 
    printPacket(packet);
  }
}

void printReceivePacket(struct Packet *packet)
{
  struct timeval tv; 
  struct tm* ptm; 
  char time_string[40]; 
  long milliseconds; 
 
  gettimeofday (&tv, NULL); 
  ptm = localtime (&tv.tv_sec); 
  strftime (time_string, sizeof (time_string), "%H:%M:%S", ptm); 
  milliseconds = tv.tv_usec / 1000; 

  if (packet->type == FILEREQ || packet->type == DATA || packet->type == ACK)
  {
    printf ("(RECV @ %s.%03ld)\n", time_string, milliseconds); 
    printPacket(packet);
  }
  else if (packet->type == CORRDATA || packet->type == CORRACK)
  {
    printf ("(CORRUPT RECV @ %s.%03ld)\n", time_string, milliseconds); 
    printPacket(packet);
  }
}

/* END OUTPUT FUNCTIONS */

/* SENDING FUNCTIONS */

void sendWindow(int sockfd, const struct sockaddr *dest_addr, socklen_t addrlen, struct Packet window[])
{
  int max = 100;
  int i = 0;

  for (i=0; i<WINDOWSIZE; i++)
  {
    //Generate random number.
    int lossRandom = (unsigned)time(NULL) * rand();
    if (lossRandom < 0)
    {
      lossRandom = -lossRandom;
    }
    int corrRandom = (unsigned)time(NULL) * rand();
    if (corrRandom < 0)
    {
      corrRandom = -corrRandom;
    }

    //Print a packet only if it wasn't lost. Then mutate it for the receiver.
    if (lossRandom % max < P_LOSS * max && (window[i].type == DATA || window[i].type == ACK || window[i].type == CORRDATA || window[i].type == CORRACK))
    {
      printf("DROPPING:\n");
    }
    else if (corrRandom % max < P_CORR * max && (window[i].type == DATA || window[i].type == ACK || window[i].type == CORRDATA || window[i].type == CORRACK))
    {
      printf("CORRUPTING\n");
    }

    if (window[i].type == DATA || window[i].type == ACK || window[i].type == CORRDATA || window[i].type == CORRACK)
    {
      printSendPacket(&window[i]);
    }
    
    if (lossRandom % max < P_LOSS * max)
    {
      if (window[i].type == DATA || window[i].type == LOSTDATA || window[i].type == CORRDATA)
      {
        window[i].type = LOSTDATA;
      }
      else
      {
        window[i].type = LOSTACK;
      }
    }
    else if (corrRandom % max < P_CORR * max)
    {
      if (window[i].type == DATA || window[i].type == LOSTDATA || window[i].type == CORRDATA)
      {
        window[i].type = CORRDATA;
      }
      else
      {
        window[i].type = CORRACK;
      }
    }
  }

  if (sendto(sockfd, (void *)window, sizeof(window[0]) * WINDOWSIZE, 0, dest_addr, addrlen) < 0)
  {
    error("Error sending");
  }

}

/* END SENDING FUNCTIONS */

/* MISC AUXILARY FUNCTIONS */

int strCopy(char *dst, char *src)
{
    int len = strlen(src);
    int i;
    for (i=0; i<len; i++)
    {
        dst[i] = src[i];
    }
    return len;
}

int ceiling(int num, int div)
{
  if (num % div == 0)
  {
    return num/div;
  }
  return (num/div)+1;
}
/* END MISC AUXILARY FUNCTIONS */