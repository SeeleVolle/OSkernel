 // arch/riscv/kernel/vm.c
#include "proc.h"
#include "mm.h"
#include "defs.h"
#include "string.h"
#include "printk.h"
/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
//4 KB的顶级页表
extern char _stext[];
extern char _srodata[];
extern char _sdata[];

//512 * 8 B = 4KB
unsigned long early_pgtbl[512] __attribute__((__aligned__(0x1000)));

void Empty(void){                          
    // printk("Really Empty!\n");
}

void setup_vm(void) {
    /* 
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表 
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。 
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */
    memset(early_pgtbl, 0, PGSIZE);
    uint64 va, pa; //1 GB = 2^30 B
    va = PHY_START;
    pa = PHY_START; //0x80000000 
    uint64 vpn = (va >> 30) & 0x1ff; //38-30
    uint64 ppn = (pa >> 12); 
    // printk("vpn :%ld\n", vpn);
    // early_pgtbl[vpn] = (ppn << 10 | 0xf);
    // early_pgtbl[vpn] = 536870927; // V R W X set to 1
    // printk("tbl_vpn: %lx\n",  early_pgtbl[vpn]);

    va = VM_START;
    vpn = (va >> 30) & 0x1ff;
    early_pgtbl[vpn] = (ppn << 10 | 0xf);
    // printk("vpn :%ld\n", vpn);
    // printk("vpn: %lx\n", vpn);

    // printk("end_list\n");
    // Empty();
    return;
}


void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 perm);

unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

void setup_vm_final(void) {
    memset(swapper_pg_dir, 0, PGSIZE);

    // No OpenSBI mapping required
    uint64 pa, va;
    // mapping kernel text X|-|R|V
    //X|-|R|V: 1011 -> 11
    pa = PHY_START + OPENSBI_SIZE;
    va = VM_START + OPENSBI_SIZE;
    uint64 text_size = (uint64)_srodata - (uint64)_stext;
    // printk("Hello1\n");
    create_mapping(swapper_pg_dir, va, pa, text_size, 11);

    // mapping kernel rodata -|-|R|V
    // -|-|R|V: 0011 -> 3
    uint64 rodata_size = (uint64) _sdata - (uint64)_srodata;
    pa += text_size;
    va += text_size;
    create_mapping(swapper_pg_dir, va, pa, rodata_size, 3);
    
    // mapping other memory -|W|R|V
    //-|W|R|V: 0111 -> 7
    uint64 memory_size = PHY_SIZE - ((uint64)_sdata - (uint64)_stext);
    pa += rodata_size;
    va += rodata_size;
    create_mapping(swapper_pg_dir, va, pa, memory_size, 7);
    
    // set satp with swapper_pg_dir
    
    //pa = va - PA2VA_OFFSET
    uint64 swap_dir_pa = (uint64)swapper_pg_dir - (uint64)PA2VA_OFFSET;
    __asm__ volatile(
        "li t0, 8\n"
        "slli t0, t0, 60\n"
        "mv t1, %[addr]\n"
        "srli t1, t1, 12\n"
        "or t0, t0, t1\n"
        "csrw satp, t0"
        : 
        : [addr] "r" (swap_dir_pa)
        : "memory"
    );

    // flush TLB
    asm volatile("sfence.vma zero, zero");
  
    // flush icache
    asm volatile("fence.i");
    return;
}


/* 创建多级页表映射关系 */
/* 不要修改该接口的参数和返回值 */
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 perm) {
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小
    perm 为映射的读写权限

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */
    uint64 * tbl[3] = {NULL, NULL, NULL};
    uint64 VPN[3] = {0,0,0};
    uint64 end = va + sz;
    
    while(va < end)
    {   
        //Process the tbl[2]
        tbl[2] = pgtbl;
        VPN[2] = (va >> 30) & 0x1ff; //38-30
        if(!(tbl[2][VPN[2]]& 0x1)){
            uint64 * page = (uint64 *)kalloc();
            tbl[2][VPN[2]] = (uint64)(((((uint64)page - (uint64)PA2VA_OFFSET) >> 12) << 10 ) | 1); //PA >> 12 = PPN
        }
        //Process the tbl[1]
        // tbl[1] = (uint64 *)(tbl[2][VPN[2]] >> 10 << 12);
        tbl[1] = (uint64 *)(((tbl[2][VPN[2]] >> 10) << 12) + (uint64)PA2VA_OFFSET);

        VPN[1] = (va >> 21) & 0x1ff; //29-21
        if(!(tbl[1][VPN[1]] & 0x1)){
            uint64 * page = (uint64 *)kalloc();
            tbl[1][VPN[1]] = (uint64)(((((uint64)page - (uint64)PA2VA_OFFSET) >> 12) << 10 ) | 1); //PA >> 12 = PPN
        }
        //Process the tbl[0]
        // tbl[0] = (uint64 *)((tbl[1][VPN[1]] >> 10) << 12); 
        tbl[0] = (uint64 *)(((tbl[1][VPN[1]] >> 10) << 12) + (uint64)PA2VA_OFFSET); 
        
        VPN[0] = (va >> 12) & 0x1ff;//20-12
        tbl[0][VPN[0]] = ((pa >> 12) << 10) | (perm & 0xf);

        va += PGSIZE;
        pa += PGSIZE;
    }
}