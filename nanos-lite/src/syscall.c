#include <common.h>
#include "syscall.h"
#include "fs.h"

// #define CONFIG_STRACE
#ifdef CONFIG_STRACE
static const char *syscall_names[] = {
  [SYS_exit]  = "exit",
  [SYS_yield] = "yield",
  [SYS_write] = "write",
  [SYS_close] = "close",
  [SYS_brk] = "brk",
  [SYS_open] = "open",
  [SYS_read] = "read",
  [SYS_lseek] = "lseek",
  [SYS_gettimeofday] = "gettimeofday",
  [SYS_execve] = "execve",
};
#endif

#define ARRLEN(x) (sizeof(x)/sizeof(x[0]))
int handler_write(Context *c);
int handler_read(Context *c);
Context* handler_execve(char* pathname, void* argv, void* envp);
Context* schedule(Context *prev);
uintptr_t handler_brk(uintptr_t new_brk);
Context* do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;

#ifdef CONFIG_STRACE
  if (a[0] < ARRLEN(syscall_names) && syscall_names[a[0]])
    Log("strace: %s(%p, %p, %p)", syscall_names[a[0]], a[1], a[2], a[3]);
  else
    Log("strace: syscall(%d, %p, %p, %p)", a[0], a[1], a[2], a[3]);
#endif
  switch (a[0]) {
    case SYS_exit:
    {
      halt(c->GPR2);
      c->GPR2 = 0; 
      break;
    }
    case SYS_yield:
    {
      // yield();
      c = schedule(c); 
      // c->GPR2 = 0;
      break;
    }
    case SYS_write:
    {
      c->GPR2 = handler_write(c);
      // c = schedule(c);
      break;
    }
    case SYS_close:
    {
      c->GPR2 = 0; // todo
      break;
    }
    case SYS_brk:
    {
      c->GPR2 = handler_brk((uintptr_t)a[1]);
      // c->GPR2 = 0; // todo
      break;
    }
    case SYS_open:
    {
      c->GPR2 = fs_open((char*)a[1], a[2], a[3]);
      // c = schedule(c);
      break;
    }
    case SYS_read:
    {
      c->GPR2 = fs_read(a[1], (char*)a[2], a[3]);
      break;
    }
    case SYS_lseek:
    {
      c->GPR2 = fs_lseek(a[1], a[2], a[3]);
      break;
    }
    case SYS_gettimeofday:
    {
      c->GPR2 = fs_read(FD_TIMER, (char*)a[2], a[3]);
      break;
    }
    case SYS_execve:
    {
      c = handler_execve((char*)a[1], (void*)a[2], (void*)a[3]); // 切到pcb_boot,由pcb_boot进行进程切换
      // c->GPR2 = 0;
      // c = schedule(c); // 系统调用没结束时不能切进程，不然虚拟空间映射变了，返回用户态时读不会原来栈上的数据
      break;
    }
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
  return c;
}


int handler_write(Context *c){
  return fs_write(c->GPR2, (char*)c->GPR3, c->GPR4);
}

int handler_read(Context *c){
  return fs_read(c->GPR2, (char*)c->GPR3, c->GPR4);
}