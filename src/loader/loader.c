#include "loader.h"

#include "elf32.h"
#include "elf64.h"

const struct _loader * loaders [] = {
    &loader_elf32,
    &loader_elf64,
    NULL
};


const struct _loader * loader_select (const struct _buffer * buffer)
{
    size_t i = 0;
    for (i = 0; loaders[i] != NULL; i++) {
        if (loaders[i]->select(buffer))
            return loaders[i];
    }

    return NULL;
}