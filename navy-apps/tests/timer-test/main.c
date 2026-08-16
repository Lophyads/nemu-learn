#include <stdio.h>
#include <NDL.h>
#include <assert.h>

int main() {
    int n = 10;
    int sec = 1;
    while (n --) {
       while(NDL_GetTicks() / 1000000 < sec);
        printf("%d second).\n", sec);

        sec ++;
    }

  printf("PASS!!!\n");

  return 0;
}
