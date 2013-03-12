#ifndef instruction_HEADER
#define instruction_HEADER

#include <inttypes.h>
#include <stdlib.h>

#include "list.h"
#include "object.h"

/*
*  Changes to instructions should maybe pay attention to rdis_regraph_function
*  which does funky things with instructions...
*/

enum {
    INS_SUC_NORMAL,
    INS_SUC_JUMP,
    INS_SUC_JCC_TRUE,
    INS_SUC_JCC_FALSE,
    INS_SUC_CALL
};

struct _ins {
    const struct _object * object;
    uint64_t       address;
    struct _list * successors; // of type _ins_value
    uint8_t *      bytes;
    size_t         size;
    char *         description;
    char *         comment;
};


struct _ins_value {
    const struct _object * object;
    uint64_t address;
    int      type;
};


struct _ins * ins_create    (uint64_t address,
                             const uint8_t * bytes,
                             size_t size,
                             const char * description,
                             const char * comment);

void          ins_delete      (struct _ins * ins);
struct _ins * ins_copy        (const struct _ins * ins);
int           ins_cmp         (const struct _ins * lhs, const struct _ins * rhs);

void          ins_s_comment     (struct _ins * ins, const char * comment);
void          ins_s_description (struct _ins * ins, const char * description);
void          ins_s_target      (struct _ins * ins, uint64_t target);
void          ins_add_successor (struct _ins * ins, uint64_t address, int type);

// returns 1 if instruction performs a call, 0 otherwise
int           ins_is_call (const struct _ins * ins);

struct _ins_value * ins_value_create (uint64_t address, int type);
void                ins_value_delete (struct _ins_value * value);
struct _ins_value * ins_value_copy   (const struct _ins_value * value);

#endif