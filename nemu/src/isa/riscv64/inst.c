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

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

void write_iringbuf(char* s);
enum {
  TYPE_I, TYPE_U, TYPE_S, TYPE_J, TYPE_B, TYPE_IR,
  TYPE_N, // none
};

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
#define immJ() do { *imm = (SEXT((BITS(i, 31, 31) << 20) | (BITS(i, 19, 12) << 12) | (BITS(i, 20, 20) << 11) | (BITS(i, 30, 21) << 1), 21));  } while(0)
#define immB() do { *imm = (SEXT((BITS(i, 31, 31) << 12) | (BITS(i, 30, 25) << 5) | (BITS(i, 7, 7) << 11) | (BITS(i, 11, 8) << 1), 13));  } while(0)
#define immIR() do { *imm = SEXT(BITS(i, 31, 25), 7); } while(0)

static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);
  switch (type) {
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_J:                   immJ(); break;
    case TYPE_B: src1R(); src2R(); immB(); break;
    case TYPE_IR: src1R(); src2R(); immIR(); break;
    case TYPE_N: break;
    default: panic("unsupported type = %d", type);
  }
}

void jump_trace(uint32_t inst_addr, uint32_t func_addr);

void ret_trace(uint32_t inst_addr);

static void ftrace_jal(uint32_t inst_addr, uint32_t func_addr, uint64_t rd){
    if (rd == 1) {  // rd == ra
        jump_trace(inst_addr, func_addr);
    }
}

