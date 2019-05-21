/***************************************/
/*             TCP client              */
/***************************************/

#include "hangmanpacket.h"

#define error(a, b) fprintf(stderr, a, b)

packetData *pck, tmpPacket;
int *server;
char hangmanWord[10];
char myWord[10];
int lastEvent;
char *guess;

void closeClient()
{
    packetData packet;
    setPacket(&packet, DISCONNECTED, "Client disconnect");
    sendPacket(&packet, server);

    printf("Closing client...\n");
    close(*server);
}

void sigTermHandler()
{
    closeClient();
}

void drawGame()
{
}

int eventHandler(packetData *packet)
{
    int currentEventCode = packet->eventCode;

    if (lastEvent != currentEventCode)
    {
        switch (packet->eventCode)
        {
        case DISCONNECTED:
            closeClient();
            break;

        case CLIENT_CONNECTED:
            printf("I got connected client wtf\n");
            break;

        case WAITING_FOR_PLAYER2:
            printf("Waiting for player2...\n");
            setPacket(packet, WAITING_FOR_PLAYER2, "Waiting for player2");
            sendPacket(packet, server);
            break;

        case GAME_START:
            printf("Both players are ready, start game...\n");
            break;

        case PLAYER1_CHOOSING_WORD:
            printf("Another player is guessing a word...\n");
            break;

        case CHOOSE_WORD:
            if (strlen(hangmanWord) == 0)
            {
                printf("I have to choose a word:");
                scanf("%s", hangmanWord);
                printf("The chosen word %s\n", hangmanWord);
                setPacket(packet, PLAYER1_WORD, hangmanWord);
                sendPacket(packet, server);
            }
            break;

        case PLAYER2_GUESS:
            strcpy(myWord, packet->data);
            printf("%s", myWord);
            drawGame();
            printf("Guess a character:");
            scanf("%s", guess);
            setPacket(packet, PLAYER2_GUESS, guess);
            sendPacket(packet, server);
            lastEvent = WAITING_FOR_SERVER;
            break;

        case PLAYER1_WAIT_TILL_END:
            drawGame();
            break;

        case WAITING_FOR_SERVER:
            sleep(1);
            break;

        default:
            printf("Server send unknown event %d data %s\n", packet->eventCode, packet->data);
            break;
        }

        lastEvent = currentEventCode;
    }

    return currentEventCode;
}

int main(int argc, char *argv[])
{ // arg count, arg vector
    signal(SIGTERM, sigTermHandler);

    /* Declarations */
    int fdServer;                  // socket endpt
    int flags;                     // rcv flags
    struct sockaddr_in serverAddr; // socket name (addr) of server
    struct sockaddr_in clientAddr; // socket name of client
    int serverSize;                // length of the socket addr. server
    int clientSize;                // length of the socket addr. client
    int bytes;                     // length of buffer
    int rcvsize;                   // received bytes
    int trnmsize;                  // transmitted bytes
    int err;                       // error code
    int ip;                        // ip address
    int event;
    char on;              //
    char server_addr[16]; // server address

    /* Initialization */
    on = 1;
    flags = 0;
    serverSize = sizeof serverAddr;
    clientSize = sizeof clientAddr;

    pck = &tmpPacket;
    server = &fdServer;

    if (argc < 2)
    {
        strcpy(server_addr, "127.0.0.1");
    }
    else
    {
        sprintf(server_addr, "%s", argv[1]);
    }
    ip = inet_addr(server_addr);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = ip;
    serverAddr.sin_port = htons(PORT_NO);

    /* Creating socket */
    fdServer = socket(AF_INET, SOCK_STREAM, 0);
    if (fdServer < 0)
    {
        error("%s: Socket creation error.\n", argv[0]);
        exit(1);
    }

    printf("Server Id %d\n", fdServer);

    /* Setting socket options */
    setsockopt(fdServer, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof on);
    setsockopt(fdServer, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof on);

    printf("Connecting to server %s:%d\n", server_addr, PORT_NO);

    /* Connecting to the server */
    err = connect(fdServer, (struct sockaddr *)&serverAddr, serverSize);
    if (err < 0)
    {
        error("%s: Cannot connect to the server.\n", argv[0]);
        exit(2);
    }

    setPacket(pck, CLIENT_CONNECTED, "HELLO");
    sendPacket(pck, server);
    printf("HELLO packet sent!\n");

    // Waiting for second player if any
    for (;;)
    {
        receivePacket(pck, server);
        event = eventHandler(pck);

        if (event == GAME_START)
        {
            break;
        }
    }

    // Infinite loop
    for (;;)
    {
        sleep(1);
        receivePacket(pck, server);
        event = eventHandler(pck);

        if (tmpPacket.eventCode == DISCONNECTED || !server)
        {
            printf("Server disconnected, stopping game...\n");
            break;
        }
    }

    /* Closing sockets and quit */
    closeClient();
    exit(0);
}
