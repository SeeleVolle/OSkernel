#include "defs.h"

.extern early_pgtbl
.extern start_kernel

    .section .text.init
    .globl _start
_start:
    li t0, PA2VA_OFFSET
	la sp, boot_stack_top
    sub sp, sp, t0

    call setup_vm
    call relocate

    #---

    call mm_init  #初始化内存管理
    call setup_vm_final  
    
    #---

    call task_init
    # set stvec
    la t2, _traps
    csrw stvec, t2 

    #---

    # set sie[STIE] = 1
    li t5, 0x20
    csrr t3, sie
    or t3, t3, t5
    csrw sie, t3

    #---

    # set interrupt
    call clock_set_next_event

    #----

    # set sstatus[SIE] = 1
    # 使schedule不受中断影响
    #li t6, 0x2
    #csrr t3, sstatus
    #or t3, t3, t6
    #csrw sstatus, t3

    #---

	jal start_kernel

relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)
    li t5, PA2VA_OFFSET
    add ra, ra, t5
    add sp, sp, t5

    # set satp with early_pgtbl

    li t0, 8
    slli t0, t0, 60 # Mode = 8
    la t1, early_pgtbl
    sub t1, t1, t5
    srli t1, t1, 12
    or t0, t0, t1

    csrw satp, t0    

    # flush tlb
    sfence.vma zero, zero
		
    # flush icache
    fence.i
    
    ret
    
    #---
    .section .bss.stack
    .globl boot_stack


boot_stack:
    .space 4096 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top:
