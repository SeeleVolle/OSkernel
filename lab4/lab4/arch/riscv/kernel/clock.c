// clock.c
#include "types.h"
#include "printk.h"
#include "sbi.h"
// QEMU中时钟的频率是10MHz, 也就是1秒钟相当于10000000个时钟周期。
unsigned long TIMECLOCK = 10000000;

unsigned long get_cycles() {
    // 编写内联汇编，使用 rdtime 获取 time 寄存器中 (也就是mtime 寄存器 )的值并返回
    // YOUR CODE HERE
    unsigned long r_time = 0;
    __asm__ volatile(
        "rdtime t0\n"
        "mv %[ret_val], t0"
        : [ret_val]"=r"(r_time)
        : 
        : "memory"
    );
    return r_time;
}

void clock_set_next_event() {
    // 下一次 时钟中断 的时间点
    unsigned long next = get_cycles() + TIMECLOCK;
    // printk("Kernel is running\n");
    // printk("[S] Supervisor Mode Timer Interrupt\n");
    // 使用 sbi_ecall 来完成对下一次时钟中断的设置
    // YOUR CODE HERE
    sbi_ecall(0x0, 0x0, next, 0, 0, 0, 0, 0);
} 
