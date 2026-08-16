/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <signal.h>

// this should be enough
static int idx = 0;
#define N 65536
static char buf[N] = {};
static char code_buf[N + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  __uint64_t result = %s; "
"  printf(\"%%lu\", result); "
"  return 0; "
"}";

static void gen_rand_expr() {
  if (idx >= N / 2){
      buf[idx++] = (char)(rand() % 9 + '1');
      return ; 
  }
  char op_list[] = {'+', '-', '*', '/'};
  switch (rand() % 4)
  {
    case 0:
      buf[idx++] = (char)(rand() % 9 + '1');
      break;
    case 1:
      buf[idx++] = '(';
      gen_rand_expr();
      buf[idx++] = ')';
      break;
    case 2:
      buf[idx++] = ' ';
      gen_rand_expr();
      break;
    default:
      gen_rand_expr();
      buf[idx++] = op_list[rand() % 4];
      gen_rand_expr();
      break;
  }
  buf[idx] = '\0';
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    idx = 0;
    buf[idx] = '\0';
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    char s[1000000];
    fp = popen("gcc /tmp/.code.c -o /tmp/.expr 2>&1", "r");
    assert(fp != NULL);
    int ret = fscanf(fp, "%s", s);
    pclose(fp);
    if (ret != EOF) {
      continue;
    }

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);
    int64_t result;
    ret = fscanf(fp, "%lu", &result);
    pclose(fp);

    fp = fopen("/data/ics2024/nemu/input.txt", "a");
    assert(fp != NULL);
    fprintf(fp, "%lu %s\n", result, buf);
    fflush(fp);  // 强制刷新缓冲区
    fclose(fp);
  }
  return 0;
}
