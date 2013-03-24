#include "arm.h"

#include "darm/darm.h"

#include "buffer.h"
#include "instruction.h"
#include "recursive_dis.h"

struct _ins   * arm_disassemble_ins       (const struct _map *, const uint64_t address);
struct _graph * arm_recursive_disassemble (const struct _map *, const uint64_t entry);

struct _arch arch_arm = {
    arm_disassemble_ins,
    {"arm Recursive Disassembly", arm_recursive_disassemble},
    {
        {"arm Recursive Disassembly", arm_recursive_disassemble},
        {NULL, NULL}
    }
};



struct _ins * arm_disassemble_ins (const struct _map * mem_map,
                                   const uint64_t address)
{
    darm_t darm;

    struct _buffer * buf      = map_fetch_max(mem_map, address);
    uint64_t         buf_addr = map_fetch_max_key(mem_map, address);

    if (buf == NULL)
        return NULL;

    size_t offset = address - buf_addr;

    if (offset + 4 >= buf->size)
        return NULL;

    uint32_t w = buf->bytes[offset];
    w |= (buf->bytes[offset + 1]) << 8;
    w |= (buf->bytes[offset + 2]) << 16;
    w |= (buf->bytes[offset + 3]) << 24;

    if (darm_armv7_disasm(&darm, w))
        return NULL;

    darm_str_t darm_str_;
    darm_str(&darm, &darm_str_);
    struct _ins * ins = ins_create(address,
                                   (uint8_t *) &w,
                                   4,
                                   darm_str_.instr,
                                   NULL);

    int64_t dest_offset = (int32_t) darm.imm;
    uint64_t dest = address + dest_offset;

    switch(darm.instr) {
    case I_B :
        if (darm.cond == C_AL) {
            ins_add_successor(ins, dest, INS_SUC_NORMAL);
            break;
        }

        ins_add_successor(ins, dest, INS_SUC_JCC_TRUE);
        ins_add_successor(ins, address + 4, INS_SUC_JCC_FALSE);

        break;

    case I_BL :
        ins_add_successor(ins, dest, INS_SUC_CALL);
        ins_add_successor(ins, address + 4, INS_SUC_NORMAL);

    case I_BX  :
    case I_BXJ :
    case I_BLX :
        break;

    case I_POP :
        break;

    case I_MOV :
        if ((darm.Rd == PC) && (darm.Rm == LR))
            break;

    default :
        ins_add_successor(ins, address + 4, INS_SUC_NORMAL);
        break;
    }

    return ins;
}


struct _graph * arm_recursive_disassemble (const struct _map * mem_map, const uint64_t entry)
{
    return recursive_disassemble(mem_map, entry, arm_disassemble_ins);
}