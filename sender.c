#include <strings.h>
#include <sys/wait.h> /* for the waitpid() system call */
#include <signal.h> /* signal name macros, and the kill() prototype */
#include <time.h> //Used to pull time for file IO
#include <sys/types.h> //Used for last modified time 
#include <sys/stat.h> //Used for last modified time.

#include <utils.h>

//Acts as the server.
void processRequestPacket(char *packet);
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
  //Declare all necessary structures
  struct sigaction sa;                          //Process handler
  struct sockaddr_in sendAddr;                  //Sender/Server address
  struct sockaddr_in recvAddr;                  //Receiver/Client address
  socklen_t addrlen = sizeof(recvAddr);         //Length of address
  int recvlen;                                  //Bytes received
  int portno = atoi(argv[1]);                   //Port number
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);  //Socket access

  if (sockfd < 1) {
    error("Error opening socket");
  }

  bzero((char *) &sendAddr, sizeof(sendAddr));
  sendAddr.sin_family = AF_INET;
  sendAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  sendAddr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *) &sendAddr, sizeof(sendAddr)) <0) {
    error("Error on binding");
  }

  //Kill zombie processes
  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
      perror("sigaction");
      exit(1);
  }

  //Wait for incoming messages.
  while (1) {
    //Sender (server) messages from the receiver.
      char packet [PACKETSIZE];
      bzero (packet, PACKETSIZE);
      recvlen = recvfrom(sockfd, packet, PACKETSIZE, 0, (struct sockaddr *)&recvAddr, &addrlen);
      if (recvlen < 0) {
        error("Couldn't receive file request");
      }
      else {
        processRequestPacket(packet); 
        printf("%s\n", packet);
        sendto(sockfd, packet, strlen(packet), 0, (struct sockaddr *)&recvAddr, addrlen);
      }
  }
     return 0;
}

void processRequestPacket(char *packet)
{
  printf("Process an incoming packet here!\n");
}