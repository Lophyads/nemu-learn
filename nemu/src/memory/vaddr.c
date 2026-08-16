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
#include <memory/paddr.h>
#define __riscv_xlen 64

// #ifdef HAS_VME
int isa_mmu_check(vaddr_t vaddr, int len, int type){
  if (cpu.csr[CSR_SATP] >> (__riscv_xlen - 1)){
    return MMU_TRANSLATE;
  }
  return MMU_DIRECT;
}
// #endif

vaddr_t get_real_addr(vaddr_t addr, int len, int type){
  int ret = isa_mmu_check(addr, len, type);
  // int ret = MMU_DIRECT;
  switch (ret){
    case MMU_DIRECT:
    {
      // printf("get_real_addr %ld\n", cpu.csr[CSR_SATP]);
      return addr;
      break;
    }
    case MMU_TRANSLATE:
    {
      // printf("isa_mmu_translate %p, %d\n", (void*)addr, isa_mmu_translate(addr, len, type));
      return isa_mmu_translate(addr, len, type);
      break;
    }
    case MMU_FAIL:
    {
      panic("vaddr ifetch error");
      break;
    }
  }
  return addr;
}

word_t vaddr_ifetch(vaddr_t addr, int len) {
  return paddr_read(get_real_addr(addr, len, MEM_TYPE_IFETCH), len);
}

word_t vaddr_read(vaddr_t addr, int len) {
  return paddr_read(get_real_addr(addr, len, MEM_TYPE_READ), len);
}

void vaddr_write(vaddr_t addr, int len, word_t data) {
  paddr_write(get_real_addr(addr, len, MEM_TYPE_WRITE), len, data);
}
