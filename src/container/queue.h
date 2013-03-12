#ifndef queue_HEADER
#define queue_HEADER

#include "object.h"

#include <stdlib.h>

struct _queue_it {
    void * data;
    struct _queue_it * next;
};

struct _queue {
    const struct _object * object;
    size_t size;
    struct _queue_it * front;
    struct _queue_it * back;
};

struct _queue * queue_create ();
void            queue_delete (struct _queue * queue);
struct _queue * queue_copy   (const struct _queue * queue);

void            queue_push   (struct _queue * queue, const void * data);
void            queue_pop    (struct _queue * queue);
void *          queue_peek   (const struct _queue * queue);

struct _queue_it * queue_it_create (const void * data);

#endif