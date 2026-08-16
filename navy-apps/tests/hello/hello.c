#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#define SYS_yield 1
extern int _syscall_(int, uintptr_t, uintptr_t, uintptr_t);
int main(int argc, char *argv[], char *envp[]) {
  // write(1, "Hello World!\n", 13);

  int i = 2;
  volatile int j = 0;
  // yield();
  // _syscall_(SYS_yield, 0, 0, 0);
  while (1) {
    j ++;
    if (j == 1000000) {
      printf("%s for the %dth time!\n",argv[1], i ++);
      j = 0;
      // _syscall_(SYS_yield, 0, 0, 0);
    }
  }
  return 0;
}
