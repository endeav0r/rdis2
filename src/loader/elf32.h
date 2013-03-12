#ifndef elf32_HEADER
#define elf32_HEADER

#include "loader.h"

extern const struct _loader loader_elf32;

int            elf32_select     (const struct _buffer * buffer);
struct _map  * elf32_memory_map (const struct _buffer * buffer);
struct _list * elf32_entries    (const struct _buffer * buffer);
struct _arch * elf32_arch       (const struct _buffer * buffer);
const char   * elf32_label      (const struct _buffer * buffer, uint64_t address);

#endif