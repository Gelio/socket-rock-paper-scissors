#include "common.h"
#include "worker-thread.h"

#define PLAY_UNKNOWN    0
#define PLAY_ROCK       1
#define PLAY_PAPER      2
#define PLAY_SCISSORS   3
#define PLAY_ERROR      4

void setWorkerThreadSignalHandling(sigset_t *previousMask)
{
    sigset_t blockMask = prepareBlockMask();
    if (pthread_sigmask(SIG_BLOCK, &blockMask, previousMask) < 0)
        ERR("pthread_sigmask");
}

void workerThreadCleanup(workerThreadArgs_t *workerArgs, sigset_t *previousMask)
{
    if (!shouldQuit)
        safeRemoveWorkerThreadFromList(workerArgs->serverData->workerThreadsList, pthread_self(), workerArgs->serverData->workerThreadsListMutex);


    free(workerArgs);
    if (pthread_sigmask(SIG_SETMASK, previousMask, NULL) < 0)
        ERR("pthread_sigmask");
}

void waitForBothPlayers(int client1Socket, int client2Socket)
{
    printf("Waiting for both players to provide input\n");
    int maxSocketFd = client1Socket > client2Socket ? client1Socket : client2Socket;
    fd_set readFds;
    do {
        FD_ZERO(&readFds);
        FD_SET(client1Socket, &readFds);
        FD_SET(client2Socket, &readFds);
    } while (select(maxSocketFd + 1, &readFds, NULL, NULL, NULL) < 2);  // select will return 2 if both sockets are ready for reading
}

int readClientPlay(int clientSocket)
{
    char letter;
    if (bulkRead(clientSocket, &letter, 1) < 1)
        return PLAY_ERROR;
    switch (letter)
    {
        case 'r':
            return PLAY_ROCK;

        case 'p':
            return PLAY_PAPER;

        case 's':
            return PLAY_SCISSORS;

        default:
            return PLAY_UNKNOWN;
    }
}

int getPlayerInput(int clientSocket)
{
    int playerInput;
    do {
        char message[] = "Your input [rps]: ";
        if (bulkWrite(clientSocket, message, strlen(message)) <= 0)
            return PLAY_ERROR;
        playerInput = readClientPlay(clientSocket);
    } while (playerInput == PLAY_UNKNOWN);

    return playerInput;
}

int comparePlays(int play1, int play2)
{
    if (play1 == play2)
        return 0;
    if (play1 == PLAY_ROCK)
    {
        if (play2 == PLAY_SCISSORS)
            return 1;
        else    // play2 == PLAY_PAPER
            return 2;
    }
    else if (play1 == PLAY_SCISSORS)
    {
        if (play2 == PLAY_ROCK)
            return 2;
        else    // play2 == PLAY_PAPER
            return 1;
    }
    else    // play1 == PLAY_PAPER
    {
        if (play2 == PLAY_ROCK)
            return 1;
        else    // play2 == PLAY_SCISSORS
            return 2;
    }
}

void *workerThread(void *args)
{
    workerThreadArgs_t *workerArgs = (workerThreadArgs_t*)args;
    struct serverData *serverData = workerArgs->serverData;
    sigset_t previousMask;
    setWorkerThreadSignalHandling(&previousMask);
    printf("[%d] started\n", workerArgs->id);

    if (pthread_mutex_lock(serverData->clientQueueMutex) != 0)
        ERR("pthread_mutex_lock");
    clientNode_t *client1 = popClientFromQueue(serverData->clientQueue),
        *client2 = popClientFromQueue(serverData->clientQueue);
    if (pthread_mutex_unlock(serverData->clientQueueMutex) != 0)
        ERR("pthread_mutex_unlock");

    int roundNumber = 0;
    int client1Points = 0,
        client2Points = 0;
    while (roundNumber < serverData->roundsCount)
    {
        roundNumber++;
        printf("[%d] round %d, fight!\n", workerArgs->id, roundNumber);
        waitForBothPlayers(client1->clientSocket, client2->clientSocket);

        int player1Input = getPlayerInput(client1->clientSocket);
        if (player1Input == PLAY_ERROR)
            goto disconnect;
        int player2Input = getPlayerInput(client2->clientSocket);
        if (player2Input == PLAY_ERROR)
            goto disconnect;

        int roundResult = comparePlays(player1Input, player2Input);
        if (roundResult == 0)
            printf("[%d] draw\n", workerArgs->id);
        else if (roundResult == 1)
        {
            printf("[%d] player 1 won\n", workerArgs->id);
            client1Points++;
        }
        else    // roundResult == 2
        {
            printf("[%d] player 2 won\n", workerArgs->id);
            client2Points++;
        }
    }

    if (client1Points == client2Points)
        printf("[%d] game ended with a draw (%d points)\n", workerArgs->id, client1Points);
    else if (client1Points > client2Points)
        printf("[%d] player 1 won (%d vs %d points)\n", workerArgs->id, client1Points, client2Points);
    else    // (client1Points < client2Points)
        printf("[%d] player 2 won (%d vs %d points)\n", workerArgs->id, client2Points, client1Points);

disconnect:
    printf("[%d] closing connection to the clients\n", workerArgs->id);
    if (TEMP_FAILURE_RETRY(close(client1->clientSocket)) < 0)
        ERR("close");
    free(client1);

    if (TEMP_FAILURE_RETRY(close(client2->clientSocket)) < 0)
        ERR("close");
    free(client2);

    int threadId = workerArgs->id;
    workerThreadCleanup(workerArgs, &previousMask);
    printf("[%d] terminated\n", threadId);
    return NULL;
}
