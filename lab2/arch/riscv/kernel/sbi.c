#include "types.h"
#include "sbi.h"
#include "printk.h"


struct sbiret sbi_ecall(int ext, int fid, uint64 arg0,
			            uint64 arg1, uint64 arg2,
			            uint64 arg3, uint64 arg4,
			            uint64 arg5) 
{
    // unimplemented  
	struct sbiret ret;     
	__asm__ volatile(
		"mv a7, %[ext]\n"
        "mv a6, %[fid]\n"
		"mv a5, %[arg5]\n"
		"mv a4, %[arg4]\n"
		"mv a3, %[arg3]\n"
		"mv a2, %[arg2]\n"
		"mv a1, %[arg1]\n"
		"mv a0, %[arg0]\n"
		"ecall\n"
        "mv %[ret_val], a1\n"
        "mv %[err_code], a0"
        : [ret_val] "=r" (ret.value), [err_code] "=r" (ret.error)
        : [ext] "r" (ext), [fid] "r" (fid), 
		  [arg1] "r" (arg1), [arg2] "r" (arg2), 
		  [arg3] "r" (arg3), [arg4] "r" (arg4), 
		  [arg5] "r" (arg5), [arg0] "r" (arg0)
			
        : "memory"
	); 
	//printk("sbi_ecall\n");
	return ret;
}

void sbi_set_timer(uint64 timer){
	// printk("set_timer\n");
	int a;
	a=1;
	sbi_ecall(0x0,0x0,timer,0,0,0,0,0);
	return;
}

// void sbi_console_putchar(void){
// 	sbi_ecall(0x1,0x0,0,0,0,0,0,0);
// }
