#include "recursive_dis.h"

#include "list.h"
#include "index.h"
#include "queue.h"

struct _graph * recursive_disassemble (const struct _map * mem_map,
                                       uint64_t entry,
                struct _ins * (* ins_callback) (const struct _map *, uint64_t))
{
    struct _queue * queue = queue_create();
    struct _map * map     = map_create();

    struct _index * index = index_create(entry);
    queue_push(queue, index);
    object_delete(index);

    while (queue->size > 0) {
        struct _index * index = queue_peek(queue);

        if (map_fetch(map, index->index)) {
            queue_pop(queue);
            continue;
        }

        struct _ins * ins = ins_callback(mem_map, index->index);
        if (ins == NULL) {
            queue_pop(queue);
            continue;
        }
        map_insert(map, index->index, ins);

        struct _list_it * lit;
        for (lit = list_iterator(ins->successors); lit != NULL; lit = lit->next) {
            struct _ins_value * successor = lit->data;
            struct _index * index = index_create(successor->address);
            queue_push(queue, index);
            object_delete(index);
        }

        queue_pop(queue);
    }

    object_delete(queue);

    // create graph nodes
    struct _graph * graph = graph_create();

    struct _map_it * mit;
    for (mit = map_iterator(map); mit != NULL; mit = map_it_next(mit)) {
        graph_add_node(graph, map_it_key(mit), map_it_data(mit));
    }

    // create graph edges
    for (mit = map_iterator(map); mit != NULL; mit = map_it_next(mit)) {
        struct _ins * ins = map_it_data(mit);
        struct _list_it * lit;
        for (lit = list_iterator(ins->successors); lit != NULL; lit = lit->next) {
            struct _ins_value * successor = lit->data;
            // don't add call edges
            if (successor->type == INS_SUC_CALL)
                continue;
            graph_add_edge(graph, ins->address, successor->address, successor);
        }
    }

    object_delete(map);

    return graph;
}