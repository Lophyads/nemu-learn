#include <common.h>

void init_mm(void);
void init_device(void);
void init_ramdisk(void);
void init_irq(void);
void init_fs(void);
void init_proc(void);

int main() {
  extern const char logo[];
  printf("%s", logo);
  Log("'Hello World!' from Nanos-lite");
  Log("Build time: %s, %s", __TIME__, __DATE__);

  init_mm();

  init_device();

  init_ramdisk();

#ifdef HAS_CTE
  init_irq();
#endif

  init_fs();

  init_proc();

  Log("Finish initialization");
  
#ifdef HAS_CTE
  // while (1)
  // {
    // register uintptr_t sp1 asm("sp");
    // Log("Current sp1 = %p", (void *)sp1);
    // asm volatile("csrsi mie, 7");        // 使能定时器中断 (MTIE)
    asm volatile("csrsi mstatus, 3");    // 开启全局中断 (MIE)
    // yield();
    
    // while (1)
    // {
      /* code */
      yield();
    // }
    
    // register uintptr_t sp2 asm("sp");
    // Log("Current sp2 = %p", (void *)sp2);
  // }
  
#endif

  panic("Should not reach here");
}
