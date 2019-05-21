/***************************************/
/*              TCP server             */
/***************************************/
#include "hangmanpacket.h"
#define error(a, b) fprintf(stderr, a, b) // error 'function'

int fdServer, fdP1, fdP2, *p1, *p2;
struct sockaddr_in server;          // socket name (addr) of server
struct sockaddr_in client, client2; // socket name of client
int serverSize;                     // length of the socket addr. server
int clientSize, client2Size;        // length of the socket addr. client
int bytes;                          // length of buffer
char hangmanWord[10];
char successWord[10];
char *guess;
char *pPosition;

int lastEvent;
int fails = 0;
char triedCharacters[255];

void closeServer()
{
    packetData packet;
    setPacket(&packet, DISCONNECTED, "Server down");
    sendPacket(&packet, p1);
    sendPacket(&packet, p2);

    printf("Shut down server...\n");
    close(fdServer);
    close(fdP1);
    close(fdP2);
    exit(0);
}

int eventHandler(packetData *packet, int *client)
{
    switch (packet->eventCode)
    {
    case DISCONNECTED:
        closeServer();
        break;

    case CLIENT_CONNECTED:
        printf("Player connected. Id: %d\n", *client);
        break;

    case WAITING_FOR_PLAYER2:
        printf("Player %d is waiting for opponent...\n", *client);
        break;

    case PLAYER1_CHOOSING_WORD:
        break;

    case PLAYER1_WORD:
        printf("Player1 choose the word %s\n", packet->data);
        strcpy(hangmanWord, packet->data);
        printf("Player1 choose the word %s\n", hangmanWord);
        setPacket(packet, PLAYER1_WAIT_TILL_END, "Sorry");
        sendPacket(packet, p1);
        setPacket(packet, PLAYER2_GUESS, successWord);
        sendPacket(packet, p2);
        break;

    case PLAYER2_GUESS:
        strcpy(guess, packet->data);
        // TODO: Checking for reserved keywords

        char c = guess[0];
        pPosition = strchr(triedCharacters, c);
        if (!pPosition)
        {
            pPosition = strchr(hangmanWord, c);
            if (!pPosition)
            {
                fails++;
                setPacket(packet, PLAYER2_GUESS, successWord);
                sendPacket(packet, p1);
            }
        }
        break;

    default:
        printf("Player %d sent unknown event %d\n", *client, packet->eventCode);
        printf("Player %d sent unknown data %s\n", *client, packet->data);
        break;
    }

    return packet->eventCode;
}

int main(int argc, char *argv[])
{ // arg count, arg vector
    packetData *packet, tmpPacket;
    /* Declarations */
    int flags;    // rcv flags
    int rcvsize;  // received bytes
    int trnmsize; // transmitted bytes
    int err;      // error code
    char on;      //
    int event;

    memset(hangmanWord, 0, sizeof(hangmanWord));
    memset(successWord, 0, sizeof(successWord));

    p1 = &fdP1;
    p2 = &fdP2;
    packet = &tmpPacket;

    /* Initialization */
    on = 1;
    flags = 0;
    bytes = BUFSIZE;
    serverSize = sizeof server;
    clientSize = sizeof client;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT_NO);

    /**
     * TCP alapú socket létrehozása
     *
     * AF_INET interneten keresztül
     * SOCK_STREAM a TCP
     *
     */
    fdServer = socket(AF_INET, SOCK_STREAM, 0);
    if (fdServer < 0)
    {
        error("%s: Socket creation error\n", argv[0]);
        exit(1);
    }

    printf("Socket created id: %d\n", fdServer);

    /**
     * Ha hibásan lett leállítva a szerver, a bindelés miatt
     * még 2-4 percig foglalva lehet az a port amin futtatva lett.
     *
     * Ezzel a beállítással engedélyezzük, hogy újra használja
     * a portját a program.     *
     */
    setsockopt(fdServer, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof on);
    /**
     * Ne szakítsa meg a kapcsolatot a program
     */
    setsockopt(fdServer, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof on);

    /* Binding socket */
    err = bind(fdServer, (struct sockaddr *)&server, serverSize);
    if (err < 0)
    {
        error("%s: Cannot bind to the socket\n", argv[0]);
        exit(2);
    }

    /* Listening */
    err = listen(fdServer, 2);
    if (err < 0)
    {
        error("%s: Cannot listen to the socket\n", argv[0]);
        exit(3);
    }

    printf("Listening on port %d\n", PORT_NO);

    /* Accepting connection request */
    fdP1 = accept(fdServer, (struct sockaddr *)&client, &clientSize);
    if (fdP1 < 0)
    {
        error("%s: Cannot accept on socket\n", argv[0]);
        exit(4);
    }

    receivePacket(packet, p1);
    eventHandler(packet, p1);
    setPacket(packet, WAITING_FOR_PLAYER2, "Waiting for second player");
    sendPacket(packet, p1);

    fdP2 = accept(fdServer, (struct sockaddr *)&client2, &client2Size);
    if (fdP2 < 0)
    {
        error("%s: Cannot accept on socket\n", argv[0]);
        exit(5);
    }

    setPacket(packet, GAME_START, "Game start");
    sendPacket(packet, p1);
    sendPacket(packet, p2);

    for (;;)
    {

        if (strlen(hangmanWord) == 0)
        {
            receivePacket(packet, p1);
            event = eventHandler(packet, p1);
            setPacket(packet, CHOOSE_WORD, "Player1 choose a word!");
            sendPacket(packet, p1);
            setPacket(packet, PLAYER1_CHOOSING_WORD, "Player1 is choosing word");
            sendPacket(packet, p2);
        }
        else
        {
            receivePacket(packet, p1);
            event = eventHandler(packet, p1);
            receivePacket(packet, p2);
            event = eventHandler(packet, p2);
        }
    }
}
