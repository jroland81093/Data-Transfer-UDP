#include <strings.h>
#include <sys/wait.h> /* for the waitpid() system call */
#include <signal.h> /* signal name macros, and the kill() prototype */
#include <time.h> //Used to pull time for file IO
#include <sys/types.h> //Used for last modified time 
#include <sys/stat.h> //Used for last modified time.

#include <utils.h>

//Acts as the server.

void sendWindow(int filefd, int sockfd, int *lastAck, struct Packet *packet, const struct sockaddr *dest_addr, socklen_t addrlen);
void receiveWindow(int filefd, int sockfd, int *lastAck, struct Packet *packet, const struct sockaddr *dest_addr, socklen_t addrlen);

void processRequestPacket(struct Packet *packet);

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
    //Declare necessary structures for one file I/O.
    struct Packet packet;
    int lastAck = 0;

    //Read initial packet to start GBN algorithm.
    if (recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&recvAddr, &addrlen) < 0) {
        error("Couldn't receive file request");
    }
    printReceivePacket(&packet);

    //Setup Local File I/O to determine size, etc.
    struct stat fileStat;
    FILE *fp = fopen(packet.name, "rb");
    int filefd = fileno(fp);
    if (fp == NULL) {
      sendEndOfFile(sockfd, (struct sockaddr *)&recvAddr, addrlen);
      error("File not found\n");
    }
    if(fstat(filefd, &fileStat) < 0) {
      sendEndOfFile(sockfd, (struct sockaddr *)&recvAddr, addrlen);
      error("Could not determine file\n");
    }
    packet.seqSize = ceiling(fileStat.st_size, LOADSIZE);

    //Send until we receive an ACK of packet->seqSize.
    while(1)
    {
        sendWindow(filefd, sockfd, &lastAck, &packet, (struct sockaddr *)&recvAddr, addrlen);
        receiveWindow(filefd, sockfd, &lastAck, &packet, (struct sockaddr *)&recvAddr, addrlen);
        if (lastAck == packet.seqSize)
        {
          sendEndOfFile(sockfd, (struct sockaddr *)&recvAddr, addrlen);
          break;
        }
        break;
    }

  }
  //recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&recvAddr, &addrlen
  //sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&recvAddr, addrlen)
  return 0;
}

void sendWindow(int filefd, int sockfd, int *lastAck, struct Packet *packet, const struct sockaddr *dest_addr, socklen_t addrlen)
//Sends the next window of files to the receiver. Returns 0 if more file to be processed, 1 if we are done.
{
  //Check error cases here...
  int i;
  int result;
  char buff[LOADSIZE];
  bzero(buff, LOADSIZE);

  for (i=0; i<WINDOWSIZE; i++)
  {
    result = pread(filefd, buff, LOADSIZE, LOADSIZE*((*lastAck)+i));
    if((*lastAck)+i >= packet->seqSize)
    //End of file, we should return.
    {
      return;
    }
    else if (result < 0) {
      error("Error reading file");
    }
    else
    {
      packet->type = DATA;
      packet->seqNumber = (*lastAck)+i+1;
      packet->checkSum = checkSumHash(buff);
      memcpy((void *)packet->load, (void *) buff, LOADSIZE);

      if (sendToHelper(sockfd, packet, sizeof(*packet), 0, dest_addr, addrlen) < 0)
      {
          printf("DROPPED: ");
      }
      printSendPacket(packet);
    }
  }
}


void receiveWindow(int filefd, int sockfd, int *lastAck, struct Packet *packet, const struct sockaddr *dest_addr, socklen_t addrlen)
//Implement timeout operations here.
{


}