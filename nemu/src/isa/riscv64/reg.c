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
#include "local-include/reg.h"

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};
// #define CSR_MSTATUS  0x300  
// #define CSR_MCAUSE   0x342 
// #define CSR_MEPC     0x341  
// #define CSR_MTVEC    0x305
// #define CSR_SATP     0x180
void isa_reg_display() {
    for (int i = 0; i < MUXDEF(CONFIG_RVE, 16, 32); ++ i){
     printf("%s " MUXDEF(CONFIG_ISA64, "0x%016" PRIx64, "0x%08" PRIx32) " " MUXDEF(CONFIG_ISA64, "%" PRIu64, "%" PRIu32) 
     "\n", regs[i], gpr(i), gpr(i));
    }
    printf("mstatus=" MUXDEF(CONFIG_ISA64, "0x%016" PRIx64, "0x%08" PRIx32) ", mcause=" MUXDEF(CONFIG_ISA64, "0x%016" PRIx64, "0x%08" PRIx32) ", mepc=" MUXDEF(CONFIG_ISA64, "0x%016" PRIx64, "0x%08" PRIx32) ", mtvec=" MUXDEF(CONFIG_ISA64, "0x%016" PRIx64, "0x%08" PRIx32) ", satp=" MUXDEF(CONFIG_ISA64, "0x%016" PRIx64, "0x%08" PRIx32) "\n",cpu.csr[CSR_MSTATUS], cpu.csr[CSR_MCAUSE], cpu.csr[CSR_MEPC], cpu.csr[CSR_MTVEC], cpu.csr[CSR_SATP]);
}

word_t isa_reg_str2val(const char *s, bool *success) {
  return 0;
}

word_t get_val_by_name(char* name) {
  for (int idx = 0; idx < MUXDEF(CONFIG_RVE, 16, 32); ++ idx){
    if (strcmp(reg_name(idx), name) == 0){
      return gpr(idx);
    } 
  }
  Assert(0, "get_val_by_name error name=%s", name);
  return -1;
}
