.extern start_kernel
.extern _traps
    .section .text.init
    .globl _start
_start:
    # YOUR CODE HERE
        la sp, boot_stack_top
    # ------------------
        call task_init
        # set stvec = _traps
        la x5,_traps
        CSRW stvec,x5
    # ------------------
    
        # set sie[STIE] = 1
        lui x5,0x00000
        addi x5,x5,32
        CSRRS x0,sie,x5
    # ------------------
        
        # set first time interrupt
        # lui x5,0x989
        # addi x5,x5,0x680 #x5=10,000,000
        # rdtime a0
        # add a0,a0,x5
        # sd a0,-8(sp)
        # addi sp,sp,-8
        # call sbi_set_timer
        call clock_set_next_event

        #sbi_set_timer
    # ------------------
    
        # set sstatus[SIE] = 1
        CSRRSI x0,sstatus,2

    #test for read and write
        # call read_sstatus

        # call write_sscratch
    #-------------------
    # ------------------
        
        call start_kernel
    # ------------------
    # - your lab1 code -
    # ------------------
        
    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top: