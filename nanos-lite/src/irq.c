#include <common.h>
#include "syscall.h"

Context* do_syscall(Context *c);
Context* schedule(Context *prev);
static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_SYSCALL: {
      return do_syscall(c);
      break;
    }
    case EVENT_YIELD:{
      // Log("YIELD");
      // do_syscall(c);
      return schedule(c);
      break;
    }
    case EVENT_IRQ_TIMER:{
      // Log("IRQ_TIMER");
      return schedule(c);
      break;
    }
    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
