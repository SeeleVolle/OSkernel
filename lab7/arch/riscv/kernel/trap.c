// trap.c 
#include "sbi.h"
#include "proc.h"
#include "printk.h"
#include "syscall.h"

void trap_handler(uint64 scause, uint64 sepc, struct pt_regs *regs) {
    // 通过 `scause` 判断trap类型
    // 如果是interrupt 判断是否是timer interrupt
    // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.3.4 节
    // 其他interrupt / exception 可以直接忽略
    
    // YOUR CODE HERE
    // (scause&1UL << 63)&&((scause&0x5)==0x5)
    // printk("interrupt : %d \n", (scause&(1ULL<<63)));
    // printk("exception code: %d \n", (scause));

    if((scause&1UL << 63)&&((scause&0x5)==0x5))
    {   
        // printk("[S] Supervisor Mode Timer Interrupt\n");
        clock_set_next_event();
        do_timer();
    }
    //Interrupt =1 && exception code == 8
    else if((scause&0x8) == 8)
    {
        uint64 sys_call_num = regs->x[17];
        if(sys_call_num == 172 || sys_call_num == 64 || sys_call_num == 63)
            sys_call(regs, sys_call_num);
        else{
            printk("[S] Unhandled syscall: %lx", sys_call_num);
            while (1);
        }
    }else{
        printk("[S] Unhandled trap, ");
        printk("scause=%lx__",scause);
        printk("stval: %lx, ", regs->stval);
        printk("sepc: %lx\n", sepc);
        while(1);
    }
    return;
}