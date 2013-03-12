#include "elf64.h"

#include "index.h"
#include "instruction.h"
#include "util.h"
#include "x86.h"

#include <elf.h>
#include <string.h>


const struct _loader loader_elf64 = {
    elf64_select,
    elf64_memory_map,
    elf64_entries,
    elf64_arch,
    elf64_label
};


int elf64_check (const struct _buffer * buffer)
{
    if (    (buffer_safe_byte(buffer, EI_MAG0) == ELFMAG0)
         && (buffer_safe_byte(buffer, EI_MAG1) == ELFMAG1)
         && (buffer_safe_byte(buffer, EI_MAG2) == ELFMAG2)
         && (buffer_safe_byte(buffer, EI_MAG3) == ELFMAG3)
         && (buffer_safe_byte(buffer, EI_CLASS) == ELFCLASS64)) {
        return 0;
    }
    return 1;
}

Elf64_Ehdr * elf64_ehdr (const struct _buffer * buffer)
{
    if (sizeof(Elf64_Ehdr) > buffer->size)
        return NULL;
    return (Elf64_Ehdr *) buffer->bytes;
}


Elf64_Phdr * elf64_phdr (const struct _buffer * buffer, size_t index)
{
    Elf64_Ehdr * ehdr = elf64_ehdr(buffer);
    if (ehdr == NULL)
        return NULL;

    if (index >= ehdr->e_phnum)
        return NULL;

    if (ehdr->e_phentsize == 0)
        return NULL;

    size_t offset = ehdr->e_phoff + (ehdr->e_phentsize * index);
    if (offset + sizeof(Elf64_Phdr) > buffer->size)
        return NULL;

    return (Elf64_Phdr *) &(buffer->bytes[offset]);
}


Elf64_Shdr * elf64_shdr (const struct _buffer * buffer, size_t index)
{
    Elf64_Ehdr * ehdr = elf64_ehdr(buffer);
    if (ehdr == NULL)
        return NULL;

    if (index >= ehdr->e_shnum)
        return NULL;

    if (ehdr->e_shentsize == 0)
        return NULL;

    size_t offset = ehdr->e_shoff + (ehdr->e_shentsize * index);
    if (offset + sizeof(Elf64_Shdr) > buffer->size)
        return NULL;

    return (Elf64_Shdr *) &(buffer->bytes[offset]);
}


const char * elf64_strtab (const struct _buffer * buffer,
                           size_t strtab,
                           size_t offset)
{
    Elf64_Shdr * shdr = elf64_shdr(buffer, strtab);
    if (shdr == NULL)
        return NULL;

    offset += shdr->sh_offset;

    // ensure string null-terminates in buffer
    size_t i = offset;
    while (1) {
        if (i >= buffer->size)
            return NULL;
        if (buffer->bytes[i] == '\0')
            return (const char *) &(buffer->bytes[offset]);
        i++;
    }

    return NULL;
}


Elf64_Shdr * elf64_shdr_by_name (const struct _buffer * buffer, const char * name)
{
    Elf64_Ehdr * ehdr = elf64_ehdr(buffer);
    if (ehdr == NULL)
        return NULL;

    size_t shdr_i = 0;
    Elf64_Shdr * shdr;
    while ((shdr = elf64_shdr(buffer, shdr_i++)) != NULL) {
        const char * shdr_name = elf64_strtab(buffer, ehdr->e_shstrndx, shdr->sh_name);
        if (strcmp(shdr_name, name) == 0)
            return shdr;
    }

    return NULL;
}


void * elf64_section_element (const struct _buffer * buffer,
                              size_t shdr_index,
                              size_t index)
{
    Elf64_Shdr * shdr = elf64_shdr(buffer, shdr_index);

    if (shdr == NULL)
        return NULL;
    
    if (shdr->sh_entsize == 0) // bastards
        return NULL;

    size_t offset = shdr->sh_entsize * index;

    if (offset + shdr->sh_entsize > shdr->sh_size)
        return NULL;
    
    offset += shdr->sh_offset;

    if (offset + shdr->sh_entsize > buffer->size)
        return NULL;

    return &(buffer->bytes[offset]);
}


Elf64_Sym * elf64_sym (const struct _buffer * buffer,
                       size_t shdr_index,
                       size_t sym_index)
{
    Elf64_Shdr * shdr = elf64_shdr(buffer, shdr_index);

    if (shdr == NULL)
        return NULL;

    if (    (shdr->sh_type != SHT_SYMTAB)
         && (shdr->sh_type != SHT_DYNSYM))
        return NULL;

    return (Elf64_Sym *) elf64_section_element(buffer, shdr_index, sym_index);
}


const char * elf64_rel_name_by_address (const struct _buffer * buffer,
                                       uint64_t address)
{
    size_t shdr_i;
    Elf64_Shdr * shdr;
    for (shdr_i = 0; (shdr = elf64_shdr(buffer, shdr_i)) != NULL; shdr_i++) {
        if (    (shdr->sh_type != SHT_REL) 
             && (shdr->sh_type != SHT_RELA))
            continue;

        size_t rel_i;
        Elf64_Rel * rel;
        for (rel_i = 0; (rel = elf64_section_element(buffer, shdr_i, rel_i)) != NULL; rel_i++) {
            if (rel->r_offset == address) {
                Elf64_Sym  * sym    = elf64_sym(buffer, shdr->sh_link,
                                                ELF64_R_SYM(rel->r_info));
                Elf64_Shdr * symtab = elf64_shdr(buffer, shdr->sh_link);

                if ((sym == NULL) || (symtab == NULL))
                    continue;

                return elf64_strtab(buffer, symtab->sh_link, sym->st_name);
            }
        }
    }

    return NULL;
}


int elf64_select (const struct _buffer * buffer)
{
    return elf64_check(buffer) == 1 ? 0 : 1;
}


struct _map * elf64_memory_map (const struct _buffer * buffer)
{
    if (elf64_check(buffer))
        return NULL;

    struct _map * mem_map = map_create();

    Elf64_Phdr * phdr;
    size_t i = 0;
    while ((phdr = elf64_phdr(buffer, i++)) != NULL) {
        // create a buffer for this page
        struct _buffer * buf = buffer_create_null(phdr->p_memsz);

        // are there contents we need to copy, IN the buffer?
        if ((phdr->p_filesz > 0) && (phdr->p_offset < buffer->size)) {
            // get the max size without violating bounds
            size_t size;
            if (phdr->p_filesz + phdr->p_offset > buffer->size)
                size = buffer->size - phdr->p_offset;
            else
                size = phdr->p_filesz;

            memcpy(buf->bytes, &(buffer->bytes[phdr->p_offset]), size);
        }

        // set permissions
        if (phdr->p_flags & PF_X) buf->permissions |= BUFFER_EXECUTE;
        if (phdr->p_flags & PF_W) buf->permissions |= BUFFER_WRITE;
        if (phdr->p_flags & PF_R) buf->permissions |= BUFFER_READ;

        // merge into mem map
        mem_map_set(mem_map, phdr->p_vaddr, buf);

        object_delete(buf);
    }

    return mem_map;
}


struct _list * elf64_entries (const struct _buffer * buffer)
{
    if (elf64_check(buffer))
        return NULL;

    Elf64_Ehdr * ehdr = elf64_ehdr(buffer);
    if (ehdr == NULL)
        return NULL;

    struct _list * entries = list_create();

    struct _index * index = index_create(ehdr->e_entry);
    list_append(entries, index);
    object_delete(index);

    size_t shdr_i;
    for (shdr_i = 0; shdr_i < ehdr->e_shnum; shdr_i++) {
        size_t sym_i = 0;
        Elf64_Sym * sym;
        while ((sym = elf64_sym(buffer, shdr_i, sym_i++)) != NULL) {
            if (    (ELF64_ST_TYPE(sym->st_info) == STT_FUNC)
                 && (sym->st_value != 0)) {
                struct _index * index = index_create(sym->st_value);
                list_append(entries, index);
                object_delete(index);
            }
        }
    }

    return entries;
}


struct _arch * elf64_arch (const struct _buffer * buffer)
{
    if (elf64_check(buffer))
        return NULL;

    Elf64_Ehdr * ehdr = elf64_ehdr(buffer);
    if (ehdr == NULL)
        return NULL;

    if (ehdr->e_machine == EM_X86_64)
        return &arch_amd64;

    printf("ehdr->e_machine %d\n", ehdr->e_machine);

    return NULL;
}


const char * elf64_label (const struct _buffer * buffer, uint64_t address)
{
    size_t shdr_i;
    Elf64_Shdr * shdr;

    for (shdr_i = 0; (shdr = elf64_shdr(buffer, shdr_i)) != NULL; shdr_i++) {
        size_t sym_i;
        Elf64_Sym * sym;
        for (sym_i = 0; (sym = elf64_sym(buffer, shdr_i, sym_i)) != NULL; sym_i++) {
            if (    (ELF64_ST_TYPE(sym->st_info) == STT_FUNC)
                 && (sym->st_value == address)) {
                const char * symbol = elf64_strtab(buffer, shdr->sh_link, sym->st_name);
                if (symbol != NULL)
                    printf("%s\n", symbol);
                if (symbol != NULL)
                    return symbol;
            }
        }
    }

    // plt

    shdr = elf64_shdr_by_name(buffer, ".plt");
    if (shdr == NULL)
        return NULL;

    printf("1");

    if (    (address >= shdr->sh_addr)
         && (address <  shdr->sh_addr + shdr->sh_size)) {
        struct _arch * arch = elf64_arch(buffer);
        struct _map * map = elf64_memory_map(buffer);
        struct _ins * ins = arch->disassemble_ins(map, address);
        object_delete(map);
        printf("2");
        if (ins != NULL) {
            printf("3[%s]", ins->description);
            struct _ins_value * successor = list_first(ins->successors);
            if (successor != NULL) {
                printf("4");
                const char * name = elf64_rel_name_by_address(buffer, successor->address);
                object_delete(ins);
                return name;
            }
            object_delete(ins);
        }
    }

    return NULL;
}