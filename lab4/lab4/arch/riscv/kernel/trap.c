// trap.c 
#include "sbi.h"
#include "proc.h"
#include "printk.h"
#include "syscall.h"

void trap_handler(uint64 scause, uint64 sepc, struct pt_regs *regs) {
    
    if((scause&1UL << 63)&&((scause&0x5)==0x5))
    {   
        // printk("[S] Supervisor Mode Timer Interrupt\n");
        clock_set_next_event();
        do_timer();
    }
    //Interrupt = 0 && exception code == 8
    else if((scause&1UL << 63) == 0 && (scause&0x8) == 8)
    {
        uint64 sys_call_num = regs->x[17];
        if(sys_call_num == 172 || sys_call_num == 64)
            sys_call(regs, sys_call_num);
        else{
             printk("[S] Unhandled syscall: %lx", sys_call_num);
            while (1);
        }
    }
    // else if((scause&1UL << 63) == 0 && )
    else{
        printk("[S] Unhandled trap, ");
        printk("scause=%lx__",scause);
        printk("stval: %lx, ", regs->stval);
        printk("sepc: %lx\n", sepc);
        while(1);
    }
    return;
}