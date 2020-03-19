#ifndef ASYNC_QUEUE_H
#define ASYNC_QUEUE_H

typedef struct Node
{
    void *data;
    struct Node *next;
} node;

typedef struct QueueList
{
    int sizeOfQueue;
    node *head;
    node *tail;
} Queue;

void queueInit(Queue *q);

int enqueue(Queue *, void *);

void* dequeue(Queue *);

int getQueueSize(Queue *);

void deleteElement(Queue *q, void *data);

#endif //ASYNC_QUEUE_H
