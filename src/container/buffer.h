#ifndef buffer_HEADER
#define buffer_HEADER

#include <inttypes.h>
#include <stdlib.h>

#include "object.h"

#define BUFFER_EXECUTE (1 << 0)
#define BUFFER_WRITE   (1 << 1)
#define BUFFER_READ    (1 << 2)

struct _buffer {
    const struct _object * object;
    uint32_t  permissions;
    uint8_t * bytes;
    size_t    size;
};

struct _buffer * buffer_create      (const uint8_t * bytes, size_t size);
struct _buffer * buffer_create_null (size_t size);
struct _buffer * buffer_load_file   (const char * filename);
void             buffer_delete      (struct _buffer * buffer);
struct _buffer * buffer_copy        (const struct _buffer * buffer);

int 			 buffer_safe_byte   (const struct _buffer * buffer, size_t offset);

#endif