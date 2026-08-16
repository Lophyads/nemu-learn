#define concat_temp(x, y) x ## y
#define concat(x, y) concat_temp(x, y)
#define MAP(c, f) c(f)

#if __riscv_xlen == 32
#define LOAD  lw
#define STORE sw
#define XLEN  4
#else
#define LOAD  ld
#define STORE sd
#define XLEN  8
#endif

#define REGS_LO16(f) \
      f( 1)       f( 3) f( 4) f( 5) f( 6) f( 7) f( 8) f( 9) \
f(10) f(11) f(12) f(13) f(14) f(15)
#ifndef __riscv_e
#define REGS_HI16(f) \
                                    f(16) f(17) f(18) f(19) \
f(20) f(21) f(22) f(23) f(24) f(25) f(26) f(27) f(28) f(29) \
f(30) f(31)
#define NR_REGS 32
#else
#define REGS_HI16(f)
#define NR_REGS 16
#endif

#define REGS(f) REGS_LO16(f) REGS_HI16(f)

#define PUSH(n) STORE concat(x, n), (n * XLEN)(sp);
#define POP(n)  LOAD  concat(x, n), (n * XLEN)(sp);

#define CONTEXT_SIZE  ((NR_REGS + 4) * XLEN)
#define OFFSET_SP     ( 2 * XLEN)
#define OFFSET_CAUSE  ((NR_REGS + 0) * XLEN)
#define OFFSET_STATUS ((NR_REGS + 1) * XLEN)
#define OFFSET_EPC    ((NR_REGS + 2) * XLEN)

.align 3
.globl __am_asm_trap
__am_asm_trap:
  # 切换到内核栈：交换 sp 和 mscratch
  # 之后 sp = 内核栈顶，mscratch = 用户栈指针
  # 现在实现是不支持 中断嵌套的，如果在内核状态收到中断需要跳过执行  csrrw sp, mscratch, sp
  csrrw sp, mscratch, sp

  # 在内核栈上分配 Context 空间
  addi sp, sp, -CONTEXT_SIZE

  # 保存除 sp 外的所有通用寄存器（REGS 宏不包含 x2）
  MAP(REGS, PUSH)

  # 保存用户栈指针到 Context.sp
  # 此时 mscratch 中仍是用户栈指针
  csrr t0, mscratch
  STORE t0, OFFSET_SP(sp)

  # 保存 mcause, mstatus, mepc
  csrr t0, mcause
  # csrr t1, mstatus
  csrr t2, mepc
  STORE t0, OFFSET_CAUSE(sp)
  # STORE t1, OFFSET_STATUS(sp)
  STORE t2, OFFSET_EPC(sp)

  # 设置 mstatus.MPRV（用于 difftest）
  # li a0, (1 << 17)
  # or t1, t1, a0
  # csrw mstatus, t1

  # 将 Context* 作为参数传给 C 处理函数
  mv a0, sp
  call __am_irq_handle

  # 从 __am_irq_handle 返回后，a0 是新进程的 Context*
  mv sp, a0                  # 切换到新进程的内核栈

  # 更新 mscratch 为新进程的内核栈顶（Context 之后的位置）
  addi t0, sp, CONTEXT_SIZE
  csrw mscratch, t0

  # 恢复 mstatus, mepc
  # LOAD t1, OFFSET_STATUS(sp)
  LOAD t2, OFFSET_EPC(sp)
  # csrw mstatus, t1
  csrw mepc, t2

  # 恢复除 sp 外的通用寄存器（会覆盖 t0, t1, t2 等，但已经设置好 mscratch 和 csr）
  MAP(REGS, POP)

  LOAD sp, OFFSET_SP(sp)

  # 返回用户态
  mret