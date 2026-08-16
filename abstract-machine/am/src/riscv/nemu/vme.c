#include <am.h>
#include <nemu.h>
#include <klib.h>
static AddrSpace kas = {};
static void* (*pgalloc_usr)(int) = NULL;
static void (*pgfree_usr)(void*) = NULL;
static int vme_enable = 0;

static Area segments[] = {      // Kernel memory mappings
  NEMU_PADDR_SPACE
};

#define USER_SPACE RANGE(0x40000000, 0x80000000)

static inline void set_satp(void *pdir) {
  uintptr_t mode = 1ul << (__riscv_xlen - 1);
  asm volatile("csrw satp, %0" : : "r"(mode | ((uintptr_t)pdir >> 12)));
}

static inline uintptr_t get_satp() {
  uintptr_t satp;
  asm volatile("csrr %0, satp" : "=r"(satp));
  return satp << 12;
}

static PTE make_leaf_pte(__uint64_t pa, int prot) {
    PTE pte = ((pa >> 12) << 10);   // PPN 占据 PTE[53:10]
    if (prot & 1) pte |= PTE_R;     // PROT_READ
    if (prot & 2) pte |= PTE_W;     // PROT_WRITE
    if (prot & 4) pte |= PTE_X;     // PROT_EXEC
    if (prot & 8) pte |= PTE_U;     // user accessible
    pte |= PTE_V;                   // 有效位
    return pte;
}

// 构造非叶子 PTE（指向下一级页表）
static PTE make_nonleaf_pte(__uint64_t pa) {
    return ((pa >> 12) << 10) | PTE_V;   // R=W=X=0
}

bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  pgalloc_usr = pgalloc_f;
  pgfree_usr = pgfree_f;

  kas.ptr = pgalloc_f(PGSIZE);

  int i;
  printf("kas.ptr=%p, %d\n", kas.ptr, sizeof(PTE));
  for (i = 0; i < LENGTH(segments); i ++) {
    
    void *va = segments[i].start;
    void *end = (char*)va + 0x20000;
    end = segments[i].end;
    printf("seg_num=%d, %p\n", (end - segments[i].start) / PGSIZE, end);
    // for (; va < segments[i].end; va += PGSIZE) {
    for (; va < end; va += PGSIZE) {
      map(&kas, va, va, 0);
    }
  }

  set_satp(kas.ptr);
  vme_enable = 1;

  return true;
}

void protect(AddrSpace *as) {
  PTE *updir = (PTE*)(pgalloc_usr(PGSIZE));
  as->ptr = updir;
  as->area = USER_SPACE;
  as->pgsize = PGSIZE;
  // map kernel space
  memcpy(updir, kas.ptr, PGSIZE);
}

void unprotect(AddrSpace *as) {
}

void __am_get_cur_as(Context *c) {
  c->pdir = (vme_enable ? (void *)get_satp() : NULL);
}

void __am_switch(Context *c) {
  if (vme_enable && c->pdir != NULL) {
    set_satp(c->pdir);
  }
}
uintptr_t translate_va_to_pa(AddrSpace *as, void *va) {
  uintptr_t vaddr = (uintptr_t)va;
  uintptr_t *pt = (uintptr_t *)as->ptr;  // 根页表（内核可访问其物理地址）
  // Sv39: 39位虚拟地址，三级页表
  uintptr_t vpn[3];
  vpn[0] = (vaddr >> 12) & 0x1ff;
  vpn[1] = (vaddr >> 21) & 0x1ff;
  vpn[2] = (vaddr >> 30) & 0x1ff;

  uintptr_t pte = pt[vpn[2]];
  if (!(pte & 0x1)) return 0;
  uintptr_t *pt1 = (uintptr_t *)((pte >> 10) << 12);

  pte = pt1[vpn[1]];
  if (!(pte & 0x1)) return 0;
  uintptr_t *pt0 = (uintptr_t *)((pte >> 10) << 12);

  pte = pt0[vpn[0]];
  if (!(pte & 0x1)) return 0;
  uintptr_t ppn = (pte >> 10) << 12;
  return ppn | (vaddr & 0xfff);
}

void map(AddrSpace *as, void *va, void *pa, int prot) {
  __uint64_t vaddr = (__uint64_t)va;
  __uint64_t paddr = (__uint64_t)pa;

  // 1. 正确提取 VPN[2], VPN[1], VPN[0]（每个 9 位）
  int vpn2 = (vaddr >> 30) & 0x1FF;
  int vpn1 = (vaddr >> 21) & 0x1FF;
  int vpn0 = (vaddr >> 12) & 0x1FF;
  // offset 不使用，因为 map 只建立页映射

  PTE *l1 = (PTE *)as->ptr;   // 一级页表（根页表）
  // printf("map l1 %p\n", l1);
  // 2. 一级查找，若无效则分配二级页表
  if (!(l1[vpn2] & PTE_V)) {
      PTE *l2_page = pgalloc_usr(PGSIZE);
      // 清零新页表
      // printf("map l2_page %p\n", l2_page);
      l1[vpn2] = make_nonleaf_pte((__uint64_t)l2_page);  // 恒等映射下地址即物理地址
  }
  PTE *l2 = (PTE *)((l1[vpn2] >> 10) << 12);   // 从 PPN 恢复虚拟地址
  // printf("map l2 %p\n", l2);
  // 3. 二级查找，若无效则分配三级页表
  if (!(l2[vpn1] & PTE_V)) {
      PTE *l3_page = pgalloc_usr(PGSIZE);
      l2[vpn1] = make_nonleaf_pte((__uint64_t)l3_page);
  }
  PTE *l3 = (PTE *)((l2[vpn1] >> 10) << 12);
  // printf("map 333 %p, %p, %p\n", l1, l2, l3);
  // 4. 在三级页表中填入最终映射（叶子 PTE）
  l3[vpn0] = make_leaf_pte(paddr, prot);
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  Context* ctx = (Context*)(kstack.end - sizeof(Context));
  ctx->mepc = (uintptr_t)entry;
  ctx->pdir = as->ptr;
  return ctx;
}
