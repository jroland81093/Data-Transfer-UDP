/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h> /* for the waitpid() system call */
#include <signal.h> /* signal name macros, and the kill() prototype */
#include <time.h> //Used to pull time for file IO
#include <sys/types.h> //Used for last modified time 
#include <sys/stat.h> //Used for last modified time.
#include <string.h>

//Acts as the server.
void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void writeToConsoleAndClient(int sock);
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
  //Setup port and sockets.
  if (argc < 2) {
    error("Must provide <portno>");
  }

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
  /*
  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
      perror("sigaction");
      exit(1);
  }
  */

  //Wait for incoming messages.
  while (1) {
    printf("Waiting on port %d\n", portno);
    //Sender (server) messages from the receiver.
    //unsigned char buf[]
    //recvlen = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&recvAddr, &addrlen);
    if (recvlen < 0) {
      perror("Couldn't receive file request\n");
    }
    else {

    }


    //sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, addrlen)

    /*
    pid = fork(); //Create a new process.
    if (pid < 0) {
      error("Error on fork");
    }
    if (pid == 0) {
      printf
      exit(0);
    }
    */
  }
     return 0;
}