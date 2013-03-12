#ifndef function_HEADER
#define function_HEADER

#include <inttypes.h>

#include "object.h"

struct _function {
    const struct _object * object;
    const char    * name;
    uint64_t        address;
    struct _graph * graph;
};


struct _function * function_create (uint64_t address,
                                    const struct _graph * graph,
                                    const char * name);
void               function_delete (struct _function * function);
struct _function * function_copy   (const struct _function * function);
int                function_cmp    (const struct _function * lhs,
                                    const struct _function * rhs);
void               function_s_name (struct _function * function,
                                    const char * name);
#endif