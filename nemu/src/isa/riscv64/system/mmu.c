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
#include <memory/vaddr.h>
#include <memory/paddr.h>

typedef uintptr_t PTE;
#define PTE_V 0x01
#define PTE_R     0x002 /* Read */
#define PTE_W     0x004 /* Write */
#define PTE_X     0x008 /* Execute */
paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  uint64_t satp = cpu.csr[CSR_SATP];

  int vpn2 = (vaddr >> 30) & 0x1FF;
  int vpn1 = (vaddr >> 21) & 0x1FF;
  int vpn0 = (vaddr >> 12) & 0x1FF;
  int offset = vaddr & 0xFFF;

  uint64_t root_ppn = satp & ((1ULL << 44) - 1);
  paddr_t root_pa = root_ppn << 12;
  static bool flag = false;
  // if(cpu.pc == 0x40002034) flag = true;
  // else flag = false;
  if(flag) {
    printf("   [pc=" FMT_WORD "]\n", cpu.pc);
    // isa_reg_display();
    printf("isa_mmu_translate [" FMT_WORD ", root_pa = " FMT_WORD ", type = %d] \n", vaddr, (vaddr_t)(root_pa), type);
  }
  // 读取一级 PTE
  uint64_t pte1;
  pte1 = paddr_read(root_pa + vpn2 * 8, sizeof(pte1));
  if(flag) {
    printf("   [pc=" FMT_WORD "]\n", cpu.pc);
    // isa_reg_display();
    printf("isa_mmu_translate [" FMT_WORD ", root_pa = " FMT_WORD ", pte1 = " FMT_WORD ", type = %d] \n", vaddr, (vaddr_t)(root_pa + vpn2 * 8), pte1, type);
  }
  if (!(pte1 & PTE_V) || (pte1 & (PTE_R | PTE_W | PTE_X))) {
      return MEM_RET_FAIL;
  }
  if(flag) printf("isa_mmu_translate [" FMT_WORD ", pte1 = " FMT_WORD "] \n", vaddr, pte1);

  uint64_t l2_ppn = (pte1 >> 10) & ((1ULL << 44) - 1);
  paddr_t l2_pa = l2_ppn << 12;

  // 读取二级 PTE
  uint64_t pte2;
  pte2 = paddr_read(l2_pa + vpn1 * 8, sizeof(pte2));

  // if(flag) printf("isa_mmu_translate [" FMT_WORD ", pte2 = " FMT_WORD "] \n", vaddr, pte2);

  if (!(pte2 & PTE_V) || (pte2 & (PTE_R | PTE_W | PTE_X))) {
      return MEM_RET_FAIL;
  }

  uint64_t l3_ppn = (pte2 >> 10) & ((1ULL << 44) - 1);
  paddr_t l3_pa = l3_ppn << 12;

  // 读取三级 PTE
  uint64_t pte3;
  pte3 = paddr_read(l3_pa + vpn0 * 8, sizeof(pte3));
  // if(flag) printf("isa_mmu_translate [" FMT_WORD ", pte3 = " FMT_WORD "] \n", vaddr, pte3);
  if (!(pte3 & PTE_V)) {
      return MEM_RET_FAIL;
  }

  uint64_t page_ppn = (pte3 >> 10) & ((1ULL << 44) - 1);
  uint64_t paddr = (page_ppn << 12) | offset;
  if(flag) printf("isa_mmu_translate [" FMT_WORD ", paddr = " FMT_WORD "] \n", vaddr, paddr);
  return paddr;
  // // 1. 正确提取 VPN[2], VPN[1], VPN[0]（每个 9 位）
  // int vpn2 = (vaddr >> 30) & 0x1FF;
  // int vpn1 = (vaddr >> 21) & 0x1FF;
  // int vpn0 = (vaddr >> 12) & 0x1FF;
  // int offset = vaddr & 0x1FF;
  // // offset 不使用，因为 map 只建立页映射
  // uint64_t satp_val = cpu.csr[CSR_SATP];
  // uint64_t ppn = satp_val & 0xFFFFFFFFFFFUL;   // 低 44 位掩码
  // PTE* l1 = (PTE*)(ppn << 12);
  // printf("isa_mmu_translate [" FMT_WORD ", " FMT_WORD ", %d] \n", vaddr, (vaddr_t)l1, vpn2);
  // // 2. 一级查找，若无效则分配二级页表
  // if (!(l1[vpn2] & PTE_V)) {
  //     printf("isa_mmu_translate 1111\n");
  //     return MEM_RET_FAIL;
  // }
  // PTE *l2 = (PTE *)((l1[vpn2] >> 10) << 12);   // 从 PPN 恢复虚拟地址

  // // 3. 二级查找，若无效则分配三级页表
  // if (!(l2[vpn1] & PTE_V)) {
  //   printf("isa_mmu_translate 22222\n");
  //     return MEM_RET_FAIL;
  // }
  // PTE *l3 = (PTE *)((l2[vpn1] >> 10) << 12);

  // if (!(l3[vpn0] & PTE_V)) { // 先不处理权限
  //   printf("isa_mmu_translate 33333\n");
  //     return MEM_RET_FAIL;
  // }

  // printf("isa_mmu_translate [" FMT_WORD ", " FMT_WORD "] \n", vaddr, (l3[vpn0] >> 10) + offset);
  // return (l3[vpn0] >> 10) + offset;
}
