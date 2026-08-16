#include <common.h>
#include <elf.h>  
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "utils.h"

typedef struct {
    uint32_t addr;
    uint32_t size;
    char name[128];
} FuncInfo;

static FuncInfo* func_list = NULL;
static size_t func_cnt = 0;

int load_elf(char* elf_path){
    Log("load elf %s", elf_path);
    int fd = open(elf_path, O_RDONLY);
    if (fd < 0) { 
        Log("No elf file.");
        return -1; 
    }
    Elf64_Ehdr ehdr;
    if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr)) {
        Log("Failed to read ELF header");
        close(fd);
        return -1;
    }

    Elf64_Shdr *shdr_table = malloc(ehdr.e_shnum * sizeof(Elf64_Shdr));
    if (!shdr_table) { 
        Log("Failed to malloc shdr_table");
        close(fd); 
        return -1; 
    }

    // 定位到节区头表起始位置
    if (lseek(fd, ehdr.e_shoff, SEEK_SET) < 0) {
        Log("lseek error");
        close(fd);
        return -1;
    }

    size_t bytes = read(fd, shdr_table, ehdr.e_shnum * sizeof(Elf64_Shdr));
    if (bytes != (size_t)(ehdr.e_shnum * sizeof(Elf64_Shdr))) {
        Log("Failed to read section headers");
        free(shdr_table);
        close(fd);
        return -1;
    }
   
    Elf64_Shdr *symtab_hdr = NULL;
    Elf64_Shdr *strtab_hdr = NULL;

    for (Elf64_Half i = 0; i < ehdr.e_shnum; i++) {
        if (shdr_table[i].sh_type == SHT_STRTAB && i != ehdr.e_shstrndx) {
            strtab_hdr = &shdr_table[i];
        } 
        else if (shdr_table[i].sh_type == SHT_SYMTAB) {
            symtab_hdr = &shdr_table[i];
        }
    }
    if (!symtab_hdr || !strtab_hdr) {
        Log("Cannot find .symtab or .strtab section");
        free(shdr_table);
        close(fd);
        return -1;
    }

    // 分配并读取符号表
    int sym_count = symtab_hdr->sh_size / sizeof(Elf64_Sym);
    Elf64_Sym *symtab = malloc(symtab_hdr->sh_size);
    if (!symtab) { 
        Log("malloc symtab error");
        free(shdr_table);
        close(fd);
        return -1;
    }
    lseek(fd, symtab_hdr->sh_offset, SEEK_SET);
    bytes = read(fd, symtab, symtab_hdr->sh_size);

    // 分配并读取字符串表
    char *strtab = malloc(strtab_hdr->sh_size);
    if (!strtab) {
        Log("malloc strtab error");
        free(symtab_hdr);
        free(shdr_table);
        close(fd);
        return -1;
    }
    lseek(fd, strtab_hdr->sh_offset, SEEK_SET);
    bytes = read(fd, strtab, strtab_hdr->sh_size);


    for (int i = 0; i < sym_count; i++) {
        int type = ELF64_ST_TYPE(symtab[i].st_info);
        if (type == STT_FUNC && symtab[i].st_size > 0) {
            func_cnt++;
        }
    }

    func_list = malloc(sizeof(FuncInfo) * func_cnt);
    if (!func_list)
    {
        Log("malloc func_list error");
        free(symtab);
        free(strtab);
        free(shdr_table);
        close(fd);
        return -1;
    }

    int idx = 0;
    for (int i = 0; i < sym_count; i++) {
        int type = ELF64_ST_TYPE(symtab[i].st_info);
        if (type == STT_FUNC && symtab[i].st_size > 0) {
            FuncInfo *f = &func_list[idx ++];
            f->addr = symtab[i].st_value;
            f->size = symtab[i].st_size;
            memcpy(f->name, strtab + symtab[i].st_name, strlen(strtab + symtab[i].st_name));
        }
    }

    {
        free(symtab);
        free(strtab);
        free(shdr_table);
        close(fd);
    }

    return 0;
}
#ifdef CONFIG_FTRACE
static int print_ftrace_level = 0;
static char tmp_buff[1024];
static char* now = tmp_buff;
void print_ftrace(uint32_t inst_addr) {
    for (int i = 0; i < func_cnt; i++) {
        if (inst_addr >= func_list[i].addr && inst_addr < func_list[i].addr + func_list[i].size) {
            now += sprintf(now, "%s", func_list[i].name);
            return; 
        }
    }
    now += sprintf(now, "???");
}
#endif


void jump_trace(uint32_t inst_addr, uint32_t func_addr){
#ifdef CONFIG_FTRACE
    now += sprintf(now, "0x%08x: ", inst_addr);
    for (int i = 0; i < print_ftrace_level; i++) {
        now += sprintf(now, " ");
    }
    print_ftrace_level++;
    now += sprintf(now, "call ");
    print_ftrace(func_addr);
    Log("%s", tmp_buff);
    now = tmp_buff;
#endif
}

void ret_trace(uint32_t inst_addr){
#ifdef CONFIG_FTRACE
    now += sprintf(now, "0x%08x: ", inst_addr);
    print_ftrace_level --;
    for (int i = 0; i < print_ftrace_level; i++) {
        now += sprintf(now, " ");
    }
    now += sprintf(now, "ret ");
    print_ftrace(inst_addr);
    Log("%s", tmp_buff);
    now = tmp_buff;
#endif
}

