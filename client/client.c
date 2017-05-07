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
    char letter;
    do {
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

    printf("Connecting to %d on port %d\n", serverHost, port);
    int socketDes = connectSocket(serverHost, port);
    printf("Connected\n");




    printf("Closing connection\n");
    if (TEMP_FAILURE_RETRY(close(socketDes)) < 0)
        ERR("close");
    printf("Terminating\n");
    return EXIT_SUCCESS;
}
