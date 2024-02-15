#include <core.h>
#include <csrs.h>
#include <irq.h>
#ifdef PLIC
#include <plic.h>
#elif APLIC
#include <aplic.h>
#elif IMSIC
#include <aplic.h>
#include <imsic.h>
#endif

#ifdef IMSIC

    static bool is_external_core_level_irqc(unsigned long cause){
        switch(cause) {
            case IRQ_S_EXT:
            case IRQ_U_EXT:
                return true;
            default:
                return false;
        }
    }
    
    static void core_level_irqc_handler(void)
    {
        unsigned long stopi;

        if ((stopi = CSRR(CSR_STOPI))) {
            CSRC(sie, SIE_SEIE);
            stopi = stopi >> TOPI_IID_SHIFT;
            if(is_external_core_level_irqc(stopi)) {
                imsic_handle();
            } else {
                unsigned long id = stopi + 1024;
                irq_handle(id);   
            }
            CSRS(sie, SIE_SEIE);
        }
    }

#else

    static bool is_external_plat_level_irqc(unsigned long cause) {
        switch(cause) {
            case SCAUSE_CODE_SEI:
            case SCAUSE_CODE_UEI:
                return true;
            default:
                return false;
        }
    }

    static void plat_level_irqc_handler(void) 
    {
        unsigned long scause = csrs_scause_read();

        if(is_external_plat_level_irqc(scause)) {
            #ifdef PLIC
                plic_handle();
            #elif APLIC
                aplic_handle();
            #endif
        } else {
            size_t msb = sizeof(unsigned long) * 8 - 1;
            unsigned long id = (scause & ~(1ull << msb)) + 1024;
            irq_handle(id);
            if(id == IPI_IRQ_ID) {
                csrs_sip_clear(SIP_SSIE);
            }
        }
    }

#endif
__attribute__((interrupt("supervisor"), aligned(4)))
void exception_handler()
{
    #ifdef IMSIC
        core_level_irqc_handler();
    #else
        plat_level_irqc_handler();
    #endif    

}
