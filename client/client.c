#include "client-common.h"

#define PORT_MIN 1000
#define BUFFER_SIZE 1

void usage(char *fileName)
{
    fprintf(stderr, "Usage: %s hostname port\n", fileName);
    fprintf(stderr, "port > %d\n", PORT_MIN);
    exit(EXIT_FAILURE);
}

void parseArguments(int argc, char **argv, char **serverHost, char **port)
{
    if (argc != 3)
        usage(argv[0]);

    *serverHost = argv[1];

    *port = argv[2];
    int portNum = atoi(*port);
    if (portNum <= PORT_MIN)
        usage(argv[0]);
}

void playRound(int socketDes)
{
    char letter = '0';
    do {
        if (letter != '\n')
            printf("Your input [rps]: ");
        letter = (char)getc(stdin);
    } while (letter != 'r' && letter != 'p' && letter != 's');

    if (bulkWrite(socketDes, &letter, 1) <= 0)
        ERR("sending letter to server");
}

int main(int argc, char **argv)
{
    char *serverHost,
        *port;
    parseArguments(argc, argv, &serverHost, &port);

    printf("Connecting to %s on port %s\n", serverHost, port);
    int socketDes = connectSocket(serverHost, port);
    printf("Connected, waiting for the game to start\n");

    int matchInProgress = 1;
    while (matchInProgress)
    {
        char buffer[BUFFER_SIZE];
        ssize_t bytesRead = bulkRead(socketDes, buffer, 1);
        if (bytesRead < 0)
            ERR("read from socket");
        else if (bytesRead == 0)
        {
            printf("Connection terminated by the server\n");
            break;
        }

        switch (buffer[0])
        {
            case 'w':
                printf("You won this round!\n");
                break;

            case 'W':
                printf("You won the match\n");
                matchInProgress = 0;
                break;

            case 'l':
                printf("You lost this round\n");
                break;

            case 'L':
                printf("You lost the match\n");
                matchInProgress = 0;
                break;

            case 'g':
                printf("New round\n");
                playRound(socketDes);
                break;

            case 'r':
                printf("Wrong input, retry\n");
                playRound(socketDes);
                break;

            case 'd':
                printf("Round ended in a draw\n");
                break;

            case 'D':
                printf("Match ended in a draw\n");
                matchInProgress = 0;
                break;

            default:
                printf("Unknown message from the server: %s\n", buffer);
                break;
        }
    }

    printf("Closing connection\n");
    if (TEMP_FAILURE_RETRY(close(socketDes)) < 0)
        ERR("close");
    printf("Terminating\n");
    return EXIT_SUCCESS;
}