static void ftrace_jalr(uint32_t inst_addr, uint32_t func_addr, uint64_t rd, uint64_t rs1, uint64_t imm){
    if (rd == 0 && rs1 == 1 && imm == 0) {
        // ret 指令: jalr x0, ra, 0
        ret_trace(inst_addr);
    } else if (rd == 1) {
        // 调用: rd == ra
        jump_trace(inst_addr, func_addr);
    } else {
        // 尾调用、间接跳转等，ftrace 可不记录
    }
}
static bool debug_flag = false;
#ifdef CONFIG_ITRACE_COND
static char log_buf[128];
#endif
word_t paddr_read(paddr_t addr, int len);
static int decode_exec(Decode *s) {
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  int rd = 0; \
  word_t src1 = 0, src2 = 0, imm = 0; \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}
  INSTPAT_START();
  // printf("inst: %u\n", (s)->isa.inst);
  if (s->pc == 0x4000240c) debug_flag = true;
  // if (debug_flag)
  // Log("decode_exec [pc=" FMT_WORD ", inst=" FMT_WORD "]", s->pc, (word_t)(s)->isa.inst);
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb, I, R(rd) = SEXT(Mr(src1 + imm, 1), 8));
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw    , I, R(rd) = SEXT(Mr(src1 + imm, 4), 32));
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2));
  INSTPAT("??????? ????? ????? 011 ????? 01000 11", sd  ,  S, Mw(src1 + imm, 8, src2));
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi  , I, R(rd) = src1 + imm);
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add  , IR, R(rd) = src1 + src2);
  INSTPAT("??????? ????? ????? 000 ????? 00110 11", addiw  , I, R(rd) = SEXT(src1 + imm, 32));
  INSTPAT("0000000 ????? ????? 000 ????? 01110 11", addw  , IR, R(rd) = SEXT(src1 + src2, 32));
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub  , IR, R(rd) = src1 - src2);
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu , I, R(rd) = src1 < imm);

  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr , I, R(rd) = s->pc + 4, s->dnpc = (src1 + imm) & ~1; ftrace_jalr(s->pc, s->dnpc, rd, BITS(s->isa.inst, 19, 15), imm));
  INSTPAT("??????? ????? ????? 011 ????? 00000 11", ld ,   I,  R(rd) = Mr(src1 + imm, 8));
  INSTPAT("??????? ????? ????? 110 ????? 00000 11", lwu, I, R(rd) = (uint32_t)Mr(src1 + imm, 4));
  INSTPAT("000000? ????? ????? 001 ????? 00100 11", slli,  I, R(rd) = src1 << (imm & 0x3f));
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll, IR, R(rd) = src1 << (src2 & 0x3f););
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal,  J, R(rd) = s->pc + 4; s->dnpc = s->pc + imm; ftrace_jal(s->pc, s->dnpc, rd));


  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne,  B, if(src1 != src2) s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq,  B, if(src1 == src2) s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh,  S, Mw(src1 + imm, 2, src2));
  INSTPAT("010000? ????? ????? 101 ????? 00100 11", srai, I, R(rd) = (word_t)((sword_t)src1 >> (imm & 0x3f)));
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi, I, R(rd) = src1 & imm);
  INSTPAT("0000000 ????? ????? 001 ????? 01110 11", sllw, IR, R(rd) = SEXT(src1 << (src2 & 0x1F), 32));
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and, IR, R(rd) = src1 & src2);
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu, IR, R(rd) = src1 < src2);
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori, I, R(rd) = src1 ^ imm);
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori, I, R(rd) = src1 | imm);
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or, IR, R(rd) = src1 | src2);
  INSTPAT("0000000 ????? ????? 101 ????? 00100 11", srli, I, R(rd) = src1 >> imm);
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge, B, if((sword_t)src1 >= (sword_t)src2) s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw,  S, Mw(src1 + imm, 4, src2));
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui, U, R(rd) = imm);
  INSTPAT("0000000 ????? ????? 101 ????? 00110 11", srliw, I, R(rd) = SEXT((uint32_t)src1 >> imm, 32));
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor, IR, R(rd) = src1 ^ src2);
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu, B, if(src1 >= src2) s->dnpc = s->pc + imm);
  INSTPAT("0000001 ????? ????? 000 ????? 01110 11", mulw, IR, R(rd) = SEXT(src1 * src2, 32));
  INSTPAT("0000001 ????? ????? 100 ????? 01110 11", divw, IR, {
      int32_t a = (int32_t)src1, b = (int32_t)src2;
      R(rd) = SEXT((b == 0) ? a : ((a == INT32_MIN && b == -1) ? 0 : (a / b)), 32);
  });
  INSTPAT("0000001 ????? ????? 110 ????? 01110 11", remw, IR, {
      int32_t a = (int32_t)src1, b = (int32_t)src2;
      R(rd) = SEXT((b == 0) ? a : ((a == INT32_MIN && b == -1) ? 0 : (a % b)), 32);
  });
  // INSTPAT("??????? ????? 00000 100 ????? 11000 11", bltz, B, if ((sword_t)src1 < 0) s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt, B, if((sword_t)src1 < (sword_t)src2) s->dnpc = s->pc + imm);
  
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt, IR, R(rd) = ((sword_t)src1 < (sword_t)src2));
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh, I, R(rd) =  SEXT(Mr(src1 + imm, 2), 16));
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu, I, R(rd) = (uint16_t)Mr(src1 + imm, 2));
  INSTPAT("0100000 ????? ????? 000 ????? 01110 11", subw, IR, R(rd) = SEXT(src1 - src2, 32));
  INSTPAT("000000? ????? ????? 101 ????? 00100 11", srli, I, R(rd) = src1 >> imm);
  INSTPAT("0000000 ????? ????? 001 ????? 00110 11", slliw, I, R(rd) = SEXT(src1 << imm, 32));
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul, IR, R(rd) = src1 * src2);
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem, IR, {
      sword_t a = (sword_t)src1, b = (sword_t)src2;
      R(rd) = (b == 0) ? a : (a % b);
  });

  
  INSTPAT("010000? ????? ????? 101 ????? 00110 11", sraiw, I, R(rd) = SEXT((int32_t)src1 >> imm,32));
  INSTPAT("0100000 ????? ????? 101 ????? 01110 11", sraw, IR, R(rd) = SEXT((int32_t)src1 >> (src2 & 0x1F), 32));
  INSTPAT("0000000 ????? ????? 101 ????? 01110 11", srlw, IR, R(rd) = SEXT((uint32_t)src1 >> (src2 & 0x1F), 32));
  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu, B, if(src1 < src2) s->dnpc = s->pc + imm);
  INSTPAT("0000001 ????? ????? 111 ????? 01110 11", remuw, IR, R(rd) = SEXT((uint32_t)src1 % (uint32_t)src2, 32));
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu, IR, R(rd) = (src2 == 0) ? src1 : (uint64_t)src1 % (uint64_t)src2);
  INSTPAT("0000001 ????? ????? 101 ????? 01110 11", divuw, IR, R(rd) = SEXT((uint32_t)src1 / (uint32_t)src2, 32));
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu, IR, R(rd) = (src2 == 0) ? ~0ULL : (uint64_t)src1 / (uint64_t)src2);
  INSTPAT("??????? ????? ????? 001 00000 11100 11", csrw, I, cpu.csr[imm & 0xfff] = src1;
      IFDEF(CONFIG_ETRACE, Log("csrw [" FMT_WORD ", " FMT_WORD "]", imm & 0xfff, src1));
      // if((imm & 0xfff) == CSR_SATP){
      //   printf("csrw [pc=" FMT_WORD ", " FMT_WORD ", val=" FMT_WORD "]\n", s->pc, imm & 0xfff, src1);
      // }
  );
  INSTPAT("0000000 00000 00000 000 00000 11100 11", ecall, I, 
    IFDEF(CONFIG_ETRACE, Log("ecall [syscall=%ld, pc=" FMT_WORD ", dnpc=" FMT_WORD " last_inst=" FMT_WORD "]",
                              R(17), s->pc, s->dnpc, paddr_read(s->pc - 4, 4)));
    isa_raise_intr(R(17) == -1 ? -1 : 0, s->pc); s->dnpc = cpu.pc;
    IFDEF(CONFIG_ETRACE, Log("ecall [syscall=%ld, pc=" FMT_WORD ", dnpc=" FMT_WORD " dnpc_inst=" FMT_WORD "]",
                              R(17), s->pc, s->dnpc, paddr_read(s->dnpc, 4)));
    // Log("ecall [syscall=%ld, pc=" FMT_WORD ", dnpc=" FMT_WORD ", s0=" FMT_WORD "]\n",R(17), s->pc, s->dnpc, R(2));
    // if(R(17) == 13) debug_flag = true;
    if(debug_flag) {
      // printf("ecall [syscall=%ld, pc=" FMT_WORD ", dnpc=" FMT_WORD "]\n",R(17), s->pc, s->dnpc);
      // isa_reg_display();
    }
    // isa_reg_display();
    // if (s->pc == 0x400092f8){
    //   debug_flag = true; 
    // }
    
  );
  INSTPAT("??????? ????? 00000 010 ????? 11100 11", csrr, I, R(rd) = cpu.csr[imm & 0xfff]);
  INSTPAT("0011000 00010 00000 000 00000 11100 11", mret, I, 
    s->dnpc = cpu.csr[CSR_MEPC];
    IFDEF(CONFIG_ETRACE, Log("mret [" FMT_WORD ", " FMT_WORD ", " FMT_WORD "]", s->pc, s->dnpc, paddr_read(s->dnpc, 4)));
    word_t mstatus = cpu.csr[CSR_MSTATUS];
    if (mstatus & MSTATUS_MPIE_MASK)
        mstatus |= MSTATUS_MIE_MASK;   // MPIE=1，MIE 置 1
    else
        mstatus &= ~MSTATUS_MIE_MASK;  // MPIE=0，MIE 清 0
    mstatus |= MSTATUS_MPIE_MASK;
    cpu.csr[CSR_MSTATUS] = mstatus;


    // Log("mret [" FMT_WORD ", " FMT_WORD "]", s->pc, s->dnpc);
    // Log("mret [" FMT_WORD ", " FMT_WORD ", s0=" FMT_WORD "]", s->pc, s->dnpc, R(2));
    if (debug_flag)
    {
      // printf("mret [" FMT_WORD ", " FMT_WORD "]\n", s->pc, s->dnpc);
      // isa_reg_display();
    }
  );

  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div, IR,
    R(rd) = (src2 == 0 || ((int64_t)src1 == (int64_t)0x8000000000000000 && (int64_t)src2 == -1))
            ? ~0ULL
            : (uint64_t)((int64_t)src1 / (int64_t)src2));
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl, IR,
      R(rd) = (uint64_t)src1 >> (src2 & 0x3F);  // 64 位移位，结果零扩展
  );
  INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti, I,
    R(rd) = (int64_t)src1 < (int64_t)imm ? 1 : 0;
  );

  INSTPAT("??????? ????? ????? 001 ????? 11100 11", csrrw, N,
        word_t csr_addr = BITS(s->isa.inst, 31, 20);
        word_t rs1_val  = R(BITS(s->isa.inst, 19, 15));

    if (rd != 0) {
        R(rd) = cpu.csr[csr_addr];
    }
    cpu.csr[csr_addr] = rs1_val;
  );

  INSTPAT("??????? ????? ????? 110 ????? 11100 11", csrsi, N,
      word_t csr  = BITS(s->isa.inst, 31, 20);   // 12 位 CSR 地址
      word_t zimm = BITS(s->isa.inst, 19, 15);   // 5 位无符号立即数（位索引）

      if (rd != 0) {
        R(rd) = cpu.csr[csr];      // 读旧 CSR 值到 rd
      }
      cpu.csr[csr] |= (1 << zimm); // CSR |= (1 << zimm)
  );
  
  // ----------------- end --------------------
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv, N, 
    Log("invaild inst[pc=" FMT_WORD ", inst=" FMT_WORD "]", s->pc, (word_t)(s)->isa.inst);
    INV(s->pc)
  );
  INSTPAT_END();
//   if(s->pc == 0x40002360
//   || s->pc == 0x40002344
//   || s->pc == 0x4000235c
//   || s->pc == 0x40002350
//   || s->pc == 0x40002354
//   || s->pc == 0x40002358
// || s->pc == 0x40004b20
// || s->pc == 0x40004ea0
// || s->pc == 0x40004ea4
// || s->pc == 0x40004b50
// || s->pc == 0x40004b28
// || s->pc == 0x40004b44) {
//     // isa_reg_display();
//     printf("[pc=" FMT_WORD ", inst=" FMT_WORD ", s0=" FMT_WORD ", sp=" FMT_WORD ", a5=" FMT_WORD "]\n", s->pc, (word_t)(s)->isa.inst, R(8), R(2), R(15));
//   }

  // if(s->pc == 0x40038170
  // || s->pc == 0x4003816c
  // || s->pc == 0x40038174
  // || s->pc == 0x80001540
//   || s->pc == 0x4000235c
//   || s->pc == 0x40002350
//   || s->pc == 0x40002354
//   || s->pc == 0x40002358
// || s->pc == 0x40004b20
// || s->pc == 0x40004ea0
// || s->pc == 0x40004ea4
// || s->pc == 0x40004b50
// || s->pc == 0x40004b28
// ) {
    // isa_reg_display();
    // printf("[pc=" FMT_WORD ", inst=" FMT_WORD ", ra=" FMT_WORD ", sp=" FMT_WORD ", t1=" FMT_WORD "]\n", s->pc, (word_t)(s)->isa.inst, R(1), R(2), R(6));
  // }
  // if (debug_flag){
  //   if (s->pc == 0x800014c8
  //   || s->pc == 0x80000c10
  //   || s->pc == 0x80000c58
  //   || s->pc == 0x80000c6c
  //   || s->pc == 0x8000025c
  //   || s->pc == 0x80000c70
  //   || s->pc == 0x80001540
  //   || s->pc == 0x80001550
  //   || s->pc == 0x800017b4
  //   || s->pc == 0x800017b8
  // ){
  //     printf("[pc=" FMT_WORD ", inst=" FMT_WORD ", ra=" FMT_WORD ", sp=" FMT_WORD ", val=" FMT_WORD "]\n", s->pc, (word_t)(s)->isa.inst, R(1), R(2), vaddr_read(0x7FFFF5e0, 8));
  //   }
  // }
  
  #ifdef CONFIG_ITRACE_COND
    sprintf(log_buf, "   [pc=" FMT_WORD ", inst=" FMT_WORD ", a7=" FMT_WORD ", sp = " FMT_WORD "]", s->pc, (word_t)(s)->isa.inst, R(17), R(2));
    write_iringbuf(log_buf);
  #endif
  R(0) = 0; // reset $zero to 0
  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}