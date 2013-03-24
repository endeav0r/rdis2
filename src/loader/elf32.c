#include "elf32.h"

#include "arm.h"
#include "index.h"
#include "instruction.h"
#include "util.h"
#include "x86.h"

#include <elf.h>
#include <string.h>


const struct _loader loader_elf32 = {
    elf32_select,
    elf32_memory_map,
    elf32_entries,
    elf32_arch,
    elf32_label
};


int elf32_check (const struct _buffer * buffer)
{
    if (    (buffer_safe_byte(buffer, EI_MAG0) == ELFMAG0)
         && (buffer_safe_byte(buffer, EI_MAG1) == ELFMAG1)
         && (buffer_safe_byte(buffer, EI_MAG2) == ELFMAG2)
         && (buffer_safe_byte(buffer, EI_MAG3) == ELFMAG3)
         && (buffer_safe_byte(buffer, EI_CLASS) == ELFCLASS32)) {
        return 0;
    }
    return 1;
}

Elf32_Ehdr * elf32_ehdr (const struct _buffer * buffer)
{
    if (sizeof(Elf32_Ehdr) > buffer->size)
        return NULL;
    return (Elf32_Ehdr *) buffer->bytes;
}


Elf32_Phdr * elf32_phdr (const struct _buffer * buffer, size_t index)
{
    Elf32_Ehdr * ehdr = elf32_ehdr(buffer);
    if (ehdr == NULL)
        return NULL;

    if (index >= ehdr->e_phnum)
        return NULL;

    if (ehdr->e_phentsize == 0)
        return NULL;

    size_t offset = ehdr->e_phoff + (ehdr->e_phentsize * index);
    if (offset + sizeof(Elf32_Phdr) > buffer->size)
        return NULL;

    return (Elf32_Phdr *) &(buffer->bytes[offset]);
}


Elf32_Shdr * elf32_shdr (const struct _buffer * buffer, size_t index)
{
    Elf32_Ehdr * ehdr = elf32_ehdr(buffer);
    if (ehdr == NULL)
        return NULL;

    if (index >= ehdr->e_shnum)
        return NULL;

    if (ehdr->e_shentsize == 0)
        return NULL;

    size_t offset = ehdr->e_shoff + (ehdr->e_shentsize * index);
    if (offset + sizeof(Elf32_Shdr) > buffer->size)
        return NULL;

    return (Elf32_Shdr *) &(buffer->bytes[offset]);
}


const char * elf32_strtab (const struct _buffer * buffer,
                           size_t strtab,
                           size_t offset)
{
    Elf32_Shdr * shdr = elf32_shdr(buffer, strtab);
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


Elf32_Shdr * elf32_shdr_by_name (const struct _buffer * buffer, const char * name)
{
    Elf32_Ehdr * ehdr = elf32_ehdr(buffer);
    if (ehdr == NULL)
        return NULL;

    size_t shdr_i = 0;
    Elf32_Shdr * shdr;
    while ((shdr = elf32_shdr(buffer, shdr_i++)) != NULL) {
        const char * shdr_name = elf32_strtab(buffer, ehdr->e_shstrndx, shdr->sh_name);
        if (strcmp(shdr_name, name) == 0)
            return shdr;
    }

    return NULL;
}


void * elf32_section_element (const struct _buffer * buffer,
                              size_t shdr_index,
                              size_t index)
{
    Elf32_Shdr * shdr = elf32_shdr(buffer, shdr_index);

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


Elf32_Sym * elf32_sym (const struct _buffer * buffer,
                       size_t shdr_index,
                       size_t sym_index)
{
    Elf32_Shdr * shdr = elf32_shdr(buffer, shdr_index);

    if (shdr == NULL)
        return NULL;

    if (    (shdr->sh_type != SHT_SYMTAB)
         && (shdr->sh_type != SHT_DYNSYM))
        return NULL;

    return (Elf32_Sym *) elf32_section_element(buffer, shdr_index, sym_index);
}


const char * elf32_rel_name_by_address (const struct _buffer * buffer,
                                       uint64_t address)
{
    size_t shdr_i;
    Elf32_Shdr * shdr;
    for (shdr_i = 0; (shdr = elf32_shdr(buffer, shdr_i)) != NULL; shdr_i++) {
        if (    (shdr->sh_type != SHT_REL) 
             && (shdr->sh_type != SHT_RELA))
            continue;

        size_t rel_i;
        Elf32_Rel * rel;
        for (rel_i = 0; (rel = elf32_section_element(buffer, shdr_i, rel_i)) != NULL; rel_i++) {
            if (rel->r_offset == address) {
                Elf32_Sym  * sym    = elf32_sym(buffer, shdr->sh_link,
                                                ELF32_R_SYM(rel->r_info));
                Elf32_Shdr * symtab = elf32_shdr(buffer, shdr->sh_link);

                if ((sym == NULL) || (symtab == NULL))
                    continue;

                return elf32_strtab(buffer, symtab->sh_link, sym->st_name);
            }
        }
    }

    return NULL;
}


int elf32_select (const struct _buffer * buffer)
{
    return elf32_check(buffer) == 1 ? 0 : 1;
}


struct _map * elf32_memory_map (const struct _buffer * buffer)
{
    if (elf32_check(buffer))
        return NULL;

    struct _map * mem_map = map_create();

    Elf32_Phdr * phdr;
    size_t i = 0;
    while ((phdr = elf32_phdr(buffer, i++)) != NULL) {
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


struct _list * elf32_entries (const struct _buffer * buffer)
{
    if (elf32_check(buffer))
        return NULL;

    Elf32_Ehdr * ehdr = elf32_ehdr(buffer);
    if (ehdr == NULL)
        return NULL;

    struct _list * entries = list_create();

    struct _index * index = index_create(ehdr->e_entry);
    list_append(entries, index);
    object_delete(index);

    size_t shdr_i;
    for (shdr_i = 0; shdr_i < ehdr->e_shnum; shdr_i++) {
        size_t sym_i = 0;
        Elf32_Sym * sym;
        while ((sym = elf32_sym(buffer, shdr_i, sym_i++)) != NULL) {
            if (    (ELF32_ST_TYPE(sym->st_info) == STT_FUNC)
                 && (sym->st_value != 0)) {
                struct _index * index = index_create(sym->st_value);
                list_append(entries, index);
                object_delete(index);
            }
        }
    }

    return entries;
}


struct _arch * elf32_arch (const struct _buffer * buffer)
{
    if (elf32_check(buffer))
        return NULL;

    Elf32_Ehdr * ehdr = elf32_ehdr(buffer);
    if (ehdr == NULL)
        return NULL;

    if (ehdr->e_machine == EM_386)
        return &arch_x86;

    if (ehdr->e_machine == EM_ARM)
        return &arch_arm;

    return NULL;
}


const char * elf32_label (const struct _buffer * buffer, uint64_t address)
{
    size_t shdr_i;
    Elf32_Shdr * shdr;

    for (shdr_i = 0; (shdr = elf32_shdr(buffer, shdr_i)) != NULL; shdr_i++) {
        size_t sym_i;
        Elf32_Sym * sym;
        for (sym_i = 0; (sym = elf32_sym(buffer, shdr_i, sym_i)) != NULL; sym_i++) {
            if (    (ELF32_ST_TYPE(sym->st_info) == STT_FUNC)
                 && (sym->st_value == address)) {
                const char * symbol = elf32_strtab(buffer, shdr->sh_link, sym->st_name);
                if (symbol != NULL)
                    return symbol;
            }
        }
    }

    return NULL;
}