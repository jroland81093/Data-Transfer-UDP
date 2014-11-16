
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent

#include <utils.h>

// Acts as the client.
void setupFileTransfer(struct Packet *packet, char *fileName);
int processDataPacket(struct Packet *packet, int prevAck);

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
    struct Packet packet;
    int prevAck = 0;
    setupFileTransfer(&packet, fileName);
    if (sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *) &sendAddr, sizeof(sendAddr)) < 0)
    {
        error("Failure initiating file transfer, try again\n");
    }
    printSendPacket(&packet);

    //Wait for packets and send responses.
    while (1)
    {
        if (recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&sendAddr, &addrlen) < 0)
        {
            error("Error receiving data");
        }
        printReceivePacket(&packet);

        //IMPLEMENT PROCESSING OF DATA AND SENDING ALGORITHM HERE.
        /*
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

int processDataPacket(struct Packet *packet, int prevAck)
//Stuffs the packet with new appropriate data to ACK back to the sender
//Returns the sequence number that is sent back
{
    if (packet->type == END)
    {
        prevAck = -1;
    }
    else if (packet->seqNumber != prevAck+1)
    {
        printf("Received packet out of order!\n\n");
        packet->seqNumber = prevAck;
    }
    /* CheckSum isn't working ... 
    else if (checkSumHash(packet->load) != packet->checkSum)
    {
        printf("Received: %d, Expected %d\n\n", checkSumHash(packet->load), packet->checkSum);
        packet->seqNumber = prevAck;
    }
    */
    else
    {
        packet->seqNumber = prevAck+1;
    }

    packet->type = ACK;
    return prevAck;
}