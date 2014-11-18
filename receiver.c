
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent

#include <utils.h>

// Acts as the client.

/*
void setupFileTransfer(struct Packet *packet, char *fileName);
int receiveWindow(struct Packet window[], int prevAck);
void sendWindow(int sockfd, int lastAck, struct Packet window[], const struct sockaddr *dest_addr, socklen_t addrlen);
void sendWindowHelper(int sockfd, struct Packet window[], const struct sockaddr *dest_addr, socklen_t addrlen);
*/

void sendFileTransfer(int sockfd, char* fileName, const struct sockaddr *dest_addr, socklen_t addrlen);
int receiveWindow(int sockfd, struct sockaddr *recv_addr, socklen_t addrlen, struct Packet window[], int prevAck);
void generateSendWindow(struct Packet window[], int lastAcked, int prevAck);

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
    sendFileTransfer(sockfd, fileName, (struct sockaddr *) &sendAddr, addrlen);
    struct Packet window[WINDOWSIZE];
    int lastAcked = 0;

    int i = 0;
    while(1)
    {
        int prevAck = lastAcked;
        lastAcked = receiveWindow(sockfd, (struct sockaddr *) &sendAddr, addrlen, window, prevAck);
        generateSendWindow(window, lastAcked, prevAck);
        sendWindow(sockfd, (struct sockaddr *) &sendAddr, addrlen, window);

        //TODO: Figure out when to break! After we successfully send the last ACK over, we should break.
    }

    close(sockfd);
    return 0;
}

void sendFileTransfer(int sockfd, char* fileName, const struct sockaddr *dest_addr, socklen_t addrlen)
//Initiate a new file transfer.
{
    struct Packet requestPacket;
    bzero((char *) &requestPacket, sizeof(requestPacket));
    requestPacket.seqNumber = 0;
    requestPacket.type = FILEREQ;
    strCopy(requestPacket.name, fileName);

    if (sendto(sockfd, (void *) &requestPacket, sizeof(requestPacket), 0, dest_addr, addrlen) < 0)
    {
        error("Failure initiating file transfer, try again\n");
    }
    printSendPacket(&requestPacket);
}

int receiveWindow(int sockfd, struct sockaddr *recv_addr, socklen_t addrlen, struct Packet window[], int prevAck)
{
    bzero((char *) window, sizeof(window[0]) * WINDOWSIZE);
    if (recvfrom(sockfd, window, sizeof(window[0]) * WINDOWSIZE, 0, recv_addr, &addrlen) < 0)
    {
        error("Error receiving data");
    }

    int i = 0;
    int ackNumber = prevAck;
    int flag = 0;

    for (i=0; i<WINDOWSIZE; i++)
    {
        if (window[i].type == LOSTDATA)
        {
            flag = 1;
        }
        /*
        else if (checkSumHash(window[i].load) != window[i].checkSum)  
        //DEBUG: Corruption won't work with the hash function.
        {
            fprintf(stderr, "CORRUPTED: ");
            printReceivePacket(&window[i]);
            flag = 1;
        }
        */
        else if (flag == 1)
        {
            fprintf(stderr, "IGNORING: ");
            printReceivePacket(&(window[prevAck+i]));
        }
        else
        {
            //Do some buffering or writing to a local file.
            printReceivePacket(&(window[prevAck+i]));
            ackNumber++;
        }
    }
    return ackNumber;
}

void generateSendWindow(struct Packet window[], int lastAcked, int prevAck)
{
    int i;

    //Get all of the valid data.
    for(i=0; i<(lastAcked - prevAck); i++)
    {
        window[i].type = ACK;
    }
    for(i=(lastAcked - prevAck); i< WINDOWSIZE; i++)
    {
        window[i].type = LOSTACK;
    }

}