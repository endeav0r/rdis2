#include "function.h"

#include <stdlib.h>
#include <string.h>

static const struct _object function_object = {
    (void     (*) (void *))                     function_delete, 
    (void *   (*) (const void *))               function_copy,
    (int      (*) (const void *, const void *)) function_cmp,
    NULL
};


struct _function * function_create (uint64_t address,
                                    const struct _graph * graph,
                                    const char * name)
{
    struct _function * function;

    function = (struct _function *) malloc(sizeof(struct _function));
    function->object  = &function_object;
    function->address = address;
    function->graph   = object_copy(graph);

    if (name == NULL)
        function->name = NULL;
    else
        function->name = strdup(name);
    return function;
}


void function_delete (struct _function * function)
{
    object_delete(function->graph);
    free(function);
}


struct _function * function_copy (const struct _function * function)
{
    return function_create(function->address, function->graph, function->name);
}


int function_cmp (const struct _function * lhs, const struct _function * rhs)
{
    if (lhs->address < rhs->address)
        return -1;
    else if (lhs->address > rhs->address)
        return 1;
    return 0;
}


void function_s_name (struct _function * function, const char * name)
{
    if (function->name != NULL)
        free((char *) function->name);

    if (name == NULL)
        function->name = NULL;
    else
        function->name = strdup(name);
}