#include <core.h>
#include <csrs.h>
#include <plic.h>
#include <aplic.h>
#include <irq.h>
#include <imsic.h>

static bool is_external(unsigned long cause) {
    #ifdef IMSIC
    switch(cause) {
        case IRQ_S_EXT:
        case IRQ_U_EXT:
            return true;
        default:
            return false;
    }
    #else
    switch(cause) {
        case SCAUSE_CODE_SEI:
        case SCAUSE_CODE_UEI:
            return true;
        default:
            return false;
    }
    #endif
}

__attribute__((interrupt("supervisor"), aligned(4)))
void exception_handler(){
    #ifdef IMSIC
    unsigned long stopi;

	while ((stopi = CSRR(CSR_STOPI))) {
        CSRC(sie, SIE_SEIE);
		stopi = stopi >> TOPI_IID_SHIFT;
        if(is_external(stopi)) {
            imsic_handle();
        } else {
            unsigned long id = stopi + 1024;
            irq_handle(id);   
        }
        CSRS(sie, SIE_SEIE);
	}
    #else
    unsigned long scause = CSRR(scause);
    if(is_external(scause)) {
        #ifndef APLIC
        plic_handle();
        #else
        aplic_handle();
        #endif
    } else {
       size_t msb = sizeof(unsigned long) * 8 - 1;
       unsigned long id = (scause & ~(1ull << msb)) + 1024;
       irq_handle(id);
       if(id == IPI_IRQ_ID) {
           CSRC(sip, SIP_SSIE);
       }
    }
    #endif
}
