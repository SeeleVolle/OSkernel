    .section .text.entry
    .align 2
    .globl _traps 
    .globl __dummy
    .globl __switch_to
_traps:
    # YOUR CODE HERE
    # -----------

        # 1. save 32 registers and sepc to stack

        sd x1 ,   -8(x2)
        sd x2 ,  -16(x2)
        sd x3 ,  -24(x2)
        sd x4 ,  -32(x2)
        sd x5 ,  -40(x2)
        sd x6 ,  -48(x2)
        sd x7 ,  -56(x2)
        sd x8 ,  -64(x2)
        sd x9 ,  -72(x2)
        sd x10,  -80(x2)
        sd x11,  -88(x2)
        sd x12,  -96(x2)
        sd x13, -104(x2)
        sd x14, -112(x2)
        sd x15, -120(x2)
        sd x16, -128(x2)
        sd x17, -136(x2)
        sd x18, -144(x2)
        sd x19, -152(x2)
        sd x20, -160(x2)
        sd x21, -168(x2)
        sd x22, -176(x2)
        sd x23, -184(x2)
        sd x24, -192(x2)
        sd x25, -200(x2)
        sd x26, -208(x2)
        sd x27, -216(x2)
        sd x28, -224(x2)
        sd x29, -232(x2)
        sd x30, -240(x2)
        sd x31, -248(x2)

        CSRR x5,sepc
        sd x5, -256(x2)
        addi x2,x2,-256

    # -----------

        # 2. call trap_handler
        CSRR a0,scause
        CSRR a1,sepc
        call trap_handler
    # -----------

        # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack
        ld x5,0(x2)
        CSRW sepc,x5

        ld x31,  8(x2)    
        ld x30,  16(x2)   
        ld x29,  24(x2)   
        ld x28,  32(x2)   
        ld x27,  40(x2)   
        ld x26,  48(x2)   
        ld x25,  56(x2)   
        ld x24,  64(x2)   
        ld x23,  72(x2)   
        ld x22,  80(x2)   
        ld x21,  88(x2)   
        ld x20,  96(x2)   
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
        ld x9,  184(x2)    
        ld x8,  192(x2)   
        ld x7,  200(x2)   
        ld x6,  208(x2)   
        ld x5,  216(x2)   
        ld x4,  224(x2)   
        ld x3,  232(x2)   
        ld x1,  248(x2)   
        ld x2,  240(x2)

        
    # -----------

        # 4. return from trap
        sret

    # -----------

__dummy:
    la t0,dummy
    CSRW sepc,t0
    sret

__switch_to:
    addi a0,a0,48
    sd ra ,   0(a0)
    sd sp ,   8(a0)
    sd s0 ,  16(a0)
    sd s1 ,  24(a0)
    sd s2 ,  32(a0)
    sd s3 ,  40(a0)
    sd s4 ,  48(a0)
    sd s5 ,  56(a0)
    sd s6 ,  64(a0)
    sd s7 ,  72(a0)
    sd s8 ,  80(a0)
    sd s9 ,  88(a0)
    sd s10,  96(a0)
    sd s11, 104(a0)

    addi a1,a1,48
    ld ra ,   0(a1)
    ld sp ,   8(a1)
    ld s0 ,  16(a1)
    ld s1 ,  24(a1)
    ld s2 ,  32(a1)
    ld s3 ,  40(a1)
    ld s4 ,  48(a1)
    ld s5 ,  56(a1)
    ld s6 ,  64(a1)
    ld s7 ,  72(a1)
    ld s8 ,  80(a1)
    ld s9 ,  88(a1)
    ld s10,  96(a1)
    ld s11, 104(a1)

    ret