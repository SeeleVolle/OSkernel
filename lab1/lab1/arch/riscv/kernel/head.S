.extern start_kernel

    .section .text.init
    .globl _start
_start:
	la sp, boot_stack_top
    #---
    # set stvec
    la t2, _traps
    csrw stvec, t2 
    #---
    #set sie
    li t5, 0x20
    csrr t3, sie
    or t3, t3, t5
    csrw sie, t3
    #---
    # set interrupt
    call clock_set_next_event
    #----
    # set sstatus
    li t6, 0x2
    csrr t3, sstatus
    or t3, t3, t6
    csrw sstatus, t3
    #---
	jal start_kernel
    #---
    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top:
