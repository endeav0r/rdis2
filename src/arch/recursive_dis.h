#ifndef recursive_dis_HEADER
#define recursive_dis_HEADER

#include <inttypes.h>

#include "graph.h"
#include "instruction.h"
#include "map.h"

struct _graph * recursive_disassemble (const struct _map * mem_map,
                                       uint64_t entry,
               struct _ins * (* ins_callback) (const struct _map *, uint64_t));

#endif