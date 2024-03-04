#include "defs.h"

    .section .text.entry
    .align 2
    .globl _traps
    .globl __dummy
    .globl __switch_to
_traps:
    # ----------
        #判断为内核线程 / 用户线程
        csrr t1, sscratch
        beq t1, x0, _trap_core

        csrw sscratch,sp
        add sp, t1, x0
        
_trap_core:
    # -----------
        # 1. save 32 registers and sepc to stack
        sd x31, -8(x2)
        sd x30, -16(x2)
        sd x29, -24(x2)
        sd x28, -32(x2)
        sd x27, -40(x2)
        sd x26, -48(x2)
        sd x25, -56(x2)
        sd x24, -64(x2)
        sd x23, -72(x2)
        sd x22, -80(x2)
        sd x21, -88(x2)
        sd x20, -96(x2)
        sd x19, -104(x2)
        sd x18, -112(x2)
        sd x17, -120(x2)
        sd x16, -128(x2)
        sd x15, -136(x2)
        sd x14, -144(x2)
        sd x13, -152(x2)
        sd x12, -160(x2)
        sd x11, -168(x2)
        sd x10, -176(x2)
        sd x9, -184(x2)
        sd x8, -192(x2)
        sd x7, -200(x2)
        sd x6, -208(x2)
        sd x5, -216(x2)
        sd x4, -224(x2)
        sd x3, -232(x2)
        sd x2, -240(x2)
        sd x1, -248(x2)
        sd x0,  -256(x2)

        csrr t0, sepc
        sd t0, -264(x2)
        csrr t0, sstatus
        sd t0, -272(x2)
        csrr t0, stval
        sd t0, -280(x2)

        addi x2, x2, -280
    # -----------
        # 2. call trap_handler
        csrr a0, scause
        csrr a1, sepc
        add a2, sp, x0  #sp ->a2
        call trap_handler 
    # -----------
        # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack

        ld t0, 16(x2)
        addi t0, t0, 4 # 手动修改 sepc 的地址，使得 sret 之后 程序继续执行
        csrw sepc, t0
        
        ld  x0,  24(x2)    
        ld  x1,  32(x2)   
        ld  x3,  48(x2)   
        ld  x4,  56(x2)   
        ld  x5,  64(x2)   
        ld  x6,  72(x2)   
        ld  x7,  80(x2)   
        ld  x8,  88(x2)   
        ld  x9,  96(x2)   
        ld x10,  104(x2)   
        ld x11, 112(x2)   
        ld x12, 120(x2)   
        ld x13, 128(x2)   
        ld x14, 136(x2)   
        ld x15, 144(x2)   
        ld x16, 152(x2)   
        ld x17, 160(x2)   
        ld x18, 168(x2)   
        ld x19, 176(x2)   
        ld x20, 184(x2)   
        ld x21, 192(x2)   
        ld x22, 200(x2)   
        ld x23, 208(x2)    
        ld x24, 216(x2)   
        ld x25, 224(x2)   
        ld x26, 232(x2)   
        ld x27, 240(x2)   
        ld x28, 248(x2)   
        ld x29, 256(x2)   
        ld x30, 264(x2)   
        ld x31, 272(x2)
        ld  x2,  40(x2)

        csrr t1, sscratch
        beq t1, x0, _trap_end
        csrw sscratch,sp
        add sp, t1, x0
    # -----------

_trap_end:
        # 4. return from trap
        sret
    # -----------

__switch_to:
    # task_struct = thread_info + state + counter + priority + pid + thread_info
    # thread initial address: tast_struck + 4 * 8 + 2 * 8 = prev + 48

    # save state to prev proces
    sd ra, 48(a0)
    sd sp, 56(a0)
    sd s0, 64(a0)
    sd s1, 72(a0)
    sd s2, 80(a0)
    sd s3, 88(a0)
    sd s4, 96(a0)
    sd s5, 104(a0)
    sd s6, 112(a0)
    sd s7, 120(a0)
    sd s8, 128(a0)
    sd s9, 136(a0)
    sd s10, 144(a0)
    sd s11, 152(a0)

    # save state of sepc, sstatus, sscratch
    csrr t1, sepc
    sd t1, 160(a0)
    csrr t1, sstatus
    sd t1, 168(a0)
    csrr t1, sscratch
    sd t1, 176(a0)

    # restore state from next process
    ld ra, 48(a1)
    ld sp, 56(a1)
    ld s0, 64(a1)
    ld s1, 72(a1)
    ld s2, 80(a1)
    ld s3, 88(a1)
    ld s4, 96(a1)
    ld s5, 104(a1)
    ld s6, 112(a1)
    ld s7, 120(a1)
    ld s8, 128(a1)
    ld s9, 136(a1)
    ld s10, 144(a1)
    ld s11, 152(a1)

    
    ld t1, 160(a1)
    csrw sepc, t1
    ld t1, 168(a1)
    csrw sstatus, t1
    ld t1, 176(a1)
    csrw sscratch, t1

    #切换页表
    li t5, PA2VA_OFFSET
    li t0, 8
    slli t0, t0, 60 # Mode = 8
    ld t1, 184(a1)
    sub t1, t1, t5
    srli t1, t1, 12
    or t0, t0, t1

    csrw satp, t0   
    sfence.vma zero, zero
    fence.i
    ret

__dummy:
    # la t0, dummy # 将 dummy() 的地址加载到寄存器 t0 中
    # csrw sepc, t0 # 将 dummy() 的地址写入寄存器 sepc

    # 交换 u-mode stack和 s-mode stack
    csrr t1, sscratch
    csrw sscratch, sp
    add sp, t1, x0

    sret