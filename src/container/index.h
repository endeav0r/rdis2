#ifndef index_HEADER
#define index_HEADER

// many times we want to store just a uint64_t in a tree/list/graph
// this allows us to do that

#include <inttypes.h>

#include "object.h"

struct _index {
    const struct _object * object;
    uint64_t index;
};


struct _index * index_create      (uint64_t index);
void		    index_delete      (struct _index * index);
struct _index * index_copy        (const struct _index * index);
int 			index_cmp         (const struct _index * lhs, const struct _index * rhs);

#endif