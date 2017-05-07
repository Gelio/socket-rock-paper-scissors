#include "common.h"

#include <sys/select.h>
#include <semaphore.h>

#include "worker-list.h"
#include "client-queue.h"
#include "worker-thread.h"
#include "server-helpers.h"

#define BACKLOG 10

void acceptNewClient(struct serverData *serverData, int threadId)
{
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(struct sockaddr_in);
    int clientSocket = TEMP_FAILURE_RETRY(accept(serverData->serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen));
    if (clientSocket < 0)
        ERR("accept");

    if (pthread_mutex_lock(serverData->clientQueueMutex) != 0)
        ERR("pthread_mutex_lock");

    addClientToQueue(serverData->clientQueue, clientSocket, &clientAddr, &clientAddrLen);

    if (threadId > 0)
    {
        pthread_t tid;
        workerThreadArgs_t *workerArgs = malloc(sizeof(workerThreadArgs_t));
        if (workerArgs == NULL)
            ERR("malloc");
        workerArgs->id = threadId;
        workerArgs->serverData = serverData;
        if (pthread_create(&tid, NULL, workerThread, (void*)workerArgs) != 0)
            ERR("pthread_create");

        addWorkerThreadToList(serverData->workerThreadsList, tid, threadId);
    }

    if (pthread_mutex_unlock(serverData->clientQueueMutex) != 0)
        ERR("pthread_mutex_unlock");
}

void serverLoop(struct serverData *serverData)
{
    int nextThreadId = 1;
    int createGame = 0; // if not zero (if false), upon a new connection a new game will be created
    // (basically it means there is one connected client waiting)

    fd_set readFds;
    while (!shouldQuit)
    {
        FD_ZERO(&readFds);
        FD_SET(serverData->serverSocket, &readFds);
        if (pselect(serverData->serverSocket + 1, &readFds, NULL, NULL, NULL, serverData->previousMaskWithSigPipe) < 0)
        {
            if (errno == EINTR)
                continue;
            ERR("pselect");
        }


        acceptNewClient(serverData, createGame ? nextThreadId++ : 0);
        if (createGame)
            printf("[SERVER] client connected, creating game\n");
        else
            printf("[SERVER] client connected\n");
        createGame = !createGame;
    }
}

int main(int argc, char **argv)
{
    int roundsCount;
    int16_t port;
    parseArguments(argc, argv, &roundsCount, &port);

    printf("[SERVER] Initialization\n");
    workerThreadNode_t *workerThreadsList = NULL;
    pthread_mutex_t workerThreadsListMutex = PTHREAD_MUTEX_INITIALIZER;
    clientNode_t *clientQueue = NULL;
    pthread_mutex_t clientQueueMutex = PTHREAD_MUTEX_INITIALIZER;
    sigset_t previousMask, previousMaskWithSigPipe;
    setSignalHandling(&previousMask, &previousMaskWithSigPipe);

    struct serverData serverData;
    serverData.serverSocket = makeSocket(AF_INET, SOCK_STREAM);
    serverData.previousMask = &previousMask;
    serverData.previousMaskWithSigPipe = &previousMaskWithSigPipe;
    serverData.workerThreadsList = &workerThreadsList;
    serverData.workerThreadsListMutex = &workerThreadsListMutex;
    serverData.clientQueue = &clientQueue;
    serverData.clientQueueMutex = &clientQueueMutex;
    serverData.roundsCount = roundsCount;

    bindSocketAndListen(serverData.serverSocket, port, BACKLOG);

    printf("[SERVER] Listening on port %d\n", port);
    serverLoop(&serverData);

    performCleanup(&serverData);
    printf("[SERVER] Terminated\n");
    return EXIT_SUCCESS;
}
