#include <core.h>
#include <cpu.h>
#include <page_tables.h>
#include <irq.h>
#include <plic.h>
#include <aplic.h>
#include <sbi.h>
#include <csrs.h>

extern void _start();

__attribute__((weak))
void arch_init(){
#ifndef SINGLE_CORE
    unsigned long hart_id = get_cpuid();
    struct sbiret ret = (struct sbiret){ .error = SBI_SUCCESS };
    size_t i = 0;    
    do {
        if(i == hart_id) continue;
        ret = sbi_hart_start(i, (unsigned long) &_start, 0);
    } while(i++, ret.error == SBI_SUCCESS);
#endif
    CSRS(sie, SIE_SEIE);
    CSRS(sstatus, SSTATUS_SIE);
}
