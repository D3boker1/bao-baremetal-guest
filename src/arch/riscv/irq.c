#include <irq.h>
#include <cpu.h>
#include <csrs.h>
#ifdef PLIC
#include <plic.h>
#elif APLIC
#include <aplic.h>
#endif
#include <sbi.h>

void irq_cpu_init(void){
    #ifdef APLIC
    aplic_idc_init();
    #endif
}

void irq_enable(unsigned id, unsigned cpu_id) {
    if(id < 1024) {
        #ifdef PLIC
        plic_enable_interrupt(get_cpuid(), id, true);
        #elif APLIC
        aplic_set_sourcecfg(id, APLIC_SOURCECFG_SM_EDGE_RISE);
        aplic_set_target_hart(id, cpu_id);
        aplic_set_enbl(id);
        #endif
    } else if (id == TIMER_IRQ_ID) {
        csrs_sie_set(SIE_STIE);
    } else if (id == IPI_IRQ_ID) {
        csrs_sie_set(SIE_SSIE);
    }
}

void irq_set_prio(unsigned id, unsigned prio) {
    #ifdef PLIC
    plic_set_prio(id, prio);
    #elif APLIC
    aplic_set_target_prio(id, prio);
    #endif
}

void irq_send_ipi(unsigned long target_cpu_mask) {
    sbi_send_ipi(target_cpu_mask, 0);
}
