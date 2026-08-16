#include <proc.h>
#include <elf.h>
#include "fs.h"

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf_Ehdr
#endif
#define ELF_MAGIC 0x464c457f
#define MIN(x,y) (x < y ? x : y)
#define MAX(x,y) (x > y ? x : y)
int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_write(int fd, const void *buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);
int fs_close(int fd);
void map(AddrSpace *as, void *va, void *pa, int prot) ;
void protect(AddrSpace *as);
void* new_page(size_t nr_page);
static uintptr_t loader(PCB *pcb, const char *filename) {
  // 1. 打开文件
  int fd = fs_open(filename, 0, 0);
  // 如果需要检查 fd 是否有效，可加 assert(fd >= 0);

  Elf_Ehdr ehdr;

  // 2. 读取 ELF 头（从文件开头读取 sizeof(Elf_Ehdr) 字节）
  fs_read(fd, &ehdr, sizeof(Elf_Ehdr));

  // 3. 检查魔数
  assert(*(uint32_t*)ehdr.e_ident == ELF_MAGIC);
  protect(&pcb->as);
  printf("filename=%s, root_pa=%p\n", filename, pcb->as.ptr);
  for (int i = 0; i < ehdr.e_phnum; i++) {
    Elf_Phdr phdr;
    fs_lseek(fd, ehdr.e_phoff + i * ehdr.e_phentsize, SEEK_SET);
    fs_read(fd, &phdr, sizeof(Elf_Phdr));

    if (phdr.p_type == PT_LOAD) {
      // 计算段的起始和结束虚拟地址（页对齐）
      uintptr_t seg_start = phdr.p_vaddr;
      uintptr_t seg_end = phdr.p_vaddr + phdr.p_memsz;
      uintptr_t va = ROUNDDOWN(seg_start, PGSIZE);
      uintptr_t end_va = ROUNDUP(seg_end, PGSIZE);

      // 记录文件读取位置
      size_t file_offset = phdr.p_offset;  // 文件中的起始偏移
      // size_t remaining_file_size = phdr.p_filesz;  // 文件中有多少字节属于该段

      for (; va < end_va; va += PGSIZE) {
        void *pa = new_page(PGSIZE);
        memset(pa, 0, PGSIZE);
        map(&pcb->as, (void*)va, pa, 0);

        // 计算该页在段内的偏移（可能为0，也可能非0）
        uintptr_t page_start = va;
        uintptr_t page_end = va + PGSIZE;

        // 该页与段有重叠的部分
        uintptr_t overlap_start = MAX(page_start, seg_start);
        uintptr_t overlap_end = MIN(page_end, seg_end);

        if (overlap_start < overlap_end) {
          // 该页有数据（可能来自文件或BSS）
          if (overlap_start < phdr.p_vaddr + phdr.p_filesz) {
            // 该页有文件数据需要复制
            size_t file_bytes_to_copy = MIN(overlap_end, phdr.p_vaddr + phdr.p_filesz) - overlap_start;
            // 文件中的偏移 = 段起始文件偏移 + (overlap_start - seg_start)
            size_t file_off = file_offset + (overlap_start - seg_start);
            // 写入到物理页的偏移 = overlap_start - page_start
            uint8_t *dest = (uint8_t*)pa + (overlap_start - page_start);

            // 读取文件数据
            fs_lseek(fd, file_off, SEEK_SET);
            fs_read(fd, dest, file_bytes_to_copy);
          }
          // 其余部分（bss）已经由 memset 清零，无需处理
        }
      }

      // 更新程序堆顶：取段结束地址向上对齐后的值
      pcb->max_brk = ROUNDUP(seg_end, PGSIZE);
    }
  }

  // 5. 关闭文件
  fs_close(fd);

  // 6. 返回入口地址
  return ehdr.e_entry;
}

uintptr_t naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  // Log("Jump to entry = %p", entry);
  // ((void(*)())entry) ();
  return entry;
}

