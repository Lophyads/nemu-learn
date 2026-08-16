#include <proc.h>
#include <os_define.h>
#define MAX_NR_PROC 1000

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
static int pcb_num = 0;
// static int switch_id = 0;
int fg_cb = 1; // 前台程序
static int schedule_num = 0;
PCB *current = NULL;
uintptr_t naive_uload(PCB *pcb, const char *filename);
Context *kcontext(Area kstack, void (*entry)(void *), void *arg);
void context_kload(PCB *p, void (*entry)(void *), void *arg);
Context *ucontext(AddrSpace *as, Area kstack, void *entry);
void context_uload(PCB *p, const char* filename, char *const argv[], char *const envp[]);
uintptr_t loader(PCB *pcb, const char *filename);
void* new_page(size_t nr_page);
uintptr_t translate_va_to_pa(AddrSpace *as, void *va);
void switch_boot_pcb() {
  current = &pcb_boot;
}
// static char *argv[]__attribute__((used)) = {"--skip", NULL};
static char *argv[]__attribute__((used)) = (char *[]){NULL};
static char *envp[]__attribute__((used)) = (char *[]){NULL};
enum {
  EXEC_TEST,
  BUSY_BOX,
  NTERM,
  BIRD,
  PAL,
  DUMMY,
  CPPTEST,
  HELLO,
  NSLIDER
} ItemName;

static struct Item {
  const char *pathname;
  char *const *argv;
  char *const *envp;
} items[]__attribute__((used)) = {
  [EXEC_TEST] = {"/bin/exec-test", (char *[]){"/bin/exec-test", NULL}, NULL},
  [BUSY_BOX] = {"/bin/busybox", (char *[]){"/bin/busybox", NULL}, NULL},
  [NTERM] = {"/bin/nterm", (char *[]){"/bin/nterm", NULL}, NULL},
  [BIRD] = {"/bin/bird", (char *[]){"/bin/bird", NULL}, NULL},
  [PAL] = {"/bin/pal", (char *[]){"/bin/pal", NULL}, NULL},
  [DUMMY] = {"/bin/dummy", (char *[]){"/bin/dummy", NULL}, NULL},
  [CPPTEST] = {"/bin/cpp-test", (char *[]){"/bin/cpp-test", NULL}, NULL},
  [HELLO] = {"/bin/hello", (char *[]){"/bin/hello", NULL}, NULL},
  [NSLIDER] = {"/bin/nslider", (char *[]){"/bin/nslider", NULL}, NULL},
};

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    Log("Hello World from Nanos-lite with arg '%p' for the %dth time!", (uintptr_t)arg, j);
    j ++;
    yield();
  }
}

void my_schedule(void *arg) {
  int j = 1;
  while (1) {
    Log("Hello World from Nanos-lite with arg '%p' for the %dth time!", (uintptr_t)arg, j);
    yield();
  }
}

void init_proc() {
  switch_boot_pcb();
  uintptr_t boot_kstack_top = (uintptr_t)&pcb_boot + sizeof(PCB); // 或者 (uintptr_t)pcb_boot.stack + STACK_SIZE
  asm volatile("csrw mscratch, %0" : : "r"(boot_kstack_top));
  Log("Initializing processes...");

  // load program here
  // naive_uload(NULL, "/bin/file-test");
  // naive_uload(NULL, "/bin/bmp-test");
  // naive_uload(NULL, "/bin/nslider");
  // naive_uload(NULL, "/bin/pal");
  // naive_uload(NULL, "/bin/nterm");
  // naive_uload(NULL, "/bin/cpp-test");
  // context_kload(hello_fun, (void *)1);
  // pcb_boot.as = &kas;
  // context_kload(&pcb_boot, my_schedule, (void *)1);
  // context_uload("/bin/pal", argv, envp);
  // context_uload(NULL, items[EXEC_TEST].pathname, items[EXEC_TEST].argv, items[EXEC_TEST].envp);
  // context_uload(NULL, items[CPPTEST].pathname, items[CPPTEST].argv, items[CPPTEST].envp);
  
  context_uload(NULL, items[HELLO].pathname, (char *[]){"/bin/hello", "AAAA", NULL}, items[HELLO].envp);
  context_uload(NULL, items[PAL].pathname, items[PAL].argv, items[PAL].envp);
  context_uload(NULL, items[BIRD].pathname, items[BIRD].argv, items[BIRD].envp);
  context_uload(NULL, items[NSLIDER].pathname, items[NSLIDER].argv, items[NSLIDER].envp);
  // context_uload(&pcb[1], items[NTERM].pathname, items[NTERM].argv, items[NTERM].envp);
  // context_uload(NULL, items[HELLO].pathname, (char *[]){"/bin/hello", "BBBB", NULL}, items[HELLO].envp);
  // context_uload(NULL, items[EXEC_TEST].pathname, items[EXEC_TEST].argv, items[EXEC_TEST].envp);
  Log("init_proc end");
}

Context* schedule(Context *prev) {
  // printf("schedule prev switch_id=%d, pcb_num=%d, entry=%p, addr=%p\n",switch_id, pcb_num, prev->mepc, &(*prev));
  // Log("schedule pre switch_id=%d, pcb_0_root_pa=%p, pcb_1_root_pa=%p, &cur=%p, &pcb_boot=%p, &pcb0=%p, &pcb1=%p", switch_id, pcb[0].cp->pdir, pcb[1].cp->pdir,  &(*current), &(*pcb_boot.cp), &(*pcb[0].cp), &(*pcb[1].cp));
  current->cp = prev;
  // Log("schedule after switch_id=%d, pcb_0_root_pa=%p, pcb_1_root_pa=%p, &cur=%p, &pcb_boot=%p, &pcb0=%p, &pcb1=%p", switch_id, pcb[0].cp->pdir, pcb[1].cp->pdir,  &(*current), &(*pcb_boot.cp), &(*pcb[0].cp), &(*pcb[1].cp));
  // Log("schedule after switch_id=%d, pcb_0_root_pa=%p, pcb_1_root_pa=%p, prev_root_pa=%p, current=%p", switch_id, pcb[0].cp->pdir, pcb[1].cp->pdir, prev->pdir, &(*current));
  // if (current == &pcb_boot)
  // {
    // if (pcb_num)
    // {
      // switch_id = (switch_id + 1) % pcb_num;
      // current = &pcb[switch_id];
    // }
    // Log("switch_next_pcb switch_id=%d, pcb_0_root_pa=%p, pcb_1_root_pa=%p, current=%p", switch_id, pcb[0].cp->pdir, pcb[1].cp->pdir, &(*current));
  // }
  // else{
  //   // Log("switch_boot_pcb");
  //   // printf("schedule after switch_id=%d, pcb_num=%d, entry=%p, addr=%p\n",switch_id, pcb_num, current->cp->mepc, &(*current));
  //   switch_boot_pcb();
  //   pcb_boot.cp->pdir = NULL;
  //   // Log("switch_boot_pcb switch_id=%d, pcb_0_root_pa=%p, pcb_1_root_pa=%p", switch_id, pcb[0].cp->pdir, pcb[1].cp->pdir);
  // }
  // printf("schedule after switch_id=%d, pcb_num=%d, entry=%p, addr=%p\n",switch_id, pcb_num, current->cp->mepc, &(*current));
  assert(pcb_num > 1);
  if (current == &pcb[fg_cb]){
    if (schedule_num ++ > 100){ // 硬件中断100次后再调度后台程序
      schedule_num = 0;
      current = &pcb[0];
    }
  }
  else{
    current = &pcb[fg_cb];
  }
  
  return current->cp;
}
// 0x81e04000(1) -> 0x8409e000 
// 0x81df4000(boot) -> 0x8409e000
// 0x81dfc000(0) -> 0x84089000
void context_kload(PCB *p, void (*entry)(void *), void *arg){
  if (pcb_num > MAX_NR_PROC){
    panic("not reach here");
  }
  if (p == NULL) p = &pcb[pcb_num++];
  p->cp = kcontext((Area) { p->stack, p + 1 }, entry, arg);
}

void context_uload(PCB *p, const char* filename, char *const vargv[], char *const venvp[]){
  if (pcb_num > MAX_NR_PROC){
    panic("not reach here");
  }
  if (p == NULL) p = &pcb[pcb_num++];
  uintptr_t entry = naive_uload(p, filename);
  p->cp = ucontext(&p->as, (Area) { p->stack, p + 1 }, (void*)entry);
  printf("context_uload entry=%p\n", entry);
// 计算参数个数
  int argc = 0;
  if (vargv) {
      while (vargv[argc] != NULL) argc++;
  }
  int envc = 0;
  if (venvp) {
      while (venvp[envc] != NULL) envc++;
  }

  uintptr_t area_end = (uintptr_t)(p->as.area.end); 
  // area_end = area_end - pcb_num * PGSIZE * 8; // todo
  uintptr_t va = area_end - PGSIZE * 8; // 分配32k的栈
  for (; va < area_end; va += PGSIZE) {
    void* pa = new_page(PGSIZE);
    memset((char*)pa, 0, PGSIZE);
    map(&p->as, (void*)va, pa, 0);
  }

  uintptr_t sp = area_end;
  uintptr_t argv_str_addr[64];
  uintptr_t envp_str_addr[64];

  // 写环境变量字符串（通过物理地址）
  for (int i = 0; i < envc; i++) {
      size_t len = strlen(venvp[i]) + 1;
      sp -= len;
      uintptr_t pa = translate_va_to_pa(&p->as, (void*)sp);
      assert(pa != 0);
      memcpy((void*)pa, venvp[i], len);
      envp_str_addr[i] = sp;
  }

  // 写 argv 字符串
  for (int i = 0; i < argc; i++) {
      size_t len = strlen(vargv[i]) + 1;
      sp -= len;
      uintptr_t pa = translate_va_to_pa(&p->as, (void*)sp);
      assert(pa != 0);
      memcpy((void*)pa, vargv[i], len);
      argv_str_addr[i] = sp;
  }

  sp &= ~(sizeof(uintptr_t) - 1);

  // 写 envp 指针数组
  sp -= (envc + 1) * sizeof(uintptr_t);
  uintptr_t pa_envp = translate_va_to_pa(&p->as, (void*)sp);
  assert(pa_envp != 0);
  uintptr_t *envp_array = (uintptr_t*)pa_envp;
  for (int i = 0; i < envc; i++) envp_array[i] = envp_str_addr[i];
  envp_array[envc] = 0;

  // 写 argv 指针数组
  sp -= (argc + 1) * sizeof(uintptr_t);
  uintptr_t pa_argv = translate_va_to_pa(&p->as, (void*)sp);
  assert(pa_argv != 0);
  uintptr_t *argv_array = (uintptr_t*)pa_argv;
  for (int i = 0; i < argc; i++) argv_array[i] = argv_str_addr[i];
  argv_array[argc] = 0;

  // 设置用户寄存器
  p->cp->GPR2 = argc;
  p->cp->GPR3 = (uintptr_t)sp;                                  // argv 数组地址
  p->cp->GPR4 = (uintptr_t)(sp + (argc + 1) * sizeof(uintptr_t)); // envp 数组地址
  p->cp->GPRx = sp; // a3
}


Context* handler_execve(char* pathname, void* argv, void* envp){
  // pcb_boot.cp->pdir = NULL; // 记录用户进程的虚拟页目录
  context_uload(current, pathname, (char **)argv, (char **)envp);// 把当前进程覆盖
  // switch_boot_pcb();
  return current->cp; // 切换到系统内核上下文
}


uintptr_t handler_brk(uintptr_t new_brk){
  // printf("new_brk=%p, current->max_brk=%p\n", new_brk, current->max_brk);
  if (new_brk <= current->max_brk){
    // current->max_brk = new_brk;
    return new_brk;
  }
  uintptr_t va = current->max_brk; 
  va = ROUNDUP(va, PGSIZE);  
  for (; va < new_brk; va += PGSIZE) {
    void* pa = new_page(PGSIZE);
    memset((char*)pa, 0, PGSIZE);
    map(&current->as, (void*)va, pa, 0);
  }
  current->max_brk = va;
  return new_brk;
}
