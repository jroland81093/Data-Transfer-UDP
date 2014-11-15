
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent

#include <utils.h>

// Acts as the client.
void initiateFileTransfer(struct Packet *sendPacket, char *fileName);
int processDataPacket(struct Packet *receivePacket, struct Packet *sendPacket, int prevAck);

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

    //Initiate file request.
    struct Packet sendPacket;
    struct Packet receivePacket;
    initiateFileTransfer(&sendPacket, fileName);
    if (sendto(sockfd, &sendPacket, sizeof(sendPacket), 0, (struct sockaddr *) &sendAddr, sizeof(sendAddr)) < 0)
    {
        error("Failure initiating file transfer, try again\n");
    }
    int prevAck = 0;


    /*
    ALGORITHM:
        Receive individual packets
        Extract their sequence numbers.
        If sequence number is out of sequence, send prevACK back. Otherwise, increment prevAck and send prevACK back.
    */
    while(1) {

        if (recvfrom(sockfd, &receivePacket, sizeof(receivePacket), 0, (struct sockaddr *)&sendAddr, &addrlen) < 0)
        {
            error("Error receiving data");
        }

        prevAck = processDataPacket(&receivePacket, &sendPacket, prevAck);
        if (sendToHelper(sockfd, &sendPacket, sizeof(sendPacket), 0, (struct sockaddr *) &sendAddr, sizeof(sendAddr)) < 0)
        {
            fprintf(stderr, "Failure sending request packet, trying again\n");
        }
        else
        {
            printf("Sent request for %s successfully\n", fileName);
            break;
        }
    }

    close(sockfd); //close socket
    return 0;
}


void initiateFileTransfer(struct Packet *sendPacket, char *fileName)
//Initiates transfer of a new file.
{
    bzero((char *)sendPacket, sizeof(*sendPacket));
    sendPacket->type = ACK; //Only time a receiver should be sending DATA is for a new request.
    sendPacket->seqNumber = 0;
    strCopy(sendPacket->name, fileName);
    printSendPacket(sendPacket);
}

int checkSumValue(struct Packet *receivePacket)
{
    return 0;
}

int processDataPacket(struct Packet *receivePacket, struct Packet *sendPacket, int prevAck)
//Stuffs the sendPacket with new appropriate data to ACK back to the sender
//Returns the sequence number that is sent back
{
    bzero((char *)sendPacket, sizeof(*sendPacket));
    sendPacket->type = ACK;
    sendPacket->seqSize = receivePacket->seqSize;
    sendPacket->checkSum = 0;
    strCopy(receivePacket->name, sendPacket->name);

    if ((receivePacket->seqNumber != prevAck+1) || checkSumValue(receivePacket) != receivePacket->checkSum)
    {
        sendPacket->seqNumber = prevAck;
    }
    else
    {
        sendPacket->seqNumber = prevAck+1;
    }
    return prevAck;
}