#include "syscall.h"

uint64_t sys_write(unsigned int fd, const char* buf, uint64_t count);
uint64_t sys_read(unsigned int fd, char* buf, uint64_t count);

void sys_call(struct pt_regs *regs, int sys_call_num){
    uint64 func = regs->x[17];

    if(func == 172){
        regs->x[10] = current->pid;
    }
    else if(func == 64){
        sys_write(regs->x[10], (char *)(regs->x[11]), regs->x[12]);
        //fd(a0) = 1
        // if(regs->x[10] == 1){
        //     //a1 = buf, a2 = count
        //     ((char *)(regs->x[11]))[regs->x[12]] = '\0';
        //     //a0 = 输出长度
        //     printk((char *)(regs->x[11])); 
        //     regs->x[10] = regs->x[12];
        // }
    }
    else if(func == 63){
        sys_read(regs->x[10], (char *)(regs->x[11]), regs->x[12]);
    }
    return;
}

uint64_t sys_write(unsigned int fd, const char* buf, uint64_t count) {
    uint64_t ret;
    struct file* target_file = &(current->files[fd]);
    if (target_file->opened) {
        target_file->write(target_file, buf, count);
    } else {
        printk("file not open_write\n");
        ret = ERROR_FILE_NOT_OPEN;
    }
    return ret;
}

uint64_t sys_read(unsigned int fd, char* buf, uint64_t count) {
    int64_t ret;
    struct file* target_file = &(current->files[fd]);
    if (target_file->opened) {
         target_file->read(target_file, buf, count);
    } else {
        printk("file not open_read\n");
        ret = ERROR_FILE_NOT_OPEN;
        while(1);

    }
    return ret;
}