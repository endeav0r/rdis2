#include "x86.h"

#include <udis86.h>

#include "buffer.h"
#include "instruction.h"
#include "recursive_dis.h"

struct _ins   * x86_disassemble_ins       (const struct _map *, const uint64_t address);
struct _graph * x86_recursive_disassemble (const struct _map *, const uint64_t entry);

struct _arch arch_x86 = {
    x86_disassemble_ins,
    {"x86 Recursive Disassembly", x86_recursive_disassemble},
    {
        {"x86 Recursive Disassembly", x86_recursive_disassemble},
        {NULL, NULL}
    }
};

struct _ins   * amd64_disassemble_ins       (const struct _map * mem_map, const uint64_t address);
struct _graph * amd64_recursive_disassemble (const struct _map *, const uint64_t entry);

struct _arch arch_amd64 = {
    amd64_disassemble_ins,
    {"amd64 Recursive Disassembly", amd64_recursive_disassemble},
    {
        {"amd64 Recursive Disassembly", amd64_recursive_disassemble},
        {NULL, NULL}
    }
};


uint64_t x86_sign_extend_lval (struct ud_operand * operand)
{
    int64_t lval = 0;
    switch (operand->size) {
    case  8 : lval = operand->lval.sbyte;  break;
    case 16 : lval = operand->lval.sword;  break;
    case 32 : lval = operand->lval.sdword; break;
    case 64 : lval = operand->lval.sqword; break;
    }
    return lval;
}


struct _ins * x86_disassemble_ins_ (const struct _map * mem_map,
                                    const uint64_t address,
                                    uint8_t mode)
{
    struct _buffer * buf      = map_fetch_max(mem_map, address);
    uint64_t         buf_addr = map_fetch_max_key(mem_map, address);

    if (buf == NULL)
        return NULL;

    size_t offset = address - buf_addr;

    ud_t ud_obj;

    ud_init(&ud_obj);
    ud_set_mode(&ud_obj, mode);
    ud_set_syntax(&ud_obj, UD_SYN_INTEL);
    ud_set_input_buffer(&ud_obj, &(buf->bytes[offset]), buf->size - offset);

    if (ud_disassemble(&ud_obj) == 0)
        return NULL;

    struct _ins * ins = ins_create(address,
                                   ud_insn_ptr(&ud_obj),
                                   ud_insn_len(&ud_obj),
                                   ud_insn_asm(&ud_obj),
                                   NULL);

    switch (ud_obj.mnemonic) {
    case UD_Ijo   :
    case UD_Ijno  :
    case UD_Ijb   :
    case UD_Ijae  :
    case UD_Ijz   :
    case UD_Ijnz  :
    case UD_Ijbe  :
    case UD_Ija   :
    case UD_Ijs   :
    case UD_Ijns  :
    case UD_Ijp   :
    case UD_Ijnp  :
    case UD_Ijl   :
    case UD_Ijge  :
    case UD_Ijle  :
    case UD_Ijg   :
    case UD_Iloop :
        ins_add_successor(ins, address + ud_insn_len(&ud_obj), INS_SUC_JCC_FALSE);
        if (ud_obj.operand[0].type == UD_OP_JIMM) {
            ins_add_successor(ins,
                              address
                              + ud_insn_len(&ud_obj)
                              + x86_sign_extend_lval(&(ud_obj.operand[0])),
                              INS_SUC_JCC_TRUE);
        }
        break;
    
    case UD_Ijmp  :
        if (ud_obj.operand[0].type == UD_OP_JIMM) {
            ins_add_successor(ins,
                              address
                              + ud_insn_len(&ud_obj)
                              + x86_sign_extend_lval(&(ud_obj.operand[0])),
                              INS_SUC_JUMP);
        }
        break;
    
    case UD_Icall :
        ins_add_successor(ins, address + ud_insn_len(&ud_obj), INS_SUC_NORMAL);
        if (ud_obj.operand[0].type == UD_OP_JIMM) {
            ins_add_successor(ins,
                              address
                              + ud_insn_len(&ud_obj)
                              + x86_sign_extend_lval(&(ud_obj.operand[0])),
                              INS_SUC_CALL);
        }
        break;

    case UD_Iret :
    case UD_Ihlt :
        break;

    default :
        ins_add_successor(ins, address + ud_insn_len(&ud_obj), INS_SUC_NORMAL);
    }

    return ins;
}


struct _ins * x86_disassemble_ins (const struct _map * mem_map, const uint64_t address)
{
    return x86_disassemble_ins_(mem_map, address, 32);
}


struct _graph * x86_recursive_disassemble (const struct _map * mem_map, const uint64_t entry)
{
    return recursive_disassemble(mem_map, entry, x86_disassemble_ins);
}


struct _ins * amd64_disassemble_ins (const struct _map * mem_map, const uint64_t address)
{
    return x86_disassemble_ins_(mem_map, address, 64);
}


struct _graph * amd64_recursive_disassemble (const struct _map * mem_map, const uint64_t entry)
{
    return recursive_disassemble(mem_map, entry, amd64_disassemble_ins);
}