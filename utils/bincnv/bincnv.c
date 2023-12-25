/* KallistiOS ##version##

   bincnv.c
   (c)2000 Megan Potter

   Test ELF to BIN converter.. loads to VMA 0x8c010000. This is an
   exact functional duplicate of the routine in process/elf.c and is
   used for testing new changes first.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define uint8 unsigned char
#define uint16 unsigned short
#define uint32 unsigned long
#define int32 long

/* ELF file header */
struct elf_hdr_t {
    unsigned char   ident[16];  /* For elf32-shl, 0x7f+"ELF"+1+1 */
    uint16      type;       /* 0x02 for ET_EXEC */
    uint16      machine;    /* 0x2a for elf32-shl */
    uint32      version;
    uint32      entry;      /* Entry point */
    uint32      phoff;      /* Program header offset */
    uint32      shoff;      /* Section header offset */
    uint32      flags;      /* Processor flags */
    uint16      ehsize;     /* ELF header size in bytes */
    uint16      phentsize;  /* Program header entry size */
    uint16      phnum;      /* Program header entry count */
    uint16      shentsize;  /* Section header entry size */
    uint16      shnum;      /* Section header entry count */
    uint16      shstrndx;   /* String table section index */
};

/* Section header types */
#define SHT_NULL    0       /* Inactive */
#define SHT_PROGBITS    1       /* Program code/data */
#define SHT_SYMTAB  2       /* Full symbol table */
#define SHT_STRTAB  3       /* String table */
#define SHT_RELA    4       /* Relocation table */
#define SHT_HASH    5       /* Sym tab hashtable */
#define SHT_DYNAMIC 6       /* Dynamic linking info */
#define SHT_NOTE    7       /* Notes */
#define SHT_NOBITS  8       /* Occupies no space in the file */
#define SHT_REL     9       /* Relocation table */
#define SHT_SHLIB   10      /* Invalid.. hehe */
#define SHT_DYNSYM  11      /* Dynamic-only sym tab */
#define SHT_LOPROC  0x70000000  /* Processor specific */
#define SHT_HIPROC  0x7fffffff
#define SHT_LOUSER  0x80000000  /* Program specific */
#define SHT_HIUSER  0xffffffff

/* Section header flags */
#define SHF_WRITE   1       /* Writable data */
#define SHF_ALLOC   2       /* Resident */
#define SHF_EXECINSTR   4       /* Executable instructions */
#define SHF_MASKPROC    0xf0000000  /* Processor specific */


/* Section header */
struct elf_shdr_t {
    uint32      name;       /* Index into string table */
    uint32      type;       /* See constants above */
    uint32      flags;
    uint32      addr;       /* In-memory offset */
    uint32      offset;     /* On-disk offset */
    uint32      size;       /* Size (if SHT_NOBITS, zero file len */
    uint32      link;       /* See below */
    uint32      info;       /* See below */
    uint32      addralign;  /* Alignment constraints */
    uint32      entsize;    /* Fixed-size table entry sizes */
};
/* Link and info fields:

switch (sh_type) {
    case SHT_DYNAMIC:
        link = section header index of the string table used by
            the entries in this section
        info = 0
    case SHT_HASH:
        ilnk = section header index of the string table to which
            this info applies
        info = 0
    case SHT_REL, SHT_RELA:
        link = section header index of associated symbol table
        info = section header index of section to which reloc applies
    case SHT_SYMTAB, SHT_DYNSYM:
        link = section header index of associated string table
        info = one greater than the symbol table index of the last
            local symbol (binding STB_LOCAL)
}

*/

/* Symbol table entry */
#define STN_UNDEF   0
#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3
#define STT_FILE    4
struct elf_sym_t {
    uint32      name;       /* Index into stringtab */
    uint32      value;
    uint32      size;
    uint8       info;       /* type == info & 0x0f */
    uint8       other;
    uint16      shndx;      /* Section index */
};

/* Relocation-A Entries */
#define R_SH_DIR32      1
struct elf_rela_t {
    uint32      offset;     /* Offset within section */
    uint32      info;       /* Symbol and type */
    int32       addend;     /* "A" constant */
};
#define ELF32_R_SYM(i) ((i) >> 8)
#define ELF32_R_TYPE(i) ((uint8)(i))

int find_sym(char *name, struct elf_sym_t* table, int tablelen) {
    int i;

    for(i = 0; i < tablelen; i++) {
        if(!strcmp((char*)table[i].name, name))
            return i;
    }

    return -1;
}

static int write_file_contents(char const * const filename, void * data, size_t size) {
    FILE * f = fopen(filename, "wb");
    int ret = f && fwrite(data, size, 1, f) == 1;
    fclose(f);
    return ret;
}

