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

#ifndef __ISA_RISCV_H__
#define __ISA_RISCV_H__

#include <common.h>
#define CSR_MSTATUS  0x300  
#define CSR_MCAUSE   0x342 
#define CSR_MEPC     0x341  
#define CSR_MTVEC    0x305
#define CSR_SATP     0x180
#define MSTATUS_MIE_MASK   (1u << 3)
#define MSTATUS_MPIE_MASK  (1u << 7)
#define IRQ_TIMER 0x8000000000000007  // for riscv64
// #define HAS_VME
typedef struct {
  word_t gpr[MUXDEF(CONFIG_RVE, 16, 32)];
  word_t csr[4096]; // 特权寄存器
  vaddr_t pc;
  bool INTR;
} MUXDEF(CONFIG_RV64, riscv64_CPU_state, riscv32_CPU_state);

// decode
typedef struct {
  uint32_t inst;
} MUXDEF(CONFIG_RV64, riscv64_ISADecodeInfo, riscv32_ISADecodeInfo);

// #ifndef HAS_VME
// #define isa_mmu_check(vaddr, len, type) (MMU_DIRECT)
// #endif

#endif
