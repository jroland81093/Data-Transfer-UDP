
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent

#include <utils.h>

// Acts as the client.
void generateRequestPacket(char *packet, char *fileName);
int main(int argc, char *argv[])
{
    if (argc < 4) {
        error("Must provide <sender_hostname, sender_portno, filename>\n");
    }
    
    struct sockaddr_in sendAddr;                    //Sender/Server address
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

    //Send a request message
    
    char packet [PACKETSIZE];
    generateRequestPacket(packet, fileName);

    while(1) {
        //Send packet for file.
        if (sendToHelper(sockfd, fileName, strlen(fileName), 0, (struct sockaddr *) &sendAddr, sizeof(sendAddr)) < 0)
        {
            fprintf(stderr, "Failure sending request packet, trying again\n");
        }
        else
        {
            printf("Sent request for %s successfully\n", fileName);
            break;
        }
    }

    while(1) {
        //Begin reading back packets that the sender sends back for GBN Algorithm.
        break;
    }
    

    close(sockfd); //close socket
    return 0;
}

void generateRequestPacket(char *packet, char *fileName)
//Refer to utils.h for packet format.
{
    bzero(packet, PACKETSIZE);
    packet[TYP_START] = 1;
    writeChar(fileName, packet, NAM_START);
}