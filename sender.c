#include <strings.h>
#include <sys/wait.h> /* for the waitpid() system call */
#include <signal.h> /* signal name macros, and the kill() prototype */
#include <time.h> //Used to pull time for file IO
#include <sys/types.h> //Used for last modified time 
#include <sys/stat.h> //Used for last modified time.

#include <utils.h>

//Acts as the server.
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
  //Declare all necessary structures
  struct sigaction sa;                          //Process handler
  struct sockaddr_in sendAddr;                  //Sender/Server address
  struct sockaddr_in recvAddr;                  //Receiver/Client address
  socklen_t addrlen = sizeof(recvAddr);         //Length of address
  int recvlen;                                  //Bytes received
  int portno = atoi(argv[1]);                   //Port number
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);  //Socket access

  if (sockfd < 0) {
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

  //Read initial packet to start GBN algorithm
  struct Packet packet;
  recvlen = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&recvAddr, &addrlen);
  printReceivePacket(&packet);
  if (recvlen < 0) {
        error("Couldn't receive file request");
  }

  int lastAck = 0;
  int pid = 0;

    /*
    ALGORITHM:
      Begin timer. Run the C select() function?
      Run for loop on WINDOWSIZE elements to send all.
      As individual ACKS come in, increment.
      Wait for either ACK(lastAck + WINDOWSIZE) to come back, or timeout.
    */
  while (1) {
      if (recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&recvAddr, &addrlen) < 0) {
        error("Couldn't receive file request");
      }
      else {
        lastAck++; 
        processRequestPacket(&packet); 
        sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&recvAddr, addrlen);
      }
  }
     return 0;
}


void processRequestPacket(struct Packet *packet)
{
  //Need to figure out an algorithm to send WINDOWSIZE packets back to the receiver.
}