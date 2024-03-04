#ifndef _SYSCALL_H
#define _SYSCALL_H
#define SYS_WRITE   64
#define SYS_GETPID  172

#include "types.h"
#include "proc.h"

extern struct task_struct* current; 

struct pt_regs {
    uint64 stval;
    uint64 sstatus;
    uint64 sepc;
    uint64 x[32];
};

void sys_call(struct pt_regs* regs, int sys_call_num);
#endif