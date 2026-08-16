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

// #define MSTATUS_MIE_MASK   (1u << 3)
// #define MSTATUS_MPIE_MASK  (1u << 7)
word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */
  cpu.csr[CSR_MEPC] = epc;
  cpu.csr[CSR_MCAUSE] = NO;
  cpu.pc = cpu.csr[CSR_MTVEC];

  word_t mstatus = cpu.csr[CSR_MSTATUS];
  if (mstatus & MSTATUS_MIE_MASK)
        mstatus |= MSTATUS_MPIE_MASK;   // 原来 MIE=1，MPIE 置 1
    else
        mstatus &= ~MSTATUS_MPIE_MASK;  // 原来 MIE=0，MPIE 清 0

  // 2. 将 MIE 清 0，禁止机器模式全局中断
  mstatus &= ~MSTATUS_MIE_MASK;
  cpu.csr[CSR_MSTATUS] = mstatus;
  return cpu.csr[CSR_MTVEC];
}

word_t isa_query_intr() {
  // Log("INTR=%d, CSR_MSTATUS=%ld", cpu.INTR, cpu.csr[CSR_MSTATUS]);
  if ( cpu.INTR && (cpu.csr[CSR_MSTATUS] & MSTATUS_MIE_MASK) ) {
    // printf("isa_query_intr-----------------\n");
    cpu.INTR = 0;
    return IRQ_TIMER;
  }
  return INTR_EMPTY;
}
