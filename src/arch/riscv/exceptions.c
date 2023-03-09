#include <core.h>
#include <csrs.h>
#include <plic.h>
#include <aplic.h>
#include <irq.h>
#include <imsic.h>

static bool is_external(unsigned long cause) {
    switch(cause) {
        case SCAUSE_CODE_SEI:
        case SCAUSE_CODE_UEI:
            return true;
        default:
            return false;
    }
}

__attribute__((interrupt("supervisor"), aligned(4)))
void exception_handler(){
    
    unsigned long scause = CSRR(scause);
    if(is_external(scause)) {
        #ifndef APLIC
        plic_handle();
        #endif
        #ifdef APLIC
        // aplic_handle();
        imsic_handle();
        #endif
    } else {
       size_t msb = sizeof(unsigned long) * 8 - 1;
       unsigned long id = (scause & ~(1ull << msb)) + 1024;
       irq_handle(id);
       if(id == IPI_IRQ_ID) {
           CSRC(sip, SIP_SSIE);
       }
    }
}
