#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <string.h>
#include <time.h>

const int PACKETSIZE = 1000;

const int TYP_START = 0;
const int SEQ_START = 4;
const int CHK_START = 8;
const int SIP_START = 12;
const int SPT_START = 16;
const int NAM_START = 20;
/*
A packet consists of 
  1) Header (100 bytes)
      Data/ACK(bool): 
        Byte 0
        1 if Data, 0 if ACK

      Sequence Number(int): 
        Byte 4-7
        Denotes the sequence number being sent or received

      Checksum(int):
        Byte 8-11
        Helps check for errors in the payload.

      SourceIP(int): Denotes sender of the packet.
        Byte 12-15

      SourcePort(int): Denotes port which sent from.
        Byte 16-19

      Name (string): Name of file requested.
        Byte 20-99

  2) Payload (900 bytes)
*/

const double PLOSS = .25;
const double PCORR = .10;

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

int writeChar (const char *src, char dst[], int dstStart)
//Returns length of the characters being written
{
  int i=0;
  while (*(src+i)!= '\0' && *(src+i) != 0)
  {
    dst[dstStart+i] = *(src+i);
    i++;
  }
  return i;
}