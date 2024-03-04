#include "printk.h"
#include "types.h"
#include "defs.h"


void read_sstatus(void){
    uint64 ss=csr_read(sstatus);
    printk("sstatus:0x%lx\n",ss);
    return;
}

void write_sscratch(void){
    uint64 ss=csr_read(sscratch);
    printk("sscratch raw:0x%lx\n",ss);
    csr_write(sscratch,~(0x0UL));
    ss=csr_read(sscratch);
    printk("sscratch now:0x%lx\n",ss);
}
