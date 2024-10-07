#pragma once
#include <pthread.h>
#include <stdlib.h>

typedef void* QueueData;

struct Queue {
    QueueData* data;
    size_t size;
    size_t capacity;
    size_t head;
    size_t tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

typedef struct Queue Queue;

static Queue* queue_create(size_t capacity) {
    Queue* queue = malloc(sizeof(Queue));
    queue->data = malloc(sizeof(QueueData) * capacity);
    queue->size = 0;
    queue->capacity = capacity;
    queue->head = 0;
    queue->tail = 0;

    queue->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    queue->cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;

    return queue;
}

static void queue_destroy(Queue* queue) {
    free(queue->data);
    free(queue);
}

static void queue_push(Queue* queue, const QueueData data) {
    pthread_mutex_lock(&queue->mutex);

    while (queue->size == queue->capacity) {
	pthread_cond_wait(&queue->cond, &queue->mutex);
    }

    queue->data[queue->tail] = data;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;

    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

static QueueData queue_pop(Queue* queue) {
    pthread_mutex_lock(&queue->mutex);
    while (queue->size == 0) {
	pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    QueueData data = queue->data[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size--;
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
    return data;
}

static size_t queue_size(Queue* queue) {
    pthread_mutex_lock(&queue->mutex);
    size_t size = queue->size;
    pthread_mutex_unlock(&queue->mutex);
    return size;
}

static size_t queue_capacity(Queue* queue) {
    pthread_mutex_lock(&queue->mutex);
    size_t capacity = queue->capacity;
    pthread_mutex_unlock(&queue->mutex);
    return capacity;
}

static int queue_empty(Queue* queue) {
    pthread_mutex_lock(&queue->mutex);
    int empty = queue->size == 0;
    pthread_mutex_unlock(&queue->mutex);
    return empty;
}

static int queue_full(Queue* queue) {
    pthread_mutex_lock(&queue->mutex);
    int full = queue->size == queue->capacity;
    pthread_mutex_unlock(&queue->mutex);
    return full;
}