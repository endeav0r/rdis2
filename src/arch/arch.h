#ifndef arch_HEADER
#define arch_HEADER

#include "graph.h"
#include "map.h"

#include <inttypes.h>

typedef struct _graph * (* arch_disassemble) (const struct _map *, const uint64_t entry);

struct _arch_dis_option {
    char * name;
    arch_disassemble disassemble;
};

struct _arch {
    struct _ins * (* disassemble_ins) (const struct _map * mem_map, const uint64_t address);
    struct _arch_dis_option default_dis_option;
    struct _arch_dis_option disassembly_options[];
};

#endif