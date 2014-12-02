#include <strings.h>
#include <sys/wait.h> /* for the waitpid() system call */
#include <signal.h> /* signal name macros, and the kill() prototype */
#include <time.h> //Used to pull time for file IO
#include <sys/types.h> //Used for last modified time 
#include <sys/stat.h> //Used for last modified time.

#include <utils.h>

//Acts as the server.
int receiveFileTransfer(int sockfd, struct sockaddr *recv_addr, socklen_t addrlen, struct Packet window[], char fileName[], int *filefd);
void generateSendWindow(struct Packet window[], char fileName[], int filefd, int seqSize, int lastAcked);
int receiveWindow(int sockfd, struct sockaddr *recv_addr, socklen_t addrlen, struct Packet window[]);

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[])
{
  //Setup port and sockets.
  if (argc < 2) {
    error("Must provide <portno>\n");
  }
  //Declare all necessary structures for networking.
  struct sigaction sa;                          //Process handler
  struct sockaddr_in sendAddr;                  //Sender/Server address
  struct sockaddr_in recvAddr;                  //Receiver/Client address
  socklen_t addrlen = sizeof(recvAddr);         //Length of address
  int portno = atoi(argv[1]);                   //Port number
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);  //Socket access

  if (sockfd < 0) {
    error("Error opening socket");
  }

  //Fill out socket information
  bzero((char *) &sendAddr, sizeof(sendAddr));
  sendAddr.sin_family = AF_INET;
  sendAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  sendAddr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *) &sendAddr, sizeof(sendAddr)) <0) {
    error("Error on binding");
  }

  /* ALGORITHM START */
  
  //Infinite loop after the server is bound to keep it running for multiple requests.
  while (1)
  {
    struct Packet window[WINDOWSIZE];
    char fileName[NAMESIZE];
    int filefd;
    int seqSize = receiveFileTransfer(sockfd, (struct sockaddr *) &recvAddr, addrlen, window, fileName, &filefd);

    if (seqSize < 0)
      //Tell the receiver that the file was not found.
    {
      window[0].type = BADFILE;
      sendto(sockfd, (void *)window , sizeof(window[0]), 0, (struct sockaddr *) &recvAddr, addrlen);
    }

    int lastAcked = 0;
    //Loop until we receive the final ACK
    while (1 && seqSize > 0)
    {
      int prevAck = lastAcked;
      generateSendWindow(window, fileName, filefd, seqSize, lastAcked);
      sendWindow(sockfd, (struct sockaddr *) &recvAddr, addrlen, window);
      lastAcked += receiveWindow(sockfd, (struct sockaddr *) &recvAddr, addrlen, window);

      if (lastAcked == seqSize)
      {
        fprintf(stderr, "RDT for %s is complete.\nWaiting for next file request...\n", fileName);
        fprintf(stderr, "--------\n\n");
        break;
      }
      else if (lastAcked == prevAck)
      //Timeout and resend packets.
      {
        sleep(TIMEOUT);
        fprintf(stderr, "TIMEOUT! Retrying now...\n\n");
      }
    }
    fclose(fdopen(filefd, "rb"));
  }
  return 0;
}

int receiveFileTransfer(int sockfd, struct sockaddr *recv_addr, socklen_t addrlen, struct Packet window[], char fileName[], int *filefd)
//Returns the sequence size of the file requested.
//fileName and filefd are populated.
{
    struct Packet requestPacket;
    bzero((char *) &requestPacket, sizeof(requestPacket));
    bzero((char *) window, sizeof(requestPacket) * WINDOWSIZE);
    bzero((char *) fileName, NAMESIZE);

    if (recvfrom(sockfd, &requestPacket, sizeof(requestPacket), 0, recv_addr, &addrlen) < 0) {
        error("Couldn't receive request");
    }
    printReceivePacket(&requestPacket);

    //Setup Local File I/O to determine size, etc.
    struct stat fileStat;
    FILE *fp = fopen(requestPacket.name, "rb");
    if (fp == NULL) {
      fprintf(stderr, "No such file or directory\n");
      fprintf(stderr, "--------\n\n");
      return -1;
    }
    int fd = fileno(fp);
    if(fstat(fd, &fileStat) < 0) {
      error("Could not determine file");
    }

    int i=0;
    int numSequences = ceiling(fileStat.st_size, LOADSIZE);
    for (i=0; i<WINDOWSIZE; i++)
    {
      window[i].seqSize = numSequences;
      strCopy(window[i].name, requestPacket.name);
    }

    //fclose(fp);
    strCopy(fileName, requestPacket.name);
    *filefd = fd;
    return numSequences;
}

void generateSendWindow(struct Packet window[], char fileName[], int filefd, int seqSize, int lastAcked)
//Sends the next window of files to the receiver.
{
  int i;
  char buff[LOADSIZE];
  bzero((char *)window, sizeof(window[0]) * WINDOWSIZE);

  for (i=0; i<WINDOWSIZE; i++)
  {
    bzero(buff, LOADSIZE);
    int seqNum = lastAcked+i+1;
    if (seqNum > seqSize)
    //End of file
    {
      break;
    }
    else if (pread(filefd, buff, LOADSIZE, LOADSIZE*(seqNum-1)) < 0)
    {
      fprintf(stderr, "Error reading file");
    }
    else
    {
      window[i].type = DATA;
      window[i].seqSize = seqSize;
      window[i].seqNumber = seqNum;
      strCopy(window[i].name, fileName);
      memcpy((void *)window[i].load, (void *) buff, LOADSIZE);
    }
  }
}

int receiveWindow(int sockfd, struct sockaddr *recv_addr, socklen_t addrlen, struct Packet window[])
{
  if (recvfrom(sockfd, (void *) window, sizeof(window[0]) * WINDOWSIZE, 0, recv_addr, &addrlen) < 0) {
      error("Couldn't receive request");
  }

  int i =0;
  int numAcks = 0;
  for (i=0; i<WINDOWSIZE; i++)
  {
    if (window[i].type == ACK)
    {
      printReceivePacket(&window[i]);
      numAcks++;
    }
    else if (window[i].type == CORRACK)
    {
      printReceivePacket(&window[i]);
    }
  }
  return numAcks;
}