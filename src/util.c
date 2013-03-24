#include "util.h"

#include <stdio.h>

#include "index.h"
#include "instruction.h"

#include <string.h>

int mem_map_set (struct _map * mem_map, uint64_t address, struct _buffer * buf)
{
    struct _buffer * buf2 = map_fetch_max(mem_map, address + buf->size);
    uint64_t key          = map_fetch_max_key(mem_map, address + buf->size);

    if (buf2 == NULL)
        map_insert(mem_map, address, buf);

    /*
    * buf fits inside buf2
    * |-------------| buf2
    *   |----|        buf
    */
    else if ((address >= key) && (address + buf->size <= key + buf2->size))
        memcpy(&(buf2->bytes[address - key]), buf->bytes, buf->size);

    /*
    * buf envelops buf2
    *    |--------|     buf2
    * |---------------| buf
    */
    else if ((address <= key) && (address + buf->size > key + buf2->size)) {
        map_remove(mem_map, key);
        return mem_map_set(mem_map, address, buf);
    }

    /*
    * buf overlaps buf2 in some other way
    */
    else if (    ((address < key) && (address + buf->size > key))
         || ((address > key) && (address < key + buf2->size))) {
        uint64_t lower  = (address < key) ? address : key;
        uint64_t higher = address + buf->size;

        if (higher < key + buf2->size)
            higher = key + buf2->size;

        struct _buffer * newbuf = buffer_create_null(higher - lower);

        size_t buf_offset  = address - lower;
        size_t buf2_offset = key - lower;

        memcpy(&(newbuf->bytes[buf2_offset]), buf2->bytes, buf2->size);
        memcpy(&(newbuf->bytes[buf_offset]), buf->bytes, buf->size);

        map_remove(mem_map, key);

        mem_map_set(mem_map, lower, newbuf);

        object_delete(newbuf);
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


char * str_append (char * string,
                   size_t * str_size,
                   size_t * str_len,
                   const char * append)
{
    size_t append_len = strlen(append);

    if (append_len + *str_len >= *str_size) {
        *str_size = append_len + *str_len + 4096;
        string = realloc(string, *str_size);
        if (string == NULL)
            return NULL;
    }

    strcat(string, append);

    *str_len += append_len;

    return string;
}


char * ins_graph_to_dot_string (struct _graph * graph)
{
    struct _graph * g = graph_create();
    struct _graph_it * git;
    for (git = graph_iterator(graph); git != NULL; git = graph_it_next(git)) {
        struct _list * list = list_create();
        list_append(list, graph_it_data(git));
        graph_add_node(g, graph_it_index(git), list);
        object_delete(list);
    }

    for (git = graph_iterator(graph); git != NULL; git = graph_it_next(git)) {
        struct _list * successors = graph_node_successors(graph_it_node(git));

        struct _list_it * lit;
        for (lit = list_iterator(successors); lit != NULL; lit = lit->next) {
            struct _graph_edge * edge = lit->data;
            graph_add_edge(g, edge->head, edge->tail, edge->data);
        }
        object_delete(successors);
    }

    graph_reduce(g);


    size_t str_size = 4096;
    size_t str_len  = 0;
    char * str = malloc(str_size);

    str[0] = '\0';

    str = str_append(str, &str_size, &str_len, "digraph G {\n");

    for (git = graph_iterator(g); git != NULL; git = graph_it_next(git)) {
        char tmp[256];
        snprintf(tmp, 256, "block_%lld [shape=box, fontname=\"monospace\", fontsize=\"9.0\" label=<",
                 (unsigned long long) graph_it_index(git));

        str = str_append(str, &str_size, &str_len, tmp);

        str = str_append(str, &str_size, &str_len, "<table cellspacing=\"0\" border=\"0\">");

        struct _list * ins_list = graph_it_data(git);
        struct _list_it * lit;
        for (lit = list_iterator(ins_list); lit != NULL; lit = lit->next) {
            struct _ins * ins = lit->data;

            snprintf(tmp, 256, "<tr><td align=\"left\"><font color=\"blue\">%04llx</font></td><td align=\"left\">%s</td></tr>",
                     (unsigned long long) ins->address,
                     ins->description);

            str = str_append(str, &str_size, &str_len, tmp);
        }

        str = str_append(str, &str_size, &str_len, "</table>>];\n");

        struct _list * successors = graph_node_successors(graph_it_node(git));
        for (lit = list_iterator(successors); lit != NULL; lit = lit->next) {
            struct _graph_edge * edge = lit->data;
            snprintf(tmp, 256, "block_%lld -> block_%lld;\n",
                     (unsigned long long) edge->head,
                     (unsigned long long) edge->tail);
            str = str_append(str, &str_size, &str_len, tmp);
        }

        object_delete(successors);
    }

    str = str_append(str, &str_size, &str_len, "bgcolor=\"transparent\"\n}");

    object_delete(g);

    return str;
}