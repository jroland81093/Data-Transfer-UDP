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
//Payload Size = 900 Bytes

#define BADFILE -1
#define FILEREQ 1
#define DATA 2
#define ACK 3
#define LOSTDATA 4
#define LOSTACK 5
#define CORRDATA 6
#define CORRACK 7

#define TIMEOUT 3 //3 Seconds to retry.
#define WINDOWSIZE 4

#define NAMESTART 16
#define NAMESIZE 84
#define LOADSIZE 30 //900

#define P_LOSS .25  //Probability of packet loss
#define P_CORR .10  //Probability of packet corruption

struct Packet {

  int type; 
  //0 for Data (Sender sends this), 1 for ACK (Receiver sends this.

  int seqSize; 
  //Denotes the total number of packets the file should be.

  int seqNumber; 
  //Sequence number of ACK or Data. 0 = Newfile.

  int checkSum; 
  //Supposed value

  char name [NAMESIZE]; 
  //Name of file being requested or transmitted.

  char load [LOADSIZE];
};

int checkSumHash(char *buff)
//SBDM Hashing Algorithm
{
  int hash = 0;
  int n;
  int i;
  for (i = 0; buff[i] != '\0'; i++)
  {
      if(buff[i] > 65)
          n = buff[i] - 'a' + 1;

      else
          n = 27;

      hash = ((hash << 3) + n);
  }
    return hash;
}

/* END PACKET DEFINITION */

/* OUTPUT FUNCTIONS */

void printPacket(struct Packet *packet)
{
  if (packet->type == FILEREQ)
  {
    fprintf(stderr, "Request for %s\n\n", packet->name);
    return;
  }
  else if (packet->type == DATA)
  {
    fprintf(stderr, "[%s DATA] ", packet->name);
    fprintf(stderr, "%d/%d, Checksum: %d, Load:\n", packet->seqNumber, packet->seqSize, packet->checkSum);
    fprintf(stderr, "%s\n\n", packet->load);
  }
  else if (packet->type == ACK)
  {
    fprintf(stderr, "[%s ACK] ", packet->name);
    fprintf(stderr, "%d/%d\n\n", packet->seqNumber, packet->seqSize);
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

    fprintf (stderr, "(SEND @ %s.%03ld)\n", time_string, milliseconds); 
    printPacket(packet);
  }
}

void printReceivePacket(struct Packet *packet)
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

    fprintf (stderr, "(RECV @ %s.%03ld)\n", time_string, milliseconds); 
    printPacket(packet);
  }
}

/* END OUTPUT FUNCTIONS */

/* SENDING FUNCTIONS */

//Returns 1 if the packet was sent correctly, 0 otherwise. 
void sendWindow(int sockfd, const struct sockaddr *dest_addr, socklen_t addrlen, struct Packet window[])
{
  int max = 100;
  int i = 0;

  for (i=0; i<WINDOWSIZE; i++)
  {
    //Generate random number.
    int random = ((unsigned)time(NULL) * rand());
    random = random < 0 ? -random : random;

    //If packet lost.
    if (0) //random % max < P_LOSS * max)
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

    //If Packet Corruption
    else if (0) //random % max < P_CORR * max)
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
    else
    {
        printSendPacket(&window[i]);
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