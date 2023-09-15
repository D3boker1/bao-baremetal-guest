#include <imsic.h>
#include <cpu.h>
#include <irq.h>
#include <stdio.h>


#define EEID 16

#define CSR_SISELECT			0x150
#define CSR_SIREG			    0x151
#define CSR_STOPEI			    0x15c

#define IMSIC_EIDELIVERY		0x70
#define IMSIC_EITHRESHOLD		0x72
#define IMSIC_EIP		        0x80
#define IMSIC_EIE		        0xC0

volatile struct imsic_global* imsic = (void*) IMSICS_BASE;


void imsic_init(void){
    uint32_t prev_val;
    uint32_t hold;

    /** Enable interrupt delivery */
    CSRW(CSR_SISELECT, IMSIC_EIDELIVERY);
    CSRW(CSR_SIREG, 1);
    hold = CSRR(CSR_SIREG);

    /** Every intp is triggrable */
    CSRW(CSR_SISELECT, IMSIC_EITHRESHOLD);
    CSRW(CSR_SIREG, 0);

    /** Enable all interrupts */
    CSRW(CSR_SISELECT, IMSIC_EIE);
    CSRW(CSR_SIREG, 0xffffffffffffffff);
}

void imsic_send_msi(unsigned long target_cpu_mask, uint32_t data){
    imsic[target_cpu_mask].s_file.seteipnum_le = data;
}

void imsic_handle(void){
    uint32_t intp_identity;
    
    while (intp_identity = (CSRR(CSR_STOPEI) >> EEID)){
        // printf("intp_identity = %d\r\n", intp_identity);
        CSRW(CSR_STOPEI, 0);
        irq_handle(intp_identity);
    };
    
}