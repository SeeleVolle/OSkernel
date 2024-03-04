#include "printk.h"
#include "sbi.h"
#include "proc.h"
#include "defs.h"

extern void test();

int start_kernel() {
    printk("2022");
    printk(" Hello RISC-V\n");
    // // sbi_ecall(0x1, 0x0, 0x41, 0, 0, 0, 0, 0);
    // char p[65] = "0000000000000000000000000000000000000000000000000000000000000000";
    // // unsigned long long a = csr_read(sscratch);
    // unsigned long long num = 0x88880000;
    // csr_write(sscratch, num);
    // unsigned long long a = csr_read(sscratch);
    // int i = 0;
    // while(a != 0)
    // {
    //     p[i++] = a % 2 + '0';
    //     // putc(p[i-1]);
    //     a /= 2;
    // }
    // //From 0 - i-1
    // for(int j = 0; j <= (i-1)/2; j++)
    // {
    //     int temp = p[j];
    //     p[j] = p[i-1-j];
    //     p[i-1-j]= temp;
    // } 
    // printk(p);
    // printk("\n");
    schedule();
    test(); // DO NOT DELETE !!!

	return 0;
}
