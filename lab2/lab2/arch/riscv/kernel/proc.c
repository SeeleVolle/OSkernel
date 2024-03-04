//arch/riscv/kernel/proc.c
#include "proc.h"
#include "mm.h"
#include "defs.h"
#include "rand.h"
#include "printk.h"
#include "test.h"

//arch/riscv/kernel/proc.c

extern void __dummy();
extern void __switch_to(struct task_struct* prev, struct task_struct* next);

struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组, 所有的线程都保存在此

/**
 * new content for unit test of 2023 OS lab2
*/
extern uint64 task_test_priority[]; // test_init 后, 用于初始化 task[i].priority 的数组
extern uint64 task_test_counter[];  // test_init 后, 用于初始化 task[i].counter  的数组

void task_init() {
    test_init(NR_TASKS);
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    idle = (struct task_struct *)kalloc();
    // 2. 设置 state 为 TASK_RUNNING;
    idle->state = TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    idle->counter = 0;
    idle->priority = 0;
    // 4. 设置 idle 的 pid 为 0
    idle->pid = 0;
    // 5. 将 current 和 task[0] 指向 idle
    task[0] = idle;
    current = idle;
    /* YOUR CODE HERE */

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    for(int i = 1; i < NR_TASKS; i++){
        task[i] = (struct task_struct *)kalloc();
        task[i]->state = TASK_RUNNING;
        task[i]->pid = i;
        task[i]->counter = task_test_counter[i];
        task[i]->priority = task_test_priority[i];
        // printk("task[i]->pid: %d\n", task[i]->pid);
        // printk("task[i]->counter: %d\n", task[i]->counter);
        // printk("task[i]->priority: %d\n", task[i]->priority);

        task[i]->thread.ra = (uint64)__dummy;
        task[i]->thread.sp = (uint64)task[i] + 4096;

    }
    // 2. 其中每个线程的 state 为 TASK_RUNNING, 此外，为了单元测试的需要，counter 和 priority 进行如下赋值：
    //      task[i].counter  = task_test_counter[i];
    //      task[i].priority = task_test_priority[i];
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`,
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址,  `sp` 设置为 该线程申请的物理页的高地址

    /* YOUR CODE HERE */

    printk("...proc_init done!\n");
}

// arch/riscv/kernel/proc.c
void dummy() {
    // printk("Hello, I am dummy\n");
    // printk("current->pid: %d\n", current->pid);
    // printk("current->counter: %d\n", current->counter);

    // schedule_test();
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if ((last_counter == -1 || current->counter != last_counter) && current->counter > 0) {
            // if(current->counter == 1){
            //     --(current->counter);   // forced the counter to be zero if this thread is going to be scheduled
            // }                           // in case that the new counter is also 1, leading the information not printed.
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
        }
    }
}

void schedule(){
    // printk("I am schedule\n");
#ifdef SJF
    int min_i = 0, cnt = 0;
    while(1){
        int min_counter = 1000;
        min_i = 1, cnt = 0;
        for(int i = 1; i < NR_TASKS; i++)
        {
            if(task[i]->counter == 0)
                cnt++;
            if(task[i]->state == TASK_RUNNING && task[i]->counter > 0 && task[i]->counter < min_counter)
            {
                min_counter = task[i]->counter;
                min_i = i;
            }
        }
        if(cnt == NR_TASKS - 1)
        {
            for(int i = 1; i < NR_TASKS; i++)
                task[i]->counter = rand() % 10;
        }
        else 
            break;
    }
    // printk("schedule_Next_pid: %d\n", task[min_i]->pid);
    if(min_i != 0)
        switch_to(task[min_i]);
    else
        printk("Error occured when schedule!\n");
#endif
#ifdef PRIORITY
    int c, next, i;
    while(1){
        // printk("Why you are stunned here\n");
        c = 0;
        next = 0;
        i = NR_TASKS;
        struct task_struct ** p = &task[NR_TASKS];
        while(--i > 0){
            // printk("pid: %d\n", task[i]->pid);
            // printk("state: %d\n", task[i]->state);
            // printk("counter: %d\n", task[i]->counter);
            if( task[i]->state == TASK_RUNNING && task[i]->counter > c)
            {
                c = (task[i])->counter;
                next = i;
            }
        }
        // printk("c: %d\n", c);
        if(c > 0) break;
        for(int j = 1; j <= NR_TASKS; j++)
            if (task[j])
                task[j]->counter = (task[j]->counter >> 1) + task[j]->priority;
    }
    // printk("next :%d\n", next);
    if(next != current->pid && next != 0)
	    switch_to(task[next]);
#endif
}



void switch_to(struct task_struct* next) {
    if(next->pid == current->pid)
        return;
    // printk("current_pid: %d\n", current->pid);
    // printk("Next_pid: %d\n", next->pid);

    //由于__switch_to只实现了上下文切换功能，所以这里需要对current指针进行更新
    struct task_struct* prev = current;
    current = next; 
        // 调用 __switch_to 进行线程切换
    __switch_to(prev, next);
}

void do_timer() {
    if(current->pid == 0)
        schedule();
    else{
        //  printk("current_pid: %d\n", current->pid);
        //  printk("current_counter: %d\n", current->counter);
        current->counter--;
        if(current->counter == 0)
            schedule();
        else{
            return;
        }
    }
}