#include <irq.h>
#include <cpu.h>
#include <csrs.h>
#include <plic.h>
#include <aplic.h>
#include <sbi.h>


void irq_cpu_init(void){
    #ifdef APLIC
    aplic_idc_init();
    #endif
}

void irq_enable(unsigned id) {
    if(id < 1024) {
        #ifndef APLIC
        plic_enable_interrupt(get_cpuid(), id, true);
        #endif
        #ifdef APLIC
        aplic_set_ienum(id);
        #endif
    } else if (id == TIMER_IRQ_ID) {
        CSRS(sie, SIE_STIE);
    } else if (id == IPI_IRQ_ID) {
        CSRS(sie, SIE_SSIE);
    }
}

void irq_set_prio(unsigned id, unsigned prio) {
    #ifndef APLIC
    plic_set_prio(id, prio);
    #endif
    #ifdef APLIC
    aplic_set_prio(id, 1);
    #endif
}

void irq_send_ipi(unsigned long target_cpu_mask) {
    sbi_send_ipi(target_cpu_mask, 0);
}

void irq_confg(unsigned id, unsigned prio, unsigned hart_indx, unsigned src_mode){
    #ifndef APLIC
    
    #endif
    #ifdef APLIC
    debug_aplic_config(id, prio, hart_indx, src_mode);
    #endif
}