#ifndef elf64_HEADER
#define elf64_HEADER

#include "loader.h"

extern const struct _loader loader_elf64;

int            elf64_select     (const struct _buffer * buffer);
struct _map  * elf64_memory_map (const struct _buffer * buffer);
struct _list * elf64_entries    (const struct _buffer * buffer);
struct _arch * elf64_arch       (const struct _buffer * buffer);
const char   * elf64_label      (const struct _buffer * buffer, uint64_t address);

#endif