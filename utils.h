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

#define WINDOWSIZE 3
#define ACK 1
#define DATA 0
#define END -1
#define NAMESTART 16
#define NAMESIZE 84
#define LOADSIZE 2 //900
#define P_LOSS .1  //Probability of packet loss
#define P_CORR .10  //Probability of packet corruption

struct Packet {
  unsigned int type; 
  //0 for Data (Sender sends this), 1 for ACK (Receiver sends this.) -1 Indicates termination (end of file).

  unsigned int seqSize; 
  //Denotes the total number of packets the file should be.

  unsigned int seqNumber; 
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
  if (packet->type == ACK && packet->seqNumber == 0)
  {
    printf("Starting %s transfer\n", packet->name);
    return;
  }
  if (packet->type == DATA) 
  {
    printf("[%s DATA] ", packet->name);
  }
  else
  {
    printf("[%s ACK] ", packet->name);
  }
  printf("%u/%u, Checksum: %d\n", packet->seqNumber, packet->seqSize, packet->checkSum);
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

ssize_t sendToHelper(int sockfd, const void *buf, size_t len, int flags, 
  const struct sockaddr *dest_addr, socklen_t addrlen)
//Tries to send packets with a probability of P_LOSS
{
  int max = 100;
  struct Packet *pack = (struct Packet *) buf;

  //Used to help generate random behavior.
  int check = pack->checkSum;
  if (check < 0)
  {
    check = -check;
  }
  unsigned int random = ((unsigned)time(NULL) * rand() * check) % max;
  if (random < P_LOSS * 100)
  {
    return -1;
  }
  int val = sendto(sockfd, buf, len, flags, dest_addr, addrlen);
  if (val < 0)
  {
    error("Error sending");
  }
  return val;
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