.globl  _start
_start:

#if defined(__ISA_AM_NATIVE__)

  movq $0, %rbp
  // (rsp + 8) should be multiple of 16 when
  // control is transfered to the function entry point.
  // See amd64 ABI manual for more details
  andq $0xfffffffffffffff0, %rsp
  movq %rax, %rdi
  call call_main

#elif defined(__ISA_X86_64__)

  movq $0, %rbp
  movq %rax, %rsp
  movq %rax, %rdi
  call call_main

#elif defined(__ISA_X86__)

  movl $0, %ebp
  call call_main

#elif defined(__ISA_MIPS32__)

  move $fp, $zero
  jal call_main

#elif defined(__riscv)
  mv s0, zero
  mv sp, a3
  addi sp, sp, -24       # 分配 24 字节（3 个 8 字节元素，用于 RV64）
  sd a0, 0(sp)         # args[0] = 0  (argc)
  sd a1, 8(sp)         # args[1] = 0  (argv)
  sd a2, 16(sp)        # args[2] = 0  (envp)
  mv a0, sp              # a0 = &args
  jal call_main

#elif defined(__ISA_LOONGARCH32R__)

  move $fp, $zero
  bl call_main

#else
#error Unsupported ISA
#endif
