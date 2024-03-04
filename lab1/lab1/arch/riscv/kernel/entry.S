    .section .text.entry
    .align 2
    .globl _traps 
_traps:
    # YOUR CODE HERE
    # -----------

        # 1. save 32 registers and sepc to stack
        sd x0, -8(x2)
        sd x1, -16(x2)
        sd x2, -24(x2)
        sd x3, -32(x2)
        sd x4, -40(x2)
        sd x5, -48(x2)
        sd x6, -56(x2)
        sd x7, -64(x2)
        sd x8, -72(x2)
        sd x9, -80(x2)
        sd x10, -88(x2)
        sd x11, -96(x2)
        sd x12, -104(x2)
        sd x13, -112(x2)
        sd x14, -120(x2)
        sd x15, -128(x2)
        sd x16, -136(x2)
        sd x17, -144(x2)
        sd x18, -152(x2)
        sd x19, -160(x2)
        sd x20, -168(x2)
        sd x21, -176(x2)
        sd x22, -184(x2)
        sd x23, -192(x2)
        sd x24, -200(x2)
        sd x25, -208(x2)
        sd x26, -216(x2)
        sd x27, -224(x2)
        sd x28, -232(x2)
        sd x29, -240(x2)
        sd x30, -248(x2)
        sd x31, -256(x2)

        csrr t0, sepc
        sd t0, -264(x2)
        addi x2, x2, -264
    # -----------

        # 2. call trap_handler
        csrr a0, scause
        csrr a1, sepc
        call trap_handler 
    # -----------

        # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack
        ld t0, 0(x2)
        csrw sepc, t0
        
        ld x31, 8(x2)
        ld x30, 16(x2)
        ld x29, 24(x2)
        ld x28, 32(x2)
        ld x27, 40(x2)
        ld x26, 48(x2)
        ld x25, 56(x2)
        ld x24, 64(x2)
        ld x23, 72(x2)
        ld x22, 80(x2)
        ld x21, 88(x2)
        ld x20, 96(x2)
        ld x19, 104(x2)
        ld x18, 112(x2)
        ld x17, 120(x2)
        ld x16, 128(x2)
        ld x15, 136(x2)
        ld x14, 144(x2)
        ld x13, 152(x2)
        ld x12, 160(x2)
        ld x11, 168(x2)
        ld x10, 176(x2)
        ld x9, 184(x2)
        ld x8, 192(x2)
        ld x7, 200(x2)
        ld x6, 208(x2)
        ld x5, 216(x2)
        ld x4, 224(x2)
        ld x3, 232(x2)
        ld x1, 248(x2)
        ld x0, 256(x2)
        ld x2, 240(x2)
    # -----------

        # 4. return from trap
        sret
    # -----------