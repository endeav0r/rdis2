#ifndef util_HEADER
#define util_HEADER

#include <inttypes.h>

#include "buffer.h"
#include "graph.h"
#include "map.h"


int mem_map_set (struct _map * mem_map, uint64_t address, struct _buffer * buf);

struct _graph * ins_graph_to_list_ins_graph (struct _graph * graph);

// takes a graph of type ins and returns all instructions with successors of
// type call
struct _list * ins_graph_to_list_call_ins (struct _graph * graph);

// takes a graph of type ins and returns all destinations of instruction
// successors of type call as a list of _index
struct _list * ins_graph_to_list_index_call_dest (struct _graph * graph);

#endif