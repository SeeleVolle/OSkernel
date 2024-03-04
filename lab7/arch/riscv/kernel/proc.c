//arch/riscv/kernel/proc.c
#include "proc.h"
#include "mm.h"
#include "defs.h"
#include "rand.h"
#include "printk.h"
#include "test.h"
#include "elf.h"


//arch/riscv/kernel/proc.c

extern char _stext[];
extern char _srodata[];
extern char _sdata[];
extern char ramdisk_start[];
extern char ramdisk_end[];
extern unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));


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

static uint64_t load_program(struct task_struct* task) {
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)ramdisk_start;

    uint64_t phdr_start = (uint64_t)ehdr + ehdr->e_phoff;
    int phdr_cnt = ehdr->e_phnum;
  
    Elf64_Phdr* phdr;
    int load_phdr_cnt = 0;
    for (int i = 0; i < phdr_cnt; i++) {
        phdr = (Elf64_Phdr*)(phdr_start + sizeof(Elf64_Phdr) * i);
        if (phdr->p_type == PT_LOAD) {
            // alloc space and copy content
            uint64 page_num = (phdr->p_vaddr - PGROUNDDOWN((uint64)phdr->p_vaddr) + phdr->p_memsz + PGSIZE - 1) / PGSIZE;
            uint64 uapp = alloc_pages(page_num);
            memset(uapp, 0, page_num*PGSIZE);
            for(int t = 0; t < page_num; t++){
                uint64* src = (uint64*)(ramdisk_start + t * PGSIZE + phdr->p_offset);
                uint64* dst = (uint64*)(uapp + t * PGSIZE + phdr->p_vaddr - PGROUNDDOWN((uint64)phdr->p_vaddr));
                for (int k = 0; k < PGSIZE / 8; k++) {
                    dst[k] = src[k]; 
                }
            }
          	// do mapping  X|W|R|V
            create_mapping(task->pgd, PGROUNDDOWN((uint64)phdr->p_vaddr), uapp - PA2VA_OFFSET, PGSIZE*page_num, 
            ((phdr->p_flags & PF_X) << 3) | ((phdr->p_flags & PF_W) << 1) | ((phdr->p_flags & PF_R) >> 1) | 0b10001);
        }
    }
  
    // allocate user stack and do mapping
    // code...
    create_mapping(task->pgd, USER_END - PGSIZE, (uint64)kalloc - PA2VA_OFFSET, PGSIZE, 23);
  
    // following code has been written for you
    // set user stack
    // pc for the user program
    task->thread.sepc = ehdr->e_entry;
    // sstatus bits set
    task->thread.sstatus = (1ULL << 5) + (1ULL << 18);
    // user stack for user program
    task->thread.sscratch = USER_END;
}


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
    idle->pgd=swapper_pg_dir;
    // 5. 将 current 和 task[0] 指向 idle
    task[0] = idle;
    current = idle;
    /* YOUR CODE HERE */

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    for(int i = 1; i < 2; i++){
        task[i] = (struct task_struct *)kalloc();
        task[i]->state = TASK_RUNNING;
        task[i]->pid = i;
        task[i]->counter = task_test_counter[i];
        task[i]->priority = task_test_priority[i];
        task[i]->thread.ra = (uint64)__dummy;
        task[i]->thread.sp = (uint64)task[i] + 4096;    

        task[i]->pgd = (pagetable_t)kalloc();
        memset(task[i]->pgd , 0, PGSIZE);
        for(int j = 0; j < PGSIZE / 8; j++){
            task[i]->pgd[j]=swapper_pg_dir[j];
        }

        load_program(task[i]);

        // uint64 uapp = alloc_pages((PGROUNDUP((uint64)uapp_end) - (uint64)uapp_start) / PGSIZE);
        // for(int t = 0; t < (PGROUNDUP((uint64)uapp_end) - (uint64)uapp_start) / PGSIZE; t++){
        //     uint64* src = (uint64*)(uapp_start + t * PGSIZE);
        //     uint64* dst = (uint64*)(uapp + t * PGSIZE);
        //     for (int k = 0; k < PGSIZE / 8; k++) {
        //         dst[k] = src[k];
        //     }
        // }
        
        // // if(i == 1)
        // // {
        // //     printk("uapp_start: %lx\n", (uint64)uapp_start);
        // //     printk("uapp_end: %lx\n", (uint64)uapp_end);
        // // }

        // create_mapping(task[i]->pgd , (uint64)USER_START,  (uint64)uapp -(uint64)PA2VA_OFFSET, (uint64)uapp_end - (uint64)uapp_start, 31);
        // create_mapping(task[i]->pgd , (uint64)USER_END - (uint64)PGSIZE, (uint64)kalloc() - (uint64)PA2VA_OFFSET, (uint64)PGSIZE, 23);

        // // if(i == 1)
        // // {
        // //     printk("task[i]->pgd: %lx\n", (((uint64)(task[i]->pgd))));
        // //     printk("task[i]->pgd: %lx\n", (uint64)(PA2VA_OFFSET));
        // //     printk("task[i]->pgd: %lx\n", (uint64)task[i]->pgd - (uint64)PA2VA_OFFSET);
        // //     printk("task[i]->pgd: %lx\n", (((uint64)((uint64)task[i]->pgd - (uint64)PA2VA_OFFSET) >> 12)));
        // // }


        // task[i]->thread.sepc = USER_START;
        // task[i]->thread.sstatus = (1ULL << 5) + (1ULL << 18);
        // task[i]->thread.sscratch = USER_END;
        task[i]->files = file_init();
    }

    printk("[S] ...proc_init done!\n");

    // printk("Checking Write Permission:\n");
    // *(_stext) = 0x35;
    // *(_srodata) = 0x36;
    // printk(".text: %lx\n", *(_stext));
    // printk(".rodata: %lx\n", *(_srodata));
    // *(_stext) = 0x9b;
    // *(_srodata) = 0x2e;
   
}


// arch/riscv/kernel/proc.c
void dummy() {
    // printk("Hello, I am dummy\n");
    // printk("current->pid: %d\n", current->pid);
    // printk("current->counter: %d\n", current->counter);

    schedule_test();
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
            printk("[PID = %d] is running. thread space begin at 0x%lx\n", current->pid, (unsigned long)current);
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
            if(task[i] == NULL || task[i]->counter == 0){
                cnt++;
                continue;
            }
            if(task[i]->state == TASK_RUNNING && task[i]->counter > 0 && task[i]->counter < min_counter)
            {
                min_counter = task[i]->counter;
                min_i = i;
            }
        }
        if(cnt == NR_TASKS - 1)
        {
            for(int i = 1; i < NR_TASKS; i++){
                if(task[i] == NULL)
                    continue;
                task[i]->counter = rand() % 10;
            }
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
        c = 0;
        next = 0;
        i = NR_TASKS;
        struct task_struct ** p = &task[NR_TASKS];
        while(--i > 0){
            if(task[i] == NULL)
                continue;
            // printk("pid: %d\n", task[i]->pid);
            // printk("state: %d\n", task[i]->state);
            // printk("counter: %d\n", task[i]->counter);
            if(task[i]->state == TASK_RUNNING && task[i]->counter > c)
            {
                c = (task[i])->counter;
                next = i;
            }
        }
        // printk("c: %d\n", c);
        if(c > 0) break;
        for(int j = 1; j <= NR_TASKS; j++)
            if (task[j]){
                task[j]->counter = (task[j]->counter >> 1) + task[j]->priority;
                printk("%d\n", task[j]->counter);
            }
                
    }
    if(next != current->pid && next != 0)
	    switch_to(task[next]);
#endif
}



void switch_to(struct task_struct* next) {
    if(next->pid == current->pid)
        return;
    // printk("current_pid: %d\n", current->pid);
    // printk("Next_pid: %d\n", next->pid);
    printk("switch to [PID = %d COUNTER = %d]\n", next->pid, next->counter);
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
        // current->counter--;
        if(current->counter == 0)
            schedule();
        else{
            return;
        }
    }
}