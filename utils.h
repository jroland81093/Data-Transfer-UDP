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

#define ACK 1
#define DATA 2
#define LOSTACK -1
#define LOSTDATA -2
#define END 0
#define WINDOWSIZE 3

#define NAMESTART 16
#define NAMESIZE 84
#define LOADSIZE 100 //900

#define P_LOSS .25  //Probability of packet loss
#define P_CORR .10  //Probability of packet corruption

struct Packet {

  int type; 
  //0 for Data (Sender sends this), 1 for ACK (Receiver sends this.

  int seqSize; 
  //Denotes the total number of packets the file should be.

  int seqNumber; 
  //Sequence number of ACK or Data. 0 = Newfile, -1= Dropped.

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
  if (packet->type == ACK && packet->seqNumber == 0)
  {
    printf("Starting %s transfer\n", packet->name);
    return;
  }
  if (packet->type == DATA || packet->type == LOSTDATA) 
  {
    printf("[%s DATA] ", packet->name);
  }
  else
  {
    printf("[%s ACK] ", packet->name);
  }
  printf("%d/%d, Checksum: %d\n", packet->seqNumber, packet->seqSize, packet->checkSum);
}

void printSendPacket(struct Packet *packet)
{
  //Append a timestamp.
  struct timeval tv; 
  struct tm* ptm; 
  char time_string[40]; 
  long milliseconds; 
 
  gettimeofday (&tv, NULL); 
  ptm = localtime (&tv.tv_sec); 
  strftime (time_string, sizeof (time_string), "%H:%M:%S", ptm); 
  milliseconds = tv.tv_usec / 1000; 
  /* Print the formatted time, in seconds, followed by a decimal point 
     and the milliseconds. */ 
  printf ("(Sent @ %s.%03ld) ", time_string, milliseconds); 
  printPacket(packet);
}

void printReceivePacket(struct Packet *packet)
{
  //Append a timestamp.
  struct timeval tv; 
  struct tm* ptm; 
  char time_string[40]; 
  long milliseconds; 
 
  gettimeofday (&tv, NULL); 
  ptm = localtime (&tv.tv_sec); 
  strftime (time_string, sizeof (time_string), "%H:%M:%S", ptm); 
  milliseconds = tv.tv_usec / 1000; 
  /* Print the formatted time, in seconds, followed by a decimal point 
     and the milliseconds. */ 
  printf ("(Received @ %s.%03ld) ", time_string, milliseconds);
  printPacket(packet);
}

/* END OUTPUT FUNCTIONS */

/* SENDING FUNCTIONS */

void sendEndOfFile(int sockfd, const struct sockaddr *dest_addr, socklen_t addrlen)
//Send to the receiver to indicate that it should terminate.
{
  struct Packet pack;
  bzero(&pack, sizeof(pack));
  pack.type = END;
  sendto(sockfd, (const void *) &pack, sizeof(pack), 0, dest_addr, addrlen);
  printf("Finished RDT of file.\n");
}

void sendWindowHelper(int sockfd, struct Packet window[], int type, const struct sockaddr *dest_addr, socklen_t addrlen)
{
  int max = 100;
  int i =0;
  for (i=0; i<WINDOWSIZE; i++)
  {
    
  }
}

void sendWindowHelper(int sockfd, struct Packet window[], int type, const struct sockaddr *dest_addr, socklen_t addrlen)
{
  int max = 100;
  int i = 0;

  for (i=0; i<WINDOWSIZE; i++)
  {
    //Only print significant messages to the screen.
    if (window[i].seqNumber != 0)
    {
      //Generate random behavior.
      int random = ((unsigned)time(NULL) * rand());
      random = random < 0 ? -random : random;

      //Simulate Packet Loss
      if (random % max < P_LOSS * max && window[i].seqNumber != 0)
      {
        fprintf(stderr, "DROPPED: ");
        printSendPacket(&(window[i]));
        window[i].type = LOSTDATA;
      }

      //Simulate Packet Corruption
      else if (random % max < P_CORR * max && window)
      {
        fprintf(stderr, "CORRUPTED: ");
        printSendPacket(&(window[i]));
        //Do stuff to corrupt the packet.
      }

      else
      {
        printSendPacket(&window[i]);
      }
    }
  }
  fprintf(stderr, "\n");

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