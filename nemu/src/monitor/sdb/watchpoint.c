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

#include "sdb.h"

#define NR_WP 32
#define N 100000
typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char expr[N];
  word_t old_val;
  bool is_expr;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp(){
    assert(free_ != NULL);
    WP* tmp = free_;
    free_ = free_->next;
    tmp->next = NULL;
    return tmp;
}

void free_wp(WP *wp){
  wp->next = free_;
  free_ = wp;
}

bool add_wp(char* e){
  bool success = true;
  WP* wp = new_wp();
  wp->old_val = expr(e, &success, &wp->is_expr);
  strcpy(wp->expr, e);
  if (success) {
    if (head)
      wp->next = head;
    head = wp;
  }
  return success;
}

bool del_wp(int idx){
  WP* cur = head;
  WP* pre = NULL;
  while(cur != NULL){
    if (cur->NO == idx){
      if (pre == NULL) head = cur->next;
      else pre->next = cur->next;
      free_wp(cur);
      return true;
    }
    pre = cur;
    cur = cur->next;
  }
  return false;
}
bool check_watch_point(vaddr_t pc){
  WP* cur = head;
  bool succ = true;
  bool _ = false;
  bool trigger = false;
  while (cur != NULL)
  {
    if (cur->is_expr){
      if(expr(cur->expr, &succ, &_)){
        Log("watch point [id=%d] [%s]", cur->NO, cur->expr);
      }
    }
    else{
      word_t res = expr(cur->expr, &succ, &_);
      if(res != cur->old_val){
        Log("watch point [id=%d] [old_val=%lu] [new_val=%lu]", cur->NO, cur->old_val, res);
        cur->old_val = res;
        trigger = true;
      }
    }
    cur = cur->next;
  }
  return trigger;
}

void watch_point_display(){
  WP* cur = head;
  while(cur != NULL){
    printf("[%d] [%s]\n", cur->NO, cur->expr);
    cur = cur->next;
  }
}