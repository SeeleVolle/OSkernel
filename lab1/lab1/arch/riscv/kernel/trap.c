// trap.c 
#include "sbi.h"
#include "printk.h"

void trap_handler(unsigned long scause, unsigned long sepc) {
    // 通过 `scause` 判断trap类型
    // 如果是interrupt 判断是否是timer interrupt
    // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.3.4 节
    // 其他interrupt / exception 可以直接忽略
    
    // YOUR CODE HERE
    // (scause&1UL << 63)&&((scause&0x5)==0x5)
    if((scause&(1ULL<<63)) && (scause&0x5 == 0x5))
    {   
        // printk("Supervisor Mode Timer Interrupt\n");
        clock_set_next_event();
    }
    else
    {
        printk("Other Interrupt\n");
    }
}