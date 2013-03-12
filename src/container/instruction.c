#include "instruction.h"

#include <string.h>

static const struct _object ins_object = {
    (void   (*) (void *))                     ins_delete, 
    (void * (*) (const void *))               ins_copy,
    (int    (*) (const void *, const void *)) ins_cmp,
    NULL
};

static const struct _object ins_value_object = {
    (void   (*) (void *))       ins_value_delete, 
    (void * (*) (const void *)) ins_value_copy,
    NULL,
    NULL
};

struct _ins * ins_create  (uint64_t address,
                           const uint8_t * bytes,
                           size_t size,
                           const char * description,
                           const char * comment)
{
    struct _ins * ins;

    ins = (struct _ins *) malloc(sizeof(struct _ins));

    ins->object  = &ins_object;
    ins->address = address;

    ins->bytes = malloc(size);
    memcpy(ins->bytes, bytes, size);

    ins->size = size;

    if (description == NULL)
        ins->description = NULL;
    else
        ins->description = strdup(description);

    if (comment == NULL)
        ins->comment = NULL;
    else
        ins->comment = strdup(comment);

    ins->successors = list_create();

    return ins;
}


void ins_delete (struct _ins * ins)
{
    free(ins->bytes);
    if (ins->description != NULL)
        free(ins->description);
    if (ins->comment != NULL)
        free(ins->comment);
    object_delete(ins->successors);
    free(ins);
}


struct _ins * ins_copy (const struct _ins * ins)
{
    struct _ins * new_ins = ins_create(ins->address,
                                       ins->bytes,
                                       ins->size,
                                       ins->description,
                                       ins->comment);
    object_delete(new_ins->successors);
    new_ins->successors = object_copy(ins->successors);

    return new_ins;
}


int ins_cmp (const struct _ins * lhs, const struct _ins * rhs)
{
    if (lhs->address < rhs->address)
        return -1;
    else if (lhs->address > rhs->address)
        return 1;
    return 0;
}


void ins_s_description (struct _ins * ins, const char * description)
{
    if (ins->description != NULL)
        free(ins->description);

    if (description == NULL)
        ins->description = NULL;
    else
        ins->description = strdup(description);
}


void ins_s_comment (struct _ins * ins, const char * comment)
{
    if (ins->comment != NULL)
        free(ins->comment);

    if ((comment == NULL) || (strlen(comment) == 0))
        ins->comment = NULL;
    else
        ins->comment = strdup(comment);
}


void ins_add_successor (struct _ins * ins, uint64_t address, int type)
{
    struct _ins_value * successor = ins_value_create(address, type);
    list_append(ins->successors, successor);
    object_delete(successor);
}


int ins_is_call (const struct _ins * ins)
{
    struct _list_it * lit;
    for (lit = list_iterator(ins->successors); lit != NULL; lit = lit->next) {
        struct _ins_value * successor = lit->data;
        if (successor->type == INS_SUC_CALL)
            return 1;
    }
    return 0;
}


struct _ins_value * ins_value_create (uint64_t address, int type)
{
    struct _ins_value * value = malloc(sizeof(struct _ins_value));

    value->object  = &ins_value_object;
    value->address = address;
    value->type    = type;

    return value;
}


void ins_value_delete (struct _ins_value * value)
{
    free(value);
}


struct _ins_value * ins_value_copy (const struct _ins_value * value)
{
    return ins_value_create(value->address, value->type);
}