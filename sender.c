#include <strings.h>
#include <sys/wait.h> /* for the waitpid() system call */
#include <signal.h> /* signal name macros, and the kill() prototype */
#include <time.h> //Used to pull time for file IO
#include <sys/types.h> //Used for last modified time 
#include <sys/stat.h> //Used for last modified time.

#include <utils.h>

//Acts as the server.

/*
void sendWindow(int filefd, int sockfd, int lastAck, struct Packet *packet, const struct sockaddr *dest_addr, socklen_t addrlen);
void sendWindowHelper(int sockfd, struct Packet window[], const struct sockaddr *dest_addr, socklen_t addrlen);
int receiveWindow(struct Packet window[], int lastAcked);

*/

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

  //Kill zombie processes (Only need if we are multitherading)
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
      perror("sigaction");
      exit(1);
  }

  /* ALGORITHM START */
  
  //Infinite loop after the server is bound to keep it running for multiple requests.
  while (1)
  {
    struct Packet window[WINDOWSIZE];
    char fileName[NAMESIZE];
    int filefd;
    int seqSize = receiveFileTransfer(sockfd, (struct sockaddr *) &recvAddr, addrlen, window, fileName, &filefd);
    int lastAcked = 0;

    //Loop until we receive the final ACK
    while (1)
    {
      int prevAck = lastAcked;
      generateSendWindow(window, fileName, filefd, seqSize, lastAcked);
      sendWindow(sockfd, (struct sockaddr *) &recvAddr, addrlen, window);
      lastAcked += receiveWindow(sockfd, (struct sockaddr *) &recvAddr, addrlen, window);

      if (lastAcked == seqSize)
      {
        fprintf(stderr, "RDT is done!\n");
        break;
      }
      else if (lastAcked == prevAck)
      //Timeout and resent
      {
        fprintf(stderr, "TIMEOUT!\n");
      }
    }
    fclose(fdopen(filefd, "rb"));
  }
  //recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&recvAddr, &addrlen
  //sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&recvAddr, addrlen)
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
    //DEBUG: If the file isn't there, we get a seg fault?
    FILE *fp = fopen(requestPacket.name, "rb");
    if (fp == NULL) {
      //sendEndOfFile(sockfd, (struct sockaddr *)&recvAddr, addrlen);
      error("File not found");
    }
    int filed = fileno(fp);
    if(fstat(filed, &fileStat) < 0) {
      //sendEndOfFile(sockfd, (struct sockaddr *)&recvAddr, addrlen);
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
    *filefd = filed;
    return numSequences;
}

void generateSendWindow(struct Packet window[], char fileName[], int filefd, int seqSize, int lastAcked)
//Sends the next window of files to the receiver.
{
  int i;
  int result;
  char buff[LOADSIZE];
  bzero(buff, LOADSIZE);
  bzero((char *)window, sizeof(window[0]) * WINDOWSIZE);

  for (i=0; i<WINDOWSIZE; i++)
  {
    int seqNum = lastAcked+i+1;
    result = pread(filefd, buff, LOADSIZE, LOADSIZE*(seqNum-1));

    if (seqNum > seqSize)
      //End of file
    {
      break;
    }
    else if (result < 0)
    {
      fprintf(stderr, "Error reading file");
    }
    else
    {
      window[i].type = DATA;
      window[i].seqSize = seqSize;
      window[i].seqNumber = seqNum;
      window[i].checkSum = checkSumHash(buff);
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

  int i=0;

  while (window[i].type == ACK)
  {
    printReceivePacket(&window[i]);
    i++;
  }
  return i;
}