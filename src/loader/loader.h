#ifndef loader_HEADER
#define loader_HEADER

#include "arch.h"
#include "buffer.h"
#include "list.h"
#include "map.h"

struct _loader {
	int             (* select)     (const struct _buffer * buffer);
	struct _map   * (* memory_map) (const struct _buffer * buffer);
	struct _list  * (* entries)    (const struct _buffer * buffer);
	struct _arch  * (* arch)       (const struct _buffer * buffer);
	const char    * (* label)      (const struct _buffer * buffer, uint64_t address);
};

const struct _loader * loader_select (const struct _buffer *);

#endif