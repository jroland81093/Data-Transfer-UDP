
#include <netinet/in.h>
#include <netdb.h>

#include <utils.h>

// Acts as the client.

void sendFileTransfer(int sockfd, char* fileName, const struct sockaddr *dest_addr, socklen_t addrlen);
int receiveWindow(int sockfd, struct sockaddr *recv_addr, socklen_t addrlen, struct Packet window[], int prevAck, int fd, int *seqSize);
void writeToFileSystem(int fd, struct Packet packet);
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
    FILE *fp = fopen("OUTPUT", "wb");
    int filefd = fileno(fp);

    int seqSize = -1;
    while(1)
    {
        int prevAck = lastAcked;
        lastAcked = receiveWindow(sockfd, (struct sockaddr *) &sendAddr, addrlen, window, prevAck, filefd, &seqSize);
        generateSendWindow(window, lastAcked, prevAck);
        sendWindow(sockfd, (struct sockaddr *) &sendAddr, addrlen, window);

        //break when last packet is sent (when its sequence number matches total sequence size)
        if (lastAcked == seqSize)
        {
            break;
        }
    }

    fclose(fp);
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

int receiveWindow(int sockfd, struct sockaddr *recv_addr, socklen_t addrlen, struct Packet window[], int prevAck, int fd, int *seqSize)
{
    bzero((char *) window, sizeof(window[0]) * WINDOWSIZE);
    //Zero out the receiving window before receiving data.

    if (recvfrom(sockfd, window, sizeof(window[0]) * WINDOWSIZE, 0, recv_addr, &addrlen) < 0)
    {
        error("Error receiving data");
    }

    if (window[0].type == BADFILE)
    {
        error("File not found!");
    }

    int i = 0;
    int ackNumber = prevAck;
    int flag = 0;

    for (i=0; i<WINDOWSIZE; i++)
    {
        if (window[i].seqNumber > 0 && window[i].seqNumber <= window[i].seqSize && window[i].type > 0)
            //If this is a valid packet
        {
            //DEBUG: If the packet received has already been received, truncate it?
            if (window[i].type == LOSTDATA)
            {
                flag = 1;
            }
            else if (window[i].type == CORRDATA && flag == 0)
            {
                flag = 1;
                printReceivePacket(&window[i]);
            }
            else if (flag == 1)
            {
                printf("IGNORING: ");
                printReceivePacket(&(window[i]));
            }
            else if (window[i].type == DATA)
            {
                *seqSize = window[i].seqSize;
                writeToFileSystem(fd, window[i]);
                printReceivePacket(&(window[i]));
                ackNumber++;
            }
        }
    }
    return ackNumber;
}

void writeToFileSystem(int fd, struct Packet packet)
{
    if (packet.seqNumber == packet.seqSize)
    {
        int size = 0;
        while (packet.load[size] != 0)
        {
            size++;
        }
        pwrite(fd, (void *) packet.load, size, (packet.seqNumber - 1 ) * LOADSIZE);
    }
    else
    {
        pwrite(fd, (void *) packet.load, LOADSIZE, (packet.seqNumber - 1 ) * LOADSIZE);
    }
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