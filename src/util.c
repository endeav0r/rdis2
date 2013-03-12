#include "util.h"

#include "index.h"
#include "instruction.h"

#include <string.h>

int mem_map_set (struct _map * mem_map, uint64_t address, struct _buffer * buf)
{
    struct _buffer * buf2 = map_fetch_max(mem_map, address + buf->size);
    uint64_t key          = map_fetch_max_key(mem_map, address + buf->size);

    if (    (buf2 != NULL)
         && (    ((address <= key) && (address + buf->size > key))
              || (    (address <= key + buf2->size)
                   && (address + buf->size > key + buf2->size))
              || ((address >= key) && (address + buf->size < key + buf2->size)))) {

        // if this section fits inside a previous section, modify in place
        if ((address >= key) && (address + buf->size <= key + buf2->size)) {
            memcpy(&(buf2->bytes[address - key]), buf->bytes, buf->size);
        }

        // if this section comes before a previous section
        else if (address <= key) {
            uint64_t new_size;
            if (address + buf->size < key + buf2->size)
                new_size = key + buf2->size;
            else
                new_size = address + buf->size;
            new_size -= address;

            uint8_t * tmp = malloc(new_size);
            memcpy(&(tmp[key - address]), buf2->bytes, buf2->size);
            memcpy(tmp, buf->bytes, buf->size);

            struct _buffer * new_buffer = buffer_create(tmp, new_size);
            free(tmp);
            map_remove(mem_map, key);
            map_insert(mem_map, address, new_buffer);
            object_delete(new_buffer);
        }

        // if this section overlaps previous section but starts after previous
        // section starts
        else {
            uint64_t new_size = address + buf->size - key;
            uint8_t * tmp = malloc(new_size);
            memcpy(tmp, buf2->bytes, buf2->size);
            memcpy(&(tmp[address - key]), buf->bytes, buf->size);

            struct _buffer * new_buffer = buffer_create(tmp, new_size);
            free(tmp);
            map_remove(mem_map, key);
            map_insert(mem_map, key, new_buffer);
            object_delete(new_buffer);
        }
    }
    else {
        map_insert(mem_map, address, buf);
    }

    return 0;
}


struct _graph * ins_graph_to_list_ins_graph (struct _graph * graph)
{
    struct _graph * lgraph = graph_create();

    struct _graph_it * git;
    for (git = graph_iterator(graph); git != NULL; git = graph_it_next(git)) {
        struct _list * list = list_create();
        list_append(list, graph_it_data(git));
        graph_add_node(lgraph, graph_it_index(git), list);
        object_delete(list);
    }

    for (git = graph_iterator(graph); git != NULL; git = graph_it_next(git)) {
        struct _list * successors = graph_node_successors(graph_it_node(git));
        struct _list_it * lit;
        for (lit = list_iterator(successors); lit != NULL; lit = lit->next) {
            struct _graph_edge * edge = lit->data;
            graph_add_edge(lgraph, edge->head, edge->tail, edge->data);
        }
        object_delete(successors);
    }

    return lgraph;
}


struct _list * ins_graph_to_list_call_ins (struct _graph * graph)
{
    struct _list * list = list_create();

    struct _graph_it * git;
    for (git = graph_iterator(graph); git != NULL; git = graph_it_next(git)) {
        if (ins_is_call(graph_it_data(git)))
            list_append(list, graph_it_data(git));
    }

    return list;
}


struct _list * ins_graph_to_list_index_call_dest (struct _graph * graph)
{
    struct _list * list = list_create();

    struct _graph_it * git;
    for (git = graph_iterator(graph); git != NULL; git = graph_it_next(git)) {
        struct _ins * ins = graph_it_data(git);
        struct _list_it * lit;
        for (lit = list_iterator(ins->successors); lit != NULL; lit = lit->next) {
            struct _ins_value * successor = lit->data;
            if (successor->type == INS_SUC_CALL) {
                struct _index * index = index_create(successor->address);
                list_append(list, index);
                index_delete(index);
            }
        }
    }

    return list;
}