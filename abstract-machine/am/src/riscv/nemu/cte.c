#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>
#define IRQ_TIMER 0x8000000000000007
static Context* (*user_handler)(Event, Context*) = NULL;
void __am_get_cur_as(Context *c);
void __am_switch(Context *c);
// static Context tmp;
Context* __am_irq_handle(Context *c) {
  __am_get_cur_as(c);
  if (user_handler) {
    Event ev = {0};
    switch (c->mcause) {
      case 0: 
        ev.event = EVENT_SYSCALL; c->mepc += 4;  break;
      case -1: 
        ev.event = EVENT_YIELD; c->mepc += 4; break;
      case IRQ_TIMER:
        ev.event = EVENT_IRQ_TIMER; break;
      default: ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }
  __am_switch(c);
  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap)); // 将异常入口地址设置到mtvec寄存器中

  // register event handler
  user_handler = handler; // 注册一个事件处理回调函数

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  Context* ctx = (Context*)(kstack.end - sizeof(Context));
  ctx->mepc = (uintptr_t)entry;
  ctx->GPR2 = (uintptr_t)arg;
  // ctx->GPRx = (uintptr_t)heap.end;
  // ctx->GPRx = (uintptr_t)0x88000000;
  // ctx->mstatus = 0xa00001800; 
  return ctx;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
