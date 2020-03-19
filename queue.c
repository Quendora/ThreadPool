#include <stdlib.h>
#include <string.h>
#include "queue.h"

void queueInit(Queue *q)
{
    q->sizeOfQueue = 0;
    q->head = q->tail = NULL;
}

int enqueue(Queue *q, void *data)
{
    node *newNode = (node *) malloc(sizeof(node));

    if (newNode == NULL)
    {
        return -1;
    }

    newNode->data = data;
    newNode->next = NULL;

    if (q->sizeOfQueue == 0)
    {
        q->head = q->tail = newNode;
    }
    else
    {
        q->tail->next = newNode;
        q->tail = newNode;
    }

    q->sizeOfQueue++;
    return 0;
}

void *dequeue(Queue *q)
{
    if (q->sizeOfQueue > 0)
    {
        node *temp = q->head;

        if (q->sizeOfQueue > 1)
        {
            q->head = q->head->next;
        }
        else
        {
            q->head = NULL;
            q->tail = NULL;
        }

        q->sizeOfQueue--;
        void *data = temp->data;
        free(temp);

        return data;
    }
    return NULL;
}

int getQueueSize(Queue *q)
{
    return q->sizeOfQueue;
}

void deleteElement(Queue *q, void *data)
{
    if (!q || !q->head) return;

    node *current = q->head;

    if (current->data == data)
    {
        q->sizeOfQueue--;
        q->head = current->next;

        if (q->sizeOfQueue == 0)
        {
            q->tail = NULL;
        }

        free(current);
        return;
    }

    node *previous = current;
    current = current->next;

    while (current != NULL)
    {
        if (current->data == data)
        {
            if (q->tail == current) q->tail = previous;

            previous->next = current->next;
            free(current);

            return;
        }

        previous = current;
        current = current->next;
    }
}