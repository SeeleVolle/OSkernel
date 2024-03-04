# 浙江大学操作系统实验报告

**实验名称**：Lab 1: RV64 内核引导 与 时钟中断处理

**电子邮件地址**：jjhuang535@outlook.com

**手机**：18972039196

**实验地点**：曹西503

**实验日期**：2023.10.3

**学号**：3210105944 

**姓名**：黄锦骏



### 一、实验目的

* 学习 RISC-V 汇编， 编写 head.S 实现跳转到内核运行的第一个 C 函数。
* 学习 OpenSBI，理解 OpenSBI 在实验中所起到的作用，并调用 OpenSBI 提供的接口完成字符的输出。
* 学习 Makefile 相关知识， 补充项目中的 Makefile 文件， 来完成对整个工程的管理。
* 学习 RISC-V 的 trap 处理相关寄存器与指令，完成对 trap 处理的初始化。
* 理解 CPU 上下文切换机制，并正确实现上下文切换功能。
* 编写 trap 处理函数，完成对特定 trap 的处理。
* 调用 OpenSBI 提供的接口，完成对时钟中断事件的设置。

### 二、实验过程

#### 2.1 编写Head.S

将栈顶指针指向栈，并且跳转到start_kernel

```assembly
.extern start_kernel

    .section .text.entry
    .globl _start
_start:
	la sp, boot_stack_top
	jal start_kernel
    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top:

```

#### 2.2 编写Makefile

​	通过阅读工程中的 Makefile 文件，我补全了lib/Makefile文件，该文件内容如下：

```makefile
C_SRC       = $(sort $(wildcard *.c))
OBJ         = $(patsubst %.c,%.o,$(C_SRC))

all:$(OBJ)

%.o:%.c
	${GCC}  ${CFLAG} -c $<

clean:
    $(shell rm *.o 2>/dev/null)

```

其中，我们定义了变量C_SRC与OBJ，分别指代路径下所有.c文件与.o文件，以便对所有源文件进行编译，清除所有构建产物。

#### 2.3 编写sbi.c

sbi_ecall 函数中，需要完成以下内容：

1. 将 ext (Extension ID) 放入寄存器 a7 中，fid (Function ID) 放入寄存器 a6 中，将 arg0 ~ arg5 放入寄存器 a0 ~ a5 中。
2. 使用 `ecall` 指令。`ecall` 之后系统会进入 M 模式，之后 OpenSBI 会完成相关操作。
3. OpenSBI 的返回结果会存放在寄存器 a0 ， a1 中，其中 a0 为 error code， a1 为返回值， 我们用 sbiret 来接受这两个返回

编写完成的sbi.c文件如下：

```c
#include "types.h"
#include "sbi.h"


struct sbiret sbi_ecall(int ext, int fid, uint64 arg0,
			            uint64 arg1, uint64 arg2,
			            uint64 arg3, uint64 arg4,
			            uint64 arg5) 
{
	struct sbiret ret;
	uint64 error, value;
	__asm__ volatile (
		"mv a0, %[arg0]\n"
		"mv a1, %[arg1]\n"
		"mv a2, %[arg2]\n"
		"mv a3, %[arg3]\n"
		"mv a4, %[arg4]\n"
		"mv a5, %[arg5]\n"
		"mv a6, %[fid]\n"
		"mv a7, %[ext]\n"
		"ecall\n"
		"mv %[ret_val], a0\n"
		"mv %[err_code], a1"
		:[ret_val]"=r"(value), [err_code]"=r"(error) 
		:[arg0]"r"(arg0), [arg1]"r"(arg1), [arg2]"r"(arg2),[arg3]"r"(arg3), [arg4]"r"(arg4), [arg5]"r"(arg5), [fid] "r"(fid), [ext] "r" (ext)
		:"memory"
	);
	ret.error = error;
	ret.value = value;
	return ret;
}

```

直接在汇编函数中完成所有的内存移动，并最终将结果保存到Ret中返回即可

#### 2.4 修改defs.h

按照示例编写了defs.h，两个宏的作用是读/写控制状态寄存器

```c++
#ifndef _DEFS_H
#define _DEFS_H

#include "types.h"

#define csr_read(csr)                       \
({                                          \
    register uint64 __v;                    \
	asm volatile("csrr __, " #csr          \
					:"=r"(__V):             \
					: "memory")             \
    __v;                                    \
})

#define csr_write(csr, val)                         \
({                                                  \
    uint64 __v = (uint64)(val);                     \
    asm volatile ("csrw " #csr ", %0"               \
                    : : "r" (__v)                   \
                    : "memory");                    \
})

#endif

```

#### 2.5 开启Trap处理

1. 设置 `stvec`， 将 `_traps` ( `_trap` 在 4.3 中实现 ) 所表示的地址写入 `stvec`，这里我们采用 `Direct 模式`, 而 `_traps` 则是 trap 处理入口函数的基地址。
2. 开启时钟中断，将 `sie[STIE]` 置 1。
3. 设置第一次时钟中断，参考 `clock_set_next_event()` ( `clock_set_next_event()` 在 4.3.4 中介绍 ) 中的逻辑用汇编实现。
4. 开启 S 态下的中断响应， 将 `sstatus[SIE]` 置 1。

实现如下：主要利用csrr和csrw函数读写控制寄存器，首先制作立即数，再用立即数去和相应的控制寄存器进行或操作来进行置位。

Head.S

```assembly
.extern start_kernel

    .section .text.init
    .globl _start
_start:
	la sp, boot_stack_top
    #---
    # set stvec
    la t2, _traps
    csrw stvec, t2 
    #---
    #set sie
    li t5, 0x20
    csrr t3, sie
    or t3, t3, t5
    csrw sie, t3
    #---
    # set interrupt
    call clock_set_next_event
    #----
    # set sstatus
    li t6, 0x2
    csrr t3, sstatus
    or t3, t3, t6
    csrw sstatus, t3
    #---
	jal start_kernel
    #---
    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top:

```

#### 2.6 实现上下文切换

在这一部分我们需要做如下的这样一些操作

1. save 32 registers and sepc to stack
2. call trap_handler

3. restore sepc and 32 registers (x2(sp) should be restore last) from stack

4. return from trap

其中：在栈上的相关操作采用sd/ld指令，读写控制状态寄存器采用csrr/csrw，传递参数用a0, a1寄存器传递参数

Entry.S

```assembly
    .section .text.entry
    .align 2
    .globl _traps 
_traps:
    # YOUR CODE HERE
    # -----------

        # 1. save 32 registers and sepc to stack
        sd x0, -8(x2)
        sd x1, -16(x2)
        sd x2, -24(x2)
        sd x3, -32(x2)
        sd x4, -40(x2)
        sd x5, -48(x2)
        sd x6, -56(x2)
        sd x7, -64(x2)
        sd x8, -72(x2)
        sd x9, -80(x2)
        sd x10, -88(x2)
        sd x11, -96(x2)
        sd x12, -104(x2)
        sd x13, -112(x2)
        sd x14, -120(x2)
        sd x15, -128(x2)
        sd x16, -136(x2)
        sd x17, -144(x2)
        sd x18, -152(x2)
        sd x19, -160(x2)
        sd x20, -168(x2)
        sd x21, -176(x2)
        sd x22, -184(x2)
        sd x23, -192(x2)
        sd x24, -200(x2)
        sd x25, -208(x2)
        sd x26, -216(x2)
        sd x27, -224(x2)
        sd x28, -232(x2)
        sd x29, -240(x2)
        sd x30, -248(x2)
        sd x31, -256(x2)

        csrr t0, sepc
        sd t0, -264(x2)
        addi x2, x2, -264
    # -----------

        # 2. call trap_handler
        csrr a0, scause
        csrr a1, sepc
        call trap_handler 
    # -----------

        # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack
        ld t0, 0(x2)
        csrw sepc, t0
        
        ld x31, 8(x2)
        ld x30, 16(x2)
        ld x29, 24(x2)
        ld x28, 32(x2)
        ld x27, 40(x2)
        ld x26, 48(x2)
        ld x25, 56(x2)
        ld x24, 64(x2)
        ld x23, 72(x2)
        ld x22, 80(x2)
        ld x21, 88(x2)
        ld x20, 96(x2)
        ld x19, 104(x2)
        ld x18, 112(x2)
        ld x17, 120(x2)
        ld x16, 128(x2)
        ld x15, 136(x2)
        ld x14, 144(x2)
        ld x13, 152(x2)
        ld x12, 160(x2)
        ld x11, 168(x2)
        ld x10, 176(x2)
        ld x9, 184(x2)
        ld x8, 192(x2)
        ld x7, 200(x2)
        ld x6, 208(x2)
        ld x5, 216(x2)
        ld x4, 224(x2)
        ld x3, 232(x2)
        ld x1, 248(x2)
        ld x0, 256(x2)
        ld x2, 240(x2)
    # -----------

        # 4. return from trap
        sret
    # -----------
```

#### 2.7 实现Trap处理函数

+ 在这一部分，我们对传入的scause进行位判断，经过查表，我们发现当scause的最高位是1以及其他位所表示的数为0x5时，表示中断类型是Supervisor timer interrupt，所以我们对此进行判断即可

```c
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
```

#### 2.8 实现时钟中断相关函数

在这一部分中，我们只用在汇编中用rdtime指令读取寄存器得到ctime寄存器中的值，在clock_set_next_event()函数中通过sbi_ecall接口调用Opensbi的sbi_set_timer即可

```c
// clock.c
#include "types.h"
#include "printk.h"
// QEMU中时钟的频率是10MHz, 也就是1秒钟相当于10000000个时钟周期。
unsigned long TIMECLOCK = 10000000;

unsigned long get_cycles() {
    // 编写内联汇编，使用 rdtime 获取 time 寄存器中 (也就是mtime 寄存器 )的值并返回
    // YOUR CODE HERE
    unsigned long r_time = 0;
    __asm__ volatile(
        "rdtime t0\n"
        "mv %[ret], t0"
        : [ret]"=r"(r_time)
        : 
        : "memory"
    );
    return r_time;
}

void clock_set_next_event() {
    // 下一次 时钟中断 的时间点
    unsigned long next = get_cycles() + TIMECLOCK;
    printk("Kernel is running\n");
    printk("[S] Supervisor Mode Timer Interrupt\n");
    // 使用 sbi_ecall 来完成对下一次时钟中断的设置
    // YOUR CODE HERE
    sbi_ecall(0x0, 0x0, next, 0, 0, 0, 0, 0);
} 

```

#### 2.9 编译以及测试

经过编译，可以看到每隔1s，虚拟机便会触发中断，输出相应内容

![image-20231013195828998](C:\Users\squarehuang\AppData\Roaming\Typora\typora-user-images\image-20231013195828998.png)

### 三、讨论和心得

​	在本次的实验中，我学习了基本的Risc-V汇编语言编写，熟悉了相应的语法。了解了OpenSBI在实验中起到的接口作用，它作为Bootloader完成机器启动时 M-mode 下的硬件初始化与寄存器设置，并提供相应接口以便我们在S-mode下可以操作M-mode相应寄存器的值。除此之外，我也重新系统学习了Makefile的编写，了解了RISC-V的Trap处理如何进行编写，并熟悉了CPU的上下文切换机制。

### 四、思考题

1. 请总结一下 RISC-V 的 calling convention，并解释 Caller / Callee Saved Register 有什么区别？

RISC-V的函数调用如下：

+ 数据对齐：低精度数据保存至寄存器中进行相应拓展，RV64将扩展至64位

+ RISC-V通过call指令来调用编写好的函数

+ 尽可能的使用寄存器来传递参数，其中包括a0-a7整数寄存器，其中a0-a1可用来传递函数返回值，以及fa0-fa7浮点数寄存器，其中fa0-fa7可用来传递返回值。2个指针字长的返回值分别放入a0与a1
+ 对于不同的参数有着不同的传递方式，整型寄存器通过ai寄存器来传递，浮点寄存器通过fai寄存器传递，结构体的每个字段会按照指针长度对齐，参数寄存器保存结构体头部8个指针字长的数据
+ 对于小于一个指针字的参数，通过寄存器的最低有效位传递，或者通过栈传递保存在指针字的低位。对于等于两个指针字的参数，通过栈传递时自然对齐。更长的参数通过reference传递
+ 栈传递时向下增长

Caller / Callee Saved Register 区别：

+ Caller Saver Register：在callee函数运行时，这些寄存器的值可能被破坏，但无需由callee自身保存，而由caller进行这些寄存器值的保存
+ Callee Saved Register：在caller调用callee的时候，这些寄存器的值需要在callee执行前进行保存，并在callee

2. 编译之后，通过 System.map 查看 vmlinux.lds 中自定义符号的值（截图）。

   编译完成后，在lab1根目录下查看System.map中自定义符号的值

   ![image-20231014202451112](C:\Users\squarehuang\AppData\Roaming\Typora\typora-user-images\image-20231014202451112.png)

3. 用 `csr_read` 宏读取 `sstatus` 寄存器的值，对照 RISC-V 手册解释其含义（截图）。

   首先对start_kernel进行修改，修改为如下所示<img src="C:\Users\squarehuang\AppData\Roaming\Typora\typora-user-images\image-20231014210353356.png" alt="image-20231014210353356" style="zoom:67%;" />

   再启动程序，可以观察到sstatus的值

   <img src="C:\Users\squarehuang\AppData\Roaming\Typora\typora-user-images\image-20231014210423583.png" alt="image-20231014210423583" style="zoom:67%;" />

   对照RISC-V Privileged Architectures可以得到sstatus寄存器的存储结构
   ![image-20231014210505067](C:\Users\squarehuang\AppData\Roaming\Typora\typora-user-images\image-20231014210505067.png)

   对照可知：

   UIE = 0: U-mode下禁止中断

   SIE = 1: S-mode下允许中断

   SPP=0： 代表privilege level是0，之前的mode是U-Mode

   SPIE=0：代表在进入supervisor mode之前是否启动了supervisor interrupt，当supervisor mode下发生了trap时，SPIE会继承SIE的值，SIE变成0

   UBE = 0: U-mode下进行的显示内存访问为小端

4. 用 `csr_write` 宏向 `sscratch` 寄存器写入数据，并验证是否写入成功（截图）。

​	修改main.c如下图所示 ，向`sscratch`寄存器中写入`0x88880000`

![image-20231014212123549](C:\Users\squarehuang\AppData\Roaming\Typora\typora-user-images\image-20231014212123549.png)

可以查看到`sscratch`寄存器已经变为`0x88880000`	![image-20231014212046463](C:\Users\squarehuang\AppData\Roaming\Typora\typora-user-images\image-20231014212046463.png)

5. Detail your steps about how to get `arch/arm64/kernel/sys.i`

   + 运行如下指令搜索linux下用于arm64的交叉编译器，并进行安装，选择gcc-10版本，创建软连接到aarch64-linux-gnu-gcc

     ```shell
     sudo ln -sf aarch64-linux-gnu-gcc-10 aarch64-linux-gnu-gcc
     ```

     ![image-20231014220454103](C:\Users\squarehuang\AppData\Roaming\Typora\typora-user-images\image-20231014220454103.png)

   + 在之前下载的linux目录下面执行如下命令

     ```shell
     make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
     make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- /arch/arm64/kernel/sys.i
     ```

     此处显示已经编译好了`sys.i`文件

     ![image-20231014223922422](C:\Users\squarehuang\AppData\Roaming\Typora\typora-user-images\image-20231014223922422.png)

   + 可以搜索到当前目录存在sys.i

     ![image-20231014223301097](C:\Users\squarehuang\AppData\Roaming\Typora\typora-user-images\image-20231014223301097.png)

6. Find system call table of Linux v6.0 for `ARM32`, `RISC-V(32 bit)`, `RISC-V(64 bit)`, `x86(32 bit)`, `x86_64`
   List source code file, the whole system call table with macro expanded, screenshot every step.

+ 首先下载linux-6.0.1源码并解压，解压后，在`~/linux-6.0.1/arch/example-arch`下进行有关文件的搜索

![image-20231015103544216](C:\Users\squarehuang\AppData\Roaming\Typora\typora-user-images\image-20231015103544216.png)

+ arm32: 文件是`~/linux-6.0.1/arch/arm/tools/syscall.tbl`，

  完整调用表可以见附件

  ![image-20231015103738576](C:\Users\squarehuang\AppData\Roaming\Typora\typora-user-images\image-20231015103738576.png)

+ RISC-V_32/64: 文件应位于`~/linux-6.5.4/include/uapi/asm-genetic/unisted.h`中，与其他体系结构共享一个通用的系统调用表，如下图所示

  ![image-20231015111356268](C:\Users\squarehuang\AppData\Roaming\Typora\typora-user-images\image-20231015111356268.png)

+ x86_32：文件位于`./entry/syscalls/syscall_32.tbl`

  ![image-20231015104809402](C:\Users\squarehuang\AppData\Roaming\Typora\typora-user-images\image-20231015104809402.png)

+ x86_64：文件位于`./entry/syscalls/syscall_64.tbl`

![image-20231015104819892](C:\Users\squarehuang\AppData\Roaming\Typora\typora-user-images\image-20231015104819892.png)



7. Explain what is ELF file? Try readelf and objdump command on an ELF file, give screenshot of the output.
   Run an ELF file and cat `/proc/PID/maps` to give its memory layout.

+ ELF file: ELF全称是Executable and Linkable Format，即可执行可链接文件，是一种二进制文件格式，用于在类Unix系统中存储可执行程序、共享库以及充当目标文件。vmlinux即为一种ELF file

+ 使用`readelf -s vmlinux`查看lab1内核编译出的vmlinux的symbol table

  ![image-20231014231049300](C:\Users\squarehuang\AppData\Roaming\Typora\typora-user-images\image-20231014231049300.png)

+ 使用`objdump -t vmlinux`查看lab1内核编译出的vmlinux的symbol table

  ![image-20231014231632467](C:\Users\squarehuang\AppData\Roaming\Typora\typora-user-images\image-20231014231632467.png)

8. 在我们使用make run时， OpenSBI 会产生如下输出:

```plaintext
    OpenSBI v0.9
     ____                    _____ ____ _____
    / __ \                  / ____|  _ \_   _|
   | |  | |_ __   ___ _ __ | (___ | |_) || |
   | |  | | '_ \ / _ \ '_ \ \___ \|  _ < | |
   | |__| | |_) |  __/ | | |____) | |_) || |_
    \____/| .__/ \___|_| |_|_____/|____/_____|
          | |
          |_|

    ......

    Boot HART MIDELEG         : 0x0000000000000222
    Boot HART MEDELEG         : 0x000000000000b109

    ......
```

通过查看 `RISC-V Privileged Spec` 中的 `medeleg` 和 `mideleg` ，解释上面 `MIDELEG` 值的含义。

<img src="C:\Users\squarehuang\AppData\Roaming\Typora\typora-user-images\image-20231015101229147.png" alt="image-20231015101229147" style="zoom:67%;" />

+ 指令背景：medeleg和midelog是Machine Trap Delegation Registers，用作指示某种级别的Trap委托给某种级别的trap handler进行处理。1011000100001001

  具体来说，在medeleg和mideleg中set bit的时候，会将S-mode或U-mode中的相应Trap委托给S-mode trap handler。

+ 具体含义：

  MEDELEG: 意味着mcause寄存器中的S/M-Mode software interrupt, U Mode external interrupt都为1，代表着这些interrupt都可以被委派到更低权限的trap handler进行处理

  MIDELEG：意味着mip寄存器中的SSIP, STIP，SEIP都为1，即S-mode 
  software interrupts, S-mode timer interrupts, S-mode external interrupts的trap delegation都被开启了，即当前的interrupt都可以在S-mode下进行处理



### 五、附录

1. arm系统调用表：

```
#
# Linux system call numbers and entry vectors
#
# The format is:
# <num>	<abi>	<name>			[<entry point>			[<oabi compat entry point>]]
#
# Where abi is:
#  common - for system calls shared between oabi and eabi (may have compat)
#  oabi   - for oabi-only system calls (may have compat)
#  eabi   - for eabi-only system calls
#
# For each syscall number, "common" is mutually exclusive with oabi and eabi
#
0	common	restart_syscall		sys_restart_syscall
1	common	exit			sys_exit
2	common	fork			sys_fork
3	common	read			sys_read
4	common	write			sys_write
5	common	open			sys_open
6	common	close			sys_close
# 7 was sys_waitpid
8	common	creat			sys_creat
9	common	link			sys_link
10	common	unlink			sys_unlink
11	common	execve			sys_execve
12	common	chdir			sys_chdir
13	oabi	time			sys_time32
14	common	mknod			sys_mknod
15	common	chmod			sys_chmod
16	common	lchown			sys_lchown16
# 17 was sys_break
# 18 was sys_stat
19	common	lseek			sys_lseek
20	common	getpid			sys_getpid
21	common	mount			sys_mount
22	oabi	umount			sys_oldumount
23	common	setuid			sys_setuid16
24	common	getuid			sys_getuid16
25	oabi	stime			sys_stime32
26	common	ptrace			sys_ptrace
27	oabi	alarm			sys_alarm
# 28 was sys_fstat
29	common	pause			sys_pause
30	oabi	utime			sys_utime32
# 31 was sys_stty
# 32 was sys_gtty
33	common	access			sys_access
34	common	nice			sys_nice
# 35 was sys_ftime
36	common	sync			sys_sync
37	common	kill			sys_kill
38	common	rename			sys_rename
39	common	mkdir			sys_mkdir
40	common	rmdir			sys_rmdir
41	common	dup			sys_dup
42	common	pipe			sys_pipe
43	common	times			sys_times
# 44 was sys_prof
45	common	brk			sys_brk
46	common	setgid			sys_setgid16
47	common	getgid			sys_getgid16
# 48 was sys_signal
49	common	geteuid			sys_geteuid16
50	common	getegid			sys_getegid16
51	common	acct			sys_acct
52	common	umount2			sys_umount
# 53 was sys_lock
54	common	ioctl			sys_ioctl
55	common	fcntl			sys_fcntl
# 56 was sys_mpx
57	common	setpgid			sys_setpgid
# 58 was sys_ulimit
# 59 was sys_olduname
60	common	umask			sys_umask
61	common	chroot			sys_chroot
62	common	ustat			sys_ustat
63	common	dup2			sys_dup2
64	common	getppid			sys_getppid
65	common	getpgrp			sys_getpgrp
66	common	setsid			sys_setsid
67	common	sigaction		sys_sigaction
# 68 was sys_sgetmask
# 69 was sys_ssetmask
70	common	setreuid		sys_setreuid16
71	common	setregid		sys_setregid16
72	common	sigsuspend		sys_sigsuspend
73	common	sigpending		sys_sigpending
74	common	sethostname		sys_sethostname
75	common	setrlimit		sys_setrlimit
# Back compat 2GB limited rlimit
76	oabi	getrlimit		sys_old_getrlimit
77	common	getrusage		sys_getrusage
78	common	gettimeofday		sys_gettimeofday
79	common	settimeofday		sys_settimeofday
80	common	getgroups		sys_getgroups16
81	common	setgroups		sys_setgroups16
82	oabi	select			sys_old_select
83	common	symlink			sys_symlink
# 84 was sys_lstat
85	common	readlink		sys_readlink
86	common	uselib			sys_uselib
87	common	swapon			sys_swapon
88	common	reboot			sys_reboot
89	oabi	readdir			sys_old_readdir
90	oabi	mmap			sys_old_mmap
91	common	munmap			sys_munmap
92	common	truncate		sys_truncate
93	common	ftruncate		sys_ftruncate
94	common	fchmod			sys_fchmod
95	common	fchown			sys_fchown16
96	common	getpriority		sys_getpriority
97	common	setpriority		sys_setpriority
# 98 was sys_profil
99	common	statfs			sys_statfs
100	common	fstatfs			sys_fstatfs
# 101 was sys_ioperm
102	oabi	socketcall		sys_socketcall		sys_oabi_socketcall
103	common	syslog			sys_syslog
104	common	setitimer		sys_setitimer
105	common	getitimer		sys_getitimer
106	common	stat			sys_newstat
107	common	lstat			sys_newlstat
108	common	fstat			sys_newfstat
# 109 was sys_uname
# 110 was sys_iopl
111	common	vhangup			sys_vhangup
# 112 was sys_idle
# syscall to call a syscall!
113	oabi	syscall			sys_syscall
114	common	wait4			sys_wait4
115	common	swapoff			sys_swapoff
116	common	sysinfo			sys_sysinfo
117	oabi	ipc			sys_ipc			sys_oabi_ipc
118	common	fsync			sys_fsync
119	common	sigreturn		sys_sigreturn_wrapper
120	common	clone			sys_clone
121	common	setdomainname		sys_setdomainname
122	common	uname			sys_newuname
# 123 was sys_modify_ldt
124	common	adjtimex		sys_adjtimex_time32
125	common	mprotect		sys_mprotect
126	common	sigprocmask		sys_sigprocmask
# 127 was sys_create_module
128	common	init_module		sys_init_module
129	common	delete_module		sys_delete_module
# 130 was sys_get_kernel_syms
131	common	quotactl		sys_quotactl
132	common	getpgid			sys_getpgid
133	common	fchdir			sys_fchdir
134	common	bdflush			sys_ni_syscall
135	common	sysfs			sys_sysfs
136	common	personality		sys_personality
# 137 was sys_afs_syscall
138	common	setfsuid		sys_setfsuid16
139	common	setfsgid		sys_setfsgid16
140	common	_llseek			sys_llseek
141	common	getdents		sys_getdents
142	common	_newselect		sys_select
143	common	flock			sys_flock
144	common	msync			sys_msync
145	common	readv			sys_readv
146	common	writev			sys_writev
147	common	getsid			sys_getsid
148	common	fdatasync		sys_fdatasync
149	common	_sysctl			sys_ni_syscall
150	common	mlock			sys_mlock
151	common	munlock			sys_munlock
152	common	mlockall		sys_mlockall
153	common	munlockall		sys_munlockall
154	common	sched_setparam		sys_sched_setparam
155	common	sched_getparam		sys_sched_getparam
156	common	sched_setscheduler	sys_sched_setscheduler
157	common	sched_getscheduler	sys_sched_getscheduler
158	common	sched_yield		sys_sched_yield
159	common	sched_get_priority_max	sys_sched_get_priority_max
160	common	sched_get_priority_min	sys_sched_get_priority_min
161	common	sched_rr_get_interval	sys_sched_rr_get_interval_time32
162	common	nanosleep		sys_nanosleep_time32
163	common	mremap			sys_mremap
164	common	setresuid		sys_setresuid16
165	common	getresuid		sys_getresuid16
# 166 was sys_vm86
# 167 was sys_query_module
168	common	poll			sys_poll
169	common	nfsservctl
170	common	setresgid		sys_setresgid16
171	common	getresgid		sys_getresgid16
172	common	prctl			sys_prctl
173	common	rt_sigreturn		sys_rt_sigreturn_wrapper
174	common	rt_sigaction		sys_rt_sigaction
175	common	rt_sigprocmask		sys_rt_sigprocmask
176	common	rt_sigpending		sys_rt_sigpending
177	common	rt_sigtimedwait		sys_rt_sigtimedwait_time32
178	common	rt_sigqueueinfo		sys_rt_sigqueueinfo
179	common	rt_sigsuspend		sys_rt_sigsuspend
180	common	pread64			sys_pread64		sys_oabi_pread64
181	common	pwrite64		sys_pwrite64		sys_oabi_pwrite64
182	common	chown			sys_chown16
183	common	getcwd			sys_getcwd
184	common	capget			sys_capget
185	common	capset			sys_capset
186	common	sigaltstack		sys_sigaltstack
187	common	sendfile		sys_sendfile
# 188 reserved
# 189 reserved
190	common	vfork			sys_vfork
# SuS compliant getrlimit
191	common	ugetrlimit		sys_getrlimit
192	common	mmap2			sys_mmap2
193	common	truncate64		sys_truncate64		sys_oabi_truncate64
194	common	ftruncate64		sys_ftruncate64		sys_oabi_ftruncate64
195	common	stat64			sys_stat64		sys_oabi_stat64
196	common	lstat64			sys_lstat64		sys_oabi_lstat64
197	common	fstat64			sys_fstat64		sys_oabi_fstat64
198	common	lchown32		sys_lchown
199	common	getuid32		sys_getuid
200	common	getgid32		sys_getgid
201	common	geteuid32		sys_geteuid
202	common	getegid32		sys_getegid
203	common	setreuid32		sys_setreuid
204	common	setregid32		sys_setregid
205	common	getgroups32		sys_getgroups
206	common	setgroups32		sys_setgroups
207	common	fchown32		sys_fchown
208	common	setresuid32		sys_setresuid
209	common	getresuid32		sys_getresuid
210	common	setresgid32		sys_setresgid
211	common	getresgid32		sys_getresgid
212	common	chown32			sys_chown
213	common	setuid32		sys_setuid
214	common	setgid32		sys_setgid
215	common	setfsuid32		sys_setfsuid
216	common	setfsgid32		sys_setfsgid
217	common	getdents64		sys_getdents64
218	common	pivot_root		sys_pivot_root
219	common	mincore			sys_mincore
220	common	madvise			sys_madvise
221	common	fcntl64			sys_fcntl64		sys_oabi_fcntl64
# 222 for tux
# 223 is unused
224	common	gettid			sys_gettid
225	common	readahead		sys_readahead		sys_oabi_readahead
226	common	setxattr		sys_setxattr
227	common	lsetxattr		sys_lsetxattr
228	common	fsetxattr		sys_fsetxattr
229	common	getxattr		sys_getxattr
230	common	lgetxattr		sys_lgetxattr
231	common	fgetxattr		sys_fgetxattr
232	common	listxattr		sys_listxattr
233	common	llistxattr		sys_llistxattr
234	common	flistxattr		sys_flistxattr
235	common	removexattr		sys_removexattr
236	common	lremovexattr		sys_lremovexattr
237	common	fremovexattr		sys_fremovexattr
238	common	tkill			sys_tkill
239	common	sendfile64		sys_sendfile64
240	common	futex			sys_futex_time32
241	common	sched_setaffinity	sys_sched_setaffinity
242	common	sched_getaffinity	sys_sched_getaffinity
243	common	io_setup		sys_io_setup
244	common	io_destroy		sys_io_destroy
245	common	io_getevents		sys_io_getevents_time32
246	common	io_submit		sys_io_submit
247	common	io_cancel		sys_io_cancel
248	common	exit_group		sys_exit_group
249	common	lookup_dcookie		sys_lookup_dcookie
250	common	epoll_create		sys_epoll_create
251	common	epoll_ctl		sys_epoll_ctl		sys_oabi_epoll_ctl
252	common	epoll_wait		sys_epoll_wait
253	common	remap_file_pages	sys_remap_file_pages
# 254 for set_thread_area
# 255 for get_thread_area
256	common	set_tid_address		sys_set_tid_address
257	common	timer_create		sys_timer_create
258	common	timer_settime		sys_timer_settime32
259	common	timer_gettime		sys_timer_gettime32
260	common	timer_getoverrun	sys_timer_getoverrun
261	common	timer_delete		sys_timer_delete
262	common	clock_settime		sys_clock_settime32
263	common	clock_gettime		sys_clock_gettime32
264	common	clock_getres		sys_clock_getres_time32
265	common	clock_nanosleep		sys_clock_nanosleep_time32
266	common	statfs64		sys_statfs64_wrapper
267	common	fstatfs64		sys_fstatfs64_wrapper
268	common	tgkill			sys_tgkill
269	common	utimes			sys_utimes_time32
270	common	arm_fadvise64_64	sys_arm_fadvise64_64
271	common	pciconfig_iobase	sys_pciconfig_iobase
272	common	pciconfig_read		sys_pciconfig_read
273	common	pciconfig_write		sys_pciconfig_write
274	common	mq_open			sys_mq_open
275	common	mq_unlink		sys_mq_unlink
276	common	mq_timedsend		sys_mq_timedsend_time32
277	common	mq_timedreceive		sys_mq_timedreceive_time32
278	common	mq_notify		sys_mq_notify
279	common	mq_getsetattr		sys_mq_getsetattr
280	common	waitid			sys_waitid
281	common	socket			sys_socket
282	common	bind			sys_bind		sys_oabi_bind
283	common	connect			sys_connect		sys_oabi_connect
284	common	listen			sys_listen
285	common	accept			sys_accept
286	common	getsockname		sys_getsockname
287	common	getpeername		sys_getpeername
288	common	socketpair		sys_socketpair
289	common	send			sys_send
290	common	sendto			sys_sendto		sys_oabi_sendto
291	common	recv			sys_recv
292	common	recvfrom		sys_recvfrom
293	common	shutdown		sys_shutdown
294	common	setsockopt		sys_setsockopt
295	common	getsockopt		sys_getsockopt
296	common	sendmsg			sys_sendmsg		sys_oabi_sendmsg
297	common	recvmsg			sys_recvmsg
298	common	semop			sys_semop		sys_oabi_semop
299	common	semget			sys_semget
300	common	semctl			sys_old_semctl
301	common	msgsnd			sys_msgsnd
302	common	msgrcv			sys_msgrcv
303	common	msgget			sys_msgget
304	common	msgctl			sys_old_msgctl
305	common	shmat			sys_shmat
306	common	shmdt			sys_shmdt
307	common	shmget			sys_shmget
308	common	shmctl			sys_old_shmctl
309	common	add_key			sys_add_key
310	common	request_key		sys_request_key
311	common	keyctl			sys_keyctl
312	common	semtimedop		sys_semtimedop_time32	sys_oabi_semtimedop
313	common	vserver
314	common	ioprio_set		sys_ioprio_set
315	common	ioprio_get		sys_ioprio_get
316	common	inotify_init		sys_inotify_init
317	common	inotify_add_watch	sys_inotify_add_watch
318	common	inotify_rm_watch	sys_inotify_rm_watch
319	common	mbind			sys_mbind
320	common	get_mempolicy		sys_get_mempolicy
321	common	set_mempolicy		sys_set_mempolicy
322	common	openat			sys_openat
323	common	mkdirat			sys_mkdirat
324	common	mknodat			sys_mknodat
325	common	fchownat		sys_fchownat
326	common	futimesat		sys_futimesat_time32
327	common	fstatat64		sys_fstatat64		sys_oabi_fstatat64
328	common	unlinkat		sys_unlinkat
329	common	renameat		sys_renameat
330	common	linkat			sys_linkat
331	common	symlinkat		sys_symlinkat
332	common	readlinkat		sys_readlinkat
333	common	fchmodat		sys_fchmodat
334	common	faccessat		sys_faccessat
335	common	pselect6		sys_pselect6_time32
336	common	ppoll			sys_ppoll_time32
337	common	unshare			sys_unshare
338	common	set_robust_list		sys_set_robust_list
339	common	get_robust_list		sys_get_robust_list
340	common	splice			sys_splice
341	common	arm_sync_file_range	sys_sync_file_range2
342	common	tee			sys_tee
343	common	vmsplice		sys_vmsplice
344	common	move_pages		sys_move_pages
345	common	getcpu			sys_getcpu
346	common	epoll_pwait		sys_epoll_pwait
347	common	kexec_load		sys_kexec_load
348	common	utimensat		sys_utimensat_time32
349	common	signalfd		sys_signalfd
350	common	timerfd_create		sys_timerfd_create
351	common	eventfd			sys_eventfd
352	common	fallocate		sys_fallocate
353	common	timerfd_settime		sys_timerfd_settime32
354	common	timerfd_gettime		sys_timerfd_gettime32
355	common	signalfd4		sys_signalfd4
356	common	eventfd2		sys_eventfd2
357	common	epoll_create1		sys_epoll_create1
358	common	dup3			sys_dup3
359	common	pipe2			sys_pipe2
360	common	inotify_init1		sys_inotify_init1
361	common	preadv			sys_preadv
362	common	pwritev			sys_pwritev
363	common	rt_tgsigqueueinfo	sys_rt_tgsigqueueinfo
364	common	perf_event_open		sys_perf_event_open
365	common	recvmmsg		sys_recvmmsg_time32
366	common	accept4			sys_accept4
367	common	fanotify_init		sys_fanotify_init
368	common	fanotify_mark		sys_fanotify_mark
369	common	prlimit64		sys_prlimit64
370	common	name_to_handle_at	sys_name_to_handle_at
371	common	open_by_handle_at	sys_open_by_handle_at
372	common	clock_adjtime		sys_clock_adjtime32
373	common	syncfs			sys_syncfs
374	common	sendmmsg		sys_sendmmsg
375	common	setns			sys_setns
376	common	process_vm_readv	sys_process_vm_readv
377	common	process_vm_writev	sys_process_vm_writev
378	common	kcmp			sys_kcmp
379	common	finit_module		sys_finit_module
380	common	sched_setattr		sys_sched_setattr
381	common	sched_getattr		sys_sched_getattr
382	common	renameat2		sys_renameat2
383	common	seccomp			sys_seccomp
384	common	getrandom		sys_getrandom
385	common	memfd_create		sys_memfd_create
386	common	bpf			sys_bpf
387	common	execveat		sys_execveat
388	common	userfaultfd		sys_userfaultfd
389	common	membarrier		sys_membarrier
390	common	mlock2			sys_mlock2
391	common	copy_file_range		sys_copy_file_range
392	common	preadv2			sys_preadv2
393	common	pwritev2		sys_pwritev2
394	common	pkey_mprotect		sys_pkey_mprotect
395	common	pkey_alloc		sys_pkey_alloc
396	common	pkey_free		sys_pkey_free
397	common	statx			sys_statx
398	common	rseq			sys_rseq
399	common	io_pgetevents		sys_io_pgetevents_time32
400	common	migrate_pages		sys_migrate_pages
401	common	kexec_file_load		sys_kexec_file_load
# 402 is unused
403	common	clock_gettime64			sys_clock_gettime
404	common	clock_settime64			sys_clock_settime
405	common	clock_adjtime64			sys_clock_adjtime
406	common	clock_getres_time64		sys_clock_getres
407	common	clock_nanosleep_time64		sys_clock_nanosleep
408	common	timer_gettime64			sys_timer_gettime
409	common	timer_settime64			sys_timer_settime
410	common	timerfd_gettime64		sys_timerfd_gettime
411	common	timerfd_settime64		sys_timerfd_settime
412	common	utimensat_time64		sys_utimensat
413	common	pselect6_time64			sys_pselect6
414	common	ppoll_time64			sys_ppoll
416	common	io_pgetevents_time64		sys_io_pgetevents
417	common	recvmmsg_time64			sys_recvmmsg
418	common	mq_timedsend_time64		sys_mq_timedsend
419	common	mq_timedreceive_time64		sys_mq_timedreceive
420	common	semtimedop_time64		sys_semtimedop
421	common	rt_sigtimedwait_time64		sys_rt_sigtimedwait
422	common	futex_time64			sys_futex
423	common	sched_rr_get_interval_time64	sys_sched_rr_get_interval
424	common	pidfd_send_signal		sys_pidfd_send_signal
425	common	io_uring_setup			sys_io_uring_setup
426	common	io_uring_enter			sys_io_uring_enter
427	common	io_uring_register		sys_io_uring_register
428	common	open_tree			sys_open_tree
429	common	move_mount			sys_move_mount
430	common	fsopen				sys_fsopen
431	common	fsconfig			sys_fsconfig
432	common	fsmount				sys_fsmount
433	common	fspick				sys_fspick
434	common	pidfd_open			sys_pidfd_open
435	common	clone3				sys_clone3
436	common	close_range			sys_close_range
437	common	openat2				sys_openat2
438	common	pidfd_getfd			sys_pidfd_getfd
439	common	faccessat2			sys_faccessat2
440	common	process_madvise			sys_process_madvise
441	common	epoll_pwait2			sys_epoll_pwait2
442	common	mount_setattr			sys_mount_setattr
443	common	quotactl_fd			sys_quotactl_fd
444	common	landlock_create_ruleset		sys_landlock_create_ruleset
445	common	landlock_add_rule		sys_landlock_add_rule
446	common	landlock_restrict_self		sys_landlock_restrict_self
# 447 reserved for memfd_secret
448	common	process_mrelease		sys_process_mrelease
449	common	futex_waitv			sys_futex_waitv
450	common	set_mempolicy_home_node		sys_set_mempolicy_home_node

```





