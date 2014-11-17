
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent

#include <utils.h>

// Acts as the client.
void setupFileTransfer(struct Packet *packet, char *fileName);
int receiveWindow(struct Packet window[], int prevAck);
void sendWindow(int sockfd, int lastAck, struct Packet window[], const struct sockaddr *dest_addr, socklen_t addrlen);
void sendWindowHelper(int sockfd, struct Packet window[], const struct sockaddr *dest_addr, socklen_t addrlen);

int main(int argc, char *argv[])
{
    if (argc < 4) {
        error("Must provide <sender_hostname, sender_portno, filename>\n");
    }
    
    struct sockaddr_in sendAddr;                    //Sender/Server address
    socklen_t addrlen = sizeof(sendAddr); 
    struct hostent *server;                         //Contains tons of information about the server's IP, etc.
    int portno = atoi(argv[2]);
    char *fileName = argv[3];
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);    //Socket access

    if (sockfd < 1) {
    error("Error opening socket");
    }
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &sendAddr, sizeof(sendAddr));
    sendAddr.sin_family = AF_INET; //initialize server's address
    bcopy((char *)server->h_addr, (char *)&sendAddr.sin_addr.s_addr, server->h_length); //Put host address into server address
    sendAddr.sin_port = htons(portno);

    /*ALGORITHM START */

    //Initiate file request.
    struct Packet requestPacket;
    int lastAck = 0;
    setupFileTransfer(&requestPacket, fileName);
    if (sendto(sockfd, &requestPacket, sizeof(requestPacket), 0, (struct sockaddr *) &sendAddr, sizeof(sendAddr)) < 0)
    {
        error("Failure initiating file transfer, try again\n");
    }
    printf("Sending request for %s\n", fileName);

    //Wait for packets and responses.
    struct Packet window[WINDOWSIZE];
    while (1)
    {
        if (recvfrom(sockfd, window, sizeof(window[0]) * WINDOWSIZE, 0, (struct sockaddr *)&sendAddr, &addrlen) < 0)
        {
            error("Error receiving data");
        }
        lastAck = receiveWindow(window, lastAck);
        sendWindow(sockfd, lastAck, window, (struct sockaddr *) &sendAddr, addrlen);
        break;

        /*
        printReceivePacket(&packet);

        //IMPLEMENT PROCESSING OF DATA AND SENDING ALGORITHM HERE.
        prevAck = processDataPacket(&packet, prevAck);
        if (prevAck == -1)
        {
            printf("File receival complete\n");
            break;
        }
        if (sendToHelper(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *) &sendAddr, sizeof(sendAddr)) < 0)
        {
            printf("DROPPED: ");
        }
        printSendPacket(&packet);
        */
        
    }
    close(sockfd);
    return 0;
}


void setupFileTransfer(struct Packet *packet, char *fileName)
//Initiates transfer of a new file.
{
    bzero((char *)packet, sizeof(*packet));
    packet->type = ACK; //Only time a receiver should be sending DATA is for a new request.
    packet->seqNumber = 0;
    strCopy(packet->name, fileName);
}

int receiveWindow(struct Packet window[], int prevAck)
{
    int i = 0;
    int lastAck = prevAck;
    int badAck = 0;

    for (i=0; i<WINDOWSIZE; i++)
    {
        //Output only significant packets
        if (window[i].seqNumber != 0)
        {
            if (window[i].type == LOSTACK || window[i].type == LOSTDATA)
            {
                badAck = 1;
            }
            /*
            else if (checkSumHash(window[i].load) != window[i].checkSum)  
            //DEBUG: Corruption won't work with the hash function.
            {
                fprintf(stderr, "CORRUPTED: ");
                printReceivePacket(&window[i]);
                badAck = 1;
            }
            */
            else if (badAck == 1)
            {
                fprintf(stderr, "IGNORING: ");
                printReceivePacket(&window[i]);
            }
            else
            {
                lastAck++;
                printReceivePacket(&window[i]);
            }
        }
    }
    fprintf(stderr, "\n");
    return lastAck;
}

void sendWindow(int sockfd, int lastAck, struct Packet window[], const struct sockaddr *dest_addr, socklen_t addrlen)
//Only send ACKS back
{
    int i=0;
    while (window[i].seqNumber <= lastAck && window[i].seqNumber != 0 && window[i].type != LOSTACK && window[i].type != LOSTDATA)
    {
        window[i].type = ACK;
        i++;
    }
    sendWindowHelper(sockfd, window, dest_addr, addrlen);
}

void sendWindowHelper(int sockfd, struct Packet window[], const struct sockaddr *dest_addr, socklen_t addrlen)
{
  int max = 100;
  int i = 0;

  for (i=0; i<WINDOWSIZE; i++)
  {
    //Only print significant messages to the screen.
    if (window[i].seqNumber != 0)
    {
      window[i].type = ACK;
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
