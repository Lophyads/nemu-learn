#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
// #define HAVE_INITFINI_ARRAY
int main(int argc, char *argv[], char *envp[]);
extern char **environ;
void call_main(uintptr_t *args) {
  printf("111111\n");
  register uintptr_t sp asm("sp");
  printf("Current sp = %p\n", (void *)sp);

  int    argc  = (int)    args[0];
  char **argv  = (char **)args[1];
  char **envp  = (char **)args[2];

  printf("call_main argc = %d\n", argc);
  for(int i = 0; i < argc; ++ i){
    printf("argv[%d] = %s\n",i, argv[i]);
  }
  // char *empty[] =  {NULL };
  environ = envp;
  __libc_init_array();
  
  // printf("main ret=%d\n", ret);
  // uintptr_t *stack = (uintptr_t *)sp;
  // int dump_words = 64;   // 打印 64 个 word，可调整

  // for (int i = 0; i < dump_words; i++) {
  //     printf("stack[%2d] addr=%p value=0x%016lx\n",
  //           i, (void *)&stack[i], stack[i]);
  // }

  exit(main(argc, argv, environ));
  assert(0);
}
