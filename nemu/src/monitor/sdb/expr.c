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

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <memory/paddr.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_N_EQ, 
  TK_LE, TK_GE, TK_G, TK_L, TK_AND, TK_OR, 
  TK_PLUS, TK_DEL, TK_MUL, TK_DIV,
  TK_NUM, TK_EXP_L, TK_EXP_R,
  TK_REG, TK_HEX,
  TK_DEREF,
  TK_END
  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},         // plus
  {"\\-", TK_DEL},         // plus
  {"\\/", TK_DIV},         // plus
  {"\\*", TK_MUL},         // plus
  {"\\$[a-zA-Z0-9_]+", TK_REG}, // reg
  {"0x[0-9,a-f]+", TK_HEX}, // hex
  {"([0-9]+)", TK_NUM},   // plus
  {"\\(", TK_EXP_L},   // plus
  {"\\)", TK_EXP_R},   // plus
  {"==", TK_EQ},        // equal
  {"&&", TK_AND},
  {"!=", TK_N_EQ},
  {"<=", TK_LE},
  {">=", TK_GE},
  {">", TK_G},
  {"<", TK_L},
  {"\\|\\|", TK_OR},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

static int re_w[TK_END] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */

static int w_tot = 0;
void init_regex() {
  memset(re_w, 0, sizeof re_w);
  re_w[TK_DEREF] = ++ w_tot;
  re_w[TK_MUL] = re_w[TK_DIV] = ++ w_tot;
  re_w[TK_PLUS] = re_w[TK_DEL] = ++ w_tot;
  re_w[TK_G] = re_w[TK_GE] = re_w[TK_L] = re_w[TK_LE] = ++ w_tot;
  re_w[TK_EQ] = re_w[TK_N_EQ] = ++ w_tot;
  re_w[TK_AND] = ++ w_tot;
  re_w[TK_OR] = ++ w_tot;

  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[70];
} Token;
#define N 100005
static Token tokens[N] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE: break;
          case TK_NUM:
            ++ nr_token;
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            break;
          case TK_REG:
            ++ nr_token;
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start + 1, substr_len - 1);
            tokens[nr_token].str[substr_len - 1] = '\0';
            break;
          case TK_HEX:
            ++ nr_token;
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            break;
          default: 
            ++ nr_token;
            tokens[nr_token].type = rules[i].token_type;
            break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static bool check_parentheses(int l, int r){
  if (tokens[l].type != TK_EXP_L && tokens[r].type != TK_EXP_R) return false;
  int p = 0;
  for(int i = l; i < r; ++ i){
    if (tokens[i].type == TK_EXP_L)  ++ p;
    if (tokens[i].type == TK_EXP_R)  -- p;
    if (!p) return false;
  } 
  return true;
}

static int get_main_op(int l, int r){
  int a[TK_END] = {};
  memset(a, 0, sizeof a);
  int p = 0;
  for(int i = l; i <= r; ++ i) {
    switch (tokens[i].type)
    {
    case TK_EXP_L:
      p ++;
      break;
    case TK_EXP_R:
      Assert(p > 0, "%s %d", "get_main_op", i);
      -- p;
      break;
    default:
      if(!p && re_w[tokens[i].type] != 0){
        a[re_w[tokens[i].type]] = i;
      }
      break;
    }
  }
  int res = 0;
  for (int i = w_tot; i >= 1; -- i){
    if (a[i] != 0) {
        res = a[i]; 
        break;
      }
  }
  Assert(res != 0, "%s %d", "get_main_op", res);
  return res;
}

static sword_t eval(int l, int r){
  if (l > r){
    assert(0);
  }
  else if(l == r){
    assert(tokens[l].type == TK_NUM
    || tokens[l].type == TK_REG
    || tokens[l].type == TK_HEX);
    char *endptr = NULL;
    switch (tokens[l].type)
    {
      case TK_NUM:
        word_t val = strtoull(tokens[l].str, &endptr, 10);
        assert(*endptr == '\0');
        return val;
      case TK_REG:
        if (strcmp(tokens[l].str, "pc") == 0) {
          return cpu.pc;
        }
        return get_val_by_name(tokens[l].str);
      case TK_HEX:
        word_t tmp = strtoull(tokens[l].str, &endptr, 16);
        assert(*endptr == '\0');
        return tmp;
      default:
        assert(0);
        break;
    }
    return 0;
  }
  else if(check_parentheses(l, r)){
    return eval(l + 1, r - 1);
  }
  else{
    int op = get_main_op(l, r);
    sword_t v1 = tokens[op].type != TK_DEREF ? eval(l, op - 1) : 0;
    sword_t v2 = eval(op + 1, r);

    switch (tokens[op].type)
    {
      case TK_DEREF:
        return paddr_read(v2, 1);
      case TK_PLUS:
        return v1 + v2;
      case TK_DEL:
        return v1 - v2;
      case TK_MUL:
        return v1 * v2;
      case TK_DIV:
        return v1 / v2;
      case TK_EQ:
        return v1 == v2;
      case TK_N_EQ:
        return v1 != v2;
      case TK_G:
        return v1 > v2;
      case TK_GE:
        return v1 >= v2;
      case TK_L:
        return v1 < v2;
      case TK_LE:
        return v1 <= v2;
      case TK_AND:
        return v1 && v2;
      case TK_OR:
        return v1 || v2;
      default:
        assert(0);
        break;
    }
  }
}


word_t expr(char *e, bool *success, bool* is_expr) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  for (int i = 1; i <= nr_token; i ++) {
    if (tokens[i].type == TK_MUL && (i == 1 || (tokens[i - 1].type >= TK_EQ && tokens[i - 1].type <= TK_DIV)) ) {
      tokens[i].type = TK_DEREF;
    }
  }
  /* TODO: Insert codes to evaluate the expression. */
  // TODO();
  if (nr_token == 1) *is_expr = false;
  else *is_expr = true;
  return eval(1, nr_token);
}
