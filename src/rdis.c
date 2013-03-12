#include <stdio.h>
#include "arch.h"
#include "elf32.h"
#include "function.h"
#include "index.h"
#include "loader.h"
#include "queue.h"
#include "util.h"
#include "x86.h"

struct _map * recursive_dis_entries (arch_disassemble disassemble,
                                     const struct _map * mem_map,
                                     const struct _list * entries)
{
    struct _map * functions = map_create();
    struct _queue * queue = queue_create();

    struct _list_it * lit;
    for (lit = list_iterator((struct _list *) entries);
         lit != NULL;
         lit = lit->next) {
        queue_push(queue, lit->data);
    }

    while (queue->size > 0) {
        struct _index * index = queue_peek(queue);
        if (map_fetch(functions, index->index)) {
            queue_pop(queue);
            continue;
        }

        struct _graph * graph = disassemble(mem_map, index->index);

        struct _list * call_dests = ins_graph_to_list_index_call_dest(graph);
        struct _list_it * lit;
        for (lit = list_iterator(call_dests); lit != NULL; lit = lit->next) {
            queue_push(queue, lit->data);
        }
        object_delete(call_dests);

        struct _function * function = function_create(index->index, graph, NULL);

        object_delete(graph);

        map_insert(functions, index->index, function);

        object_delete(function);

        queue_pop(queue);
    }

    object_delete(queue);

    return functions;
}



int main (int argc, char * argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <executable>\n", argv[0]);
        return -1;
    }

    FILE * fh = fopen(argv[1], "rb");
    if (fh == NULL) {
        fprintf(stderr, "Could not open file %s\n", argv[1]);
        return -1;
    }

    fseek(fh, 0, SEEK_END);
    size_t filesize = ftell(fh);
    fseek(fh, 0, SEEK_SET);
    unsigned char * buf = malloc(filesize);

    fread(buf, 1, filesize, fh);

    fclose(fh);

    struct _buffer * buffer = buffer_create(buf, filesize);
    free(buf);

    const struct _loader * loader = loader_select(buffer);

    if (loader == NULL) {
        printf("no loader selected\n");
        object_delete(buffer);
        return -1;
    }
    
    if (loader == &loader_elf32)
        printf("elf32 loader selected %p\n", loader);

    struct _arch * arch = loader->arch(buffer);

    if (arch == NULL) {
        printf("no arch selected\n");
        object_delete(buffer);
        return -1;
    }

    if (arch == &arch_x86)
        printf("arch x86 selected\n");

    struct _list * entries = loader->entries(buffer);
    struct _list_it * lit;
    for (lit = list_iterator(entries); lit != NULL; lit = lit->next) {
        struct _index * index = lit->data;
        printf("entry: %llx\n", (unsigned long long) index->index);
    }

    struct _map * mem_map = loader->memory_map(buffer);
    if (mem_map == NULL) {
        printf("loader returned NULL mem_map\n");
        objects_delete(entries, buffer, NULL);
        return -1;
    }

    printf("have mem map\n");

    struct _map * functions = recursive_dis_entries(arch->default_dis_option.disassemble,
                                                    mem_map, entries);

    struct _map_it * mit;
    for (mit = map_iterator(functions); mit != NULL; mit = map_it_next(mit)) {
        struct _function * function = map_it_data(mit);
        function_s_name(function, loader->label(buffer, function->address));
        printf("function : %08llx %s\n",
               (unsigned long long) function->address,
               function->name);
    }

    objects_delete(buffer, entries, mem_map, functions, NULL);

    return 0;
}