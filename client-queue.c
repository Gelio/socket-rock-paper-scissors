#include "client-queue.h"

void addClientToQueue(clientNode_t **head, int clientSocket, struct sockaddr_in *clientAddr, socklen_t *clientAddrLen)
{
    clientNode_t *newNode = malloc(sizeof(clientNode_t));
    if (newNode == NULL)
        ERR("malloc");

    newNode->clientSocket = clientSocket;
    newNode->clientAddr = *clientAddr;
    newNode->clientAddrLen = *clientAddrLen;
    newNode->next = *head;
    *head = newNode;
}

void safeAddClientToQueue(clientNode_t **head, int clientSocket, struct sockaddr_in *clientAddr, socklen_t *clientAddrLen, pthread_mutex_t *mutex)
{
    if (pthread_mutex_lock(mutex) != 0)
        ERR("pthread_mutex_lock");

    addClientToQueue(head, clientSocket, clientAddr, clientAddrLen);

    if (pthread_mutex_unlock(mutex) != 0)
        ERR("pthread_mutex_unlock");
}

clientNode_t *popClientFromQueue(clientNode_t **head)
{
    if (*head == NULL)
        return NULL;

    if ((*head)->next == NULL)
    {
        clientNode_t *nodeToReturn = *head;
        *head = NULL;
        return nodeToReturn;
    }

    clientNode_t *tail = *head,
        *prev = NULL;
    while (tail->next != NULL)
    {
        prev = tail;
        tail = tail->next;
    }

    prev->next = NULL;
    return tail;
}

clientNode_t *safePopClientFromQueue(clientNode_t **head, pthread_mutex_t *mutex)
{
    if (pthread_mutex_lock(mutex) != 0)
        ERR("pthread_mutex_lock");

    clientNode_t *client = popClientFromQueue(head);

    if (pthread_mutex_unlock(mutex) != 0)
        ERR("pthread_mutex_unlock");

    return client;
}

void clearClientQueue(clientNode_t **head)
{
    while (*head != NULL)
    {
        clientNode_t *next = (*head)->next;
        if (TEMP_FAILURE_RETRY(close(head->clientSocket)) < 0)
            ERR("close");
        free(*head);
        *head = next;
    }
}
