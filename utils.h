#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* PACKET DEFINITION */
//Total Packet Size = 1000 Bytes
//Packet Header Size = 100 Bytes
//Payload Size = 900 Bytes

#define WINDOWSIZE 10
#define ACK 1
#define DATA 0
#define NAMESTART 16
#define NAMESIZE 84
#define LOADSIZE 900
#define PLOSS .25  //Probability of packet loss
#define PCORR .10  //Probability of packet corruption

struct Packet {
  unsigned int type; 
  //0 for Data (Sender sends this), 1 for ACK (Receiver sends this.)

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

void printPacket(struct Packet *packet)
{
  if (packet->type == 0) 
  {
    printf("[%s Data] ", packet->name);
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
  printf ("(SEND %s.%03ld) ", time_string, milliseconds); 
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
  printf ("(RECEIVE %s.%03ld) ", time_string, milliseconds);
  printPacket(packet);
}

int checkSumHash(struct Packet *packet)
{
  return 0;
}

/* END PACKET DEFINITION */


void error(char *msg)
{
    perror(msg);
    exit(1);
}

ssize_t sendToHelper(int sockfd, const void *buf, size_t len, int flags, 
  const struct sockaddr *dest_addr, socklen_t addrlen)
//Tries to send packets with a probability of PLOSS
{
  int max = 100;
  unsigned int random = ((unsigned)time(NULL) * rand()) % max;
  if (random < PLOSS * 100)
  {
    return -1;
  }
  return sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}
/*
int getFileSize(char packet[], int startPos, char *fileName)
  //Get the file stats with stat, then write into the packet.
{
  struct stat attr;
  stat(fileName, &attr);
  int size = attr.st_size;

  int numDigits = 1;
  int tenPower = 10;
  while (size >= tenPower)
  {
    numDigits++;
    tenPower*=10;
  }

  char buff[numDigits];
  bzero(buff, numDigits);
  snprintf (buff, sizeof(buff)+1, "%d", size);
  writeChar(buff, packet , startPos);

  return size;
}
*/

/* BUFFER WRITING FUNCTIONS */

void copyChar (char src[], int srcStart, int numChars, char dst[], int dstStart)
//Copy numChar chars from srcStart into dst starting from dstStart
{
  int i;
  int j=0;
  for (i=srcStart; i<numChars; i++)
  {
    dst[dstStart+j] = src[i];
    j++;
  }
}

void copyUnsignedChar (unsigned char src[], int srcStart, int numChars, unsigned char dst[], int dstStart)
//Copy numChar chars from srcStart into dst starting from dstStart
{
  int i;
  int j=0;
  for (i=srcStart; i<numChars; i++)
  {
    dst[dstStart+j] = src[i];
    j++;
  }
}

int strCopy(char *dst, char *src)
{
    int len = strlen(src);
    for (int i=0; i<len; i++)
    {
        dst[i] = src[i];
    }
    return len;
}
/* END BUFFER WRITING FUNCTIONS */