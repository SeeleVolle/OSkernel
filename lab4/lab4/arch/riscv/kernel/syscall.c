#include "syscall.h"

void sys_call(struct pt_regs *regs, int sys_call_num){
    uint64 func = regs->x[17];

    if(func == 172){
        regs->x[10] = current->pid;
    }
    else if(func == 64){
        //fd(a0) = 1
        if(regs->x[10] == 1){
            //a1 = buf, a2 = count
            ((char *)(regs->x[11]))[regs->x[12]] = '\0';
            //a0 = 输出长度
            printk((char *)(regs->x[11])); 
            regs->x[10] = regs->x[12];
        }
    }
    return;
}