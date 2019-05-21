
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define BUFSIZE 1024
#define PORT_NO 2001

#define DISCONNECTED 0
#define CLIENT_CONNECTED 1
#define WAITING_FOR_PLAYER2 2
#define UNKNOWN_EVENT 3
#define GAME_START 4
#define PLAYER1_CHOOSING_WORD 5
#define CHOOSE_WORD 6
#define PLAYER1_WORD 7
#define PLAYER2_GUESS 8
#define PLAYER1_WAIT_TILL_END 9
#define WAITING_FOR_SERVER 20

char buffer[BUFSIZE + 1]; // datagram dat buffer area

/**
 * Ilyen típusú csomagokat küld a szerver és a kliens egymásnak
 */
typedef struct
{
    // int 4 byte-ot foglal
    int eventCode;
    // a buffer méretétől 4-gyel kisebb helyre mehet bármilyen szöveg
    char data[BUFSIZE - 4];
} packetData;

int receivePacket(packetData *packet, int *fdClient)
{
    // printf("Received packet from %d\n", *fdClient);
    int received_bytes = recv(*fdClient, packet, sizeof(packetData), MSG_DONTWAIT);
    // printf("Packet event %d data %s\n", packet->eventCode, packet->data);

    if (received_bytes < 0)
    {
        // printf("Error client %d: %s\n", *fdClient, strerror(errno));
    }
    else if (received_bytes == 0)
    {
        fdClient = NULL;
    }

    return received_bytes;
}

void sendPacket(packetData *packet, int *fdClient)
{
    int sentBytes;

    // strcpy(buffer, serializePacket(buffer, packet));
    // printf("Sending to %d event %d\n", *fdClient, packet->eventCode);
    sentBytes = send(*fdClient, packet, sizeof(packetData), 0);
    if (sentBytes < 0)
    {
        printf("Couldn't send message to %d\n", *fdClient);
    }
}

void setPacket(packetData *packet, int eventCode, char *data)
{
    packet->eventCode = eventCode;
    strcpy(packet->data, data);
}
