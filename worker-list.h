#ifndef WORKER_LIST_H_INCLUDED
#define WORKER_LIST_H_INCLUDED

#include "common.h"

typedef struct workerThreadNode {
    pthread_t tid;
    int id;
    struct workerThreadNode *next;
} workerThreadNode_t;

void addWorkerThreadToList(workerThreadNode_t **head, pthread_t tid, int id);
void removeWorkerThreadFromList(workerThreadNode_t **head, pthread_t tid);
void safeRemoveWorkerThreadFromList(workerThreadNode_t **head, pthread_t tid, pthread_mutex_t *mutex);
void clearWorkerThreadList(workerThreadNode_t **head);
workerThreadNode_t *findWorkerThreadNode(workerThreadNode_t *head, pthread_t tid);
void joinWorkerThreads(workerThreadNode_t *head, pthread_mutex_t *listMutex);

#endif
