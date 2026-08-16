#include <memory.h>

static void *pf = NULL;

void* new_page(size_t nr_page) {
  nr_page = (size_t)ROUNDUP(nr_page, PGSIZE);
  void* old_pf = pf;
  pf = (char*)pf + nr_page;
  return old_pf;
}

#ifdef HAS_VME
static void* pg_alloc(int n) {
  char* ret = new_page(n);
  memset(ret, 0, (size_t)ROUNDUP(n, PGSIZE));
  return ret;
}
#endif

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
  return 0;
}

void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}