/**
 * Helper function to load the raw texture data into an array.
 * @param filename The filename of the texture.
 * @param data A pointer to an array where the data should be stored.
 * @param size A pointer to a variable where to size should be stored.
 * @return 0 on success, non-zero on error.
 */
static int read_file_contents(char const * const filename, char **data, size_t *size) {
    FILE *f = 0;

    f = fopen(filename, "rb");

    if(!f) {
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long imageSize = ftell(f);
    rewind(f);

    *data = malloc(imageSize);
    if(!*data) {
        fclose(f);
        return 2;
    }

    if(fread(*data, imageSize, 1, f) != 1) {
        free(*data);
        fclose(f);
        return 3;
    }

    fclose(f);

    *size = imageSize;

    return 0;
}

static int build_memory_image(struct elf_shdr_t * shdrs, uint16 shnum) {
    uint16 i;
    size_t sz = 0;
    for(i = 0; i < shnum; i++) {
        if(shdrs[i].flags & SHF_ALLOC) {
            shdrs[i].addr = sz;
            sz += shdrs[i].size;

            if(shdrs[i].addralign && (shdrs[i].addr % shdrs[i].addralign)) {
                shdrs[i].addr = (shdrs[i].addr + shdrs[i].addralign)
                    & ~(shdrs[i].addralign - 1);
            }
        }
    }

    return sz;
}

#define ERROR(...) { ret = 1; fprintf(stderr, __VA_ARGS__); goto cleanup; }

/* There's a lot of shit in here that's not documented or very poorly
   documented by Intel.. I hope that this works for future compilers. */
static int elf_load(char ** out, size_t * outsz, char const * const filename, uint32 vma) {
    int    ret = 0;
    char   * img    = 0;
    char   * imgout = 0;
    size_t sz;
    int    i;
    int    j;
    int    sect;
    struct elf_hdr_t  * hdr;
    struct elf_shdr_t * shdrs;
    struct elf_shdr_t * symtabhdr;
    struct elf_sym_t  * symtab;
    int    symtabsize;
    struct elf_rela_t * reltab;
    int    reltabsize;
    char   * stringtab;


    if(read_file_contents(filename, &img, &sz)) {
        ERROR("Cannot allocate memory.\n");
    }

    hdr = (struct elf_hdr_t*)img;

    if(hdr->ident[0] != 0x7f || memcmp(hdr->ident + 1, "ELF", 3)) {
        ERROR("File is not a valid ELF file\n");
    }

    if(hdr->ident[4] != 1 || hdr->ident[5] != 1) {
        ERROR("Invalid architecture flags in ELF file\n");
    }

    if(hdr->machine != 0x2a) {
        ERROR("Invalid architecture %02x in ELF file\n", hdr->machine);
    }

    /* Print some debug info */
    printf("File size is %zu bytes\n", sz);
    printf(" entry point %08lx\n", hdr->entry);
    printf(" ph offset   %08lx\n", hdr->phoff);
    printf(" sh offset   %08lx\n", hdr->shoff);
    printf(" flags       %08lx\n", hdr->flags);
    printf(" ehsize      %08x\n",  hdr->ehsize);
    printf(" phentsize   %08x\n",  hdr->phentsize);
    printf(" phnum       %08x\n",  hdr->phnum);
    printf(" shentsize   %08x\n",  hdr->shentsize);
    printf(" shnum       %08x\n",  hdr->shnum);
    printf(" shstrndx    %08x\n",  hdr->shstrndx);

    /* Locate the string table; SH elf files ought to have
       two string tables, one for section names and one for object
       string names. We'll look for the latter. */
    shdrs = (struct elf_shdr_t *)(img + hdr->shoff);
    stringtab = 0;

    for(i = 0; i < hdr->shnum; i++) {
        if(shdrs[i].type == SHT_STRTAB && i != hdr->shstrndx) {
            stringtab = (char*)(img + shdrs[i].offset);
        }
    }

    if(!stringtab) {
        ERROR("ELF contains no object string table\n");
    }

    /* Locate the symbol table */
    symtabhdr = 0;

    for(i = 0; i < hdr->shnum; i++) {
        if(shdrs[i].type == SHT_SYMTAB || shdrs[i].type == SHT_DYNSYM) {
            symtabhdr = shdrs + i;
            break;
        }
    }

    if(!symtabhdr) {
        ERROR("ELF contains no symbol table\n");
    }

    symtab = (struct elf_sym_t *)(img + symtabhdr->offset);
    symtabsize = symtabhdr->size / sizeof(struct elf_sym_t);

    /* Relocate symtab entries for quick access */
    for(i = 0; i < symtabsize; i++) {
        symtab[i].name = (uint32)(stringtab + symtab[i].name);
        printf("SYM: %s / %08lx / %08lx / %d\r\n",
               (char*)symtab[i].name, symtab[i].value,
               symtab[i].size, symtab[i].shndx);
    }

    for(i = 0; i < hdr->shnum; i++) {
        printf("  Section %d: (%08lx/%08lx)\n", i, shdrs[i].name, shdrs[i].type);
    }

    /* Build the final memory image */
    sz = build_memory_image(shdrs, hdr->shnum);
    printf("Final image is %zu bytes\n", sz);
    imgout = malloc(sz);
    if(!imgout) {
        ERROR("Cannot allocate image.\n");
        goto cleanup;
    }

    for(i = 0; i < hdr->shnum; i++) {
        if(shdrs[i].flags & SHF_ALLOC) {
            if(shdrs[i].type == SHT_NOBITS) {
                printf("%d:  setting %ld bytes of zeros at %08lx\n",
                       i, shdrs[i].size, shdrs[i].addr);
                memset(imgout + shdrs[i].addr, 0, shdrs[i].size);
            }
            else {
                printf("%d:  copying %ld bytes from %08lx to %08lx\n",
                       i, shdrs[i].size, shdrs[i].offset, shdrs[i].addr);
                memcpy(imgout + shdrs[i].addr,
                       img + shdrs[i].offset,
                       shdrs[i].size);
            }
        }
    }

    /* Find the RELA section; FIXME: More than one RELA section, REL sections */
    reltab = 0;
    /*for (i=0; i<hdr->shnum; i++) {
        if (shdrs[i].type == SHT_RELA) {
            reltab = (struct elf_rela_t *)(img + shdrs[i].offset);
            break;
        }
    }
    if (!reltab) {
        printf("ELF contains no RELA section (did you use -r?)\n");
        return 0;
    }
    reltabsize = shdrs[i].size / sizeof(struct elf_rela_t); */

    /* Process the relocations */
    for(i = 0; i < hdr->shnum; i++) {
        if(shdrs[i].type != SHT_RELA) continue;

        reltab = (struct elf_rela_t *)(img + shdrs[i].offset);
        reltabsize = shdrs[i].size / sizeof(struct elf_rela_t);
        sect = shdrs[i].info;

        printf("Relocating on section %d\r\n", sect);

        for(j = 0; j < reltabsize; j++) {
            int sym;

            if(ELF32_R_TYPE(reltab[j].info) != R_SH_DIR32) {
                ERROR("ELF contains unknown RELA type %02x\r\n",
                       ELF32_R_TYPE(reltab[j].info));
            }

            sym = ELF32_R_SYM(reltab[j].info);
            printf("  Writing REL %08lx(%08lx+%08lx+%08lx+%08lx) -> %08lx\r\n",
                   vma + shdrs[symtab[sym].shndx].addr + symtab[sym].value + reltab[j].addend,
                   vma, shdrs[symtab[sym].shndx].addr, symtab[sym].value, reltab[j].addend,
                   vma + shdrs[sect].addr + reltab[j].offset);
            *((uint32*)(imgout
                        + shdrs[sect].addr      /* assuming 1 == .text */
                        + reltab[j].offset))
            +=    vma
                  + shdrs[symtab[sym].shndx].addr
                  + symtab[sym].value
                  + reltab[j].addend;
        }
    }

    /* Look for the kernel negotiation symbols and deal with that */
    {
        int mainsym, getsvcsym, notifysym;

        mainsym = find_sym("_ko_main", symtab, symtabsize);

        if(mainsym < 0) {
            ERROR("ELF contains no _ko_main\n");
        }

        getsvcsym = find_sym("_ko_get_svc", symtab, symtabsize);

        if(mainsym < 0) {
            ERROR("ELF contains no _ko_get_svc\n");
        }

        notifysym = find_sym("_ko_notify", symtab, symtabsize);

        if(notifysym < 0) {
            ERROR("ELF contains no _ko_notify\n");
        }

        /* Patch together getsvc and notify for now */
        *((uint32*)(imgout
                    + shdrs[symtab[getsvcsym].shndx].addr
                    + symtab[getsvcsym].value))
        = vma
          + shdrs[symtab[notifysym].shndx].addr
          + symtab[notifysym].value;
    }

cleanup:
    if(!ret) {
        *outsz = sz;
        *out = imgout;
    }
    else if(imgout) {
        free(imgout);
    }

    if(img) free(img);

    return ret;
}

int main(int argc, char **argv) {
    char * out;
    size_t sz;

    if(argc != 3) {
        puts("Usage: <infile> <outfile>");
        return 0;
    }

    if(elf_load(&out, &sz, argv[1], 0x8c010000)) {
        fprintf(stderr, "Cannot load ELF file.\n");
        return 1;
    }

    if(write_file_contents(argv[2], out, sz)) {
        fprintf(stderr, "Cannot write image.\n");
        return 2;
    }

    return 0;
}
