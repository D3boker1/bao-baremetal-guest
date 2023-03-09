#include <imsic.h>
#include <cpu.h>
#include <irq.h>
#include <stdio.h>

#define IMSICS_BASE 0x28000000
#define EEID 16

#define CSR_SISELECT			0x150
#define CSR_SIREG			    0x151
#define CSR_STOPEI			    0x15c

#define IMSIC_EIDELIVERY		0x70
#define IMSIC_EITHRESHOLD		0x72
#define IMSIC_EIP		        0x80
#define IMSIC_EIE		        0xC0

volatile struct imsic_global* imsic_s_lvl = (void*) IMSICS_BASE;


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
    // for(int intp_id = 0; intp_id < NR_INTP; intp_id++){
    //     CSRW(CSR_SISELECT, IMSIC_EIE);
    //     prev_val = CSRR(CSR_SIREG);
    //     CSRW(c, prev_val | (1 << intp_id));
    // }
    CSRW(CSR_SISELECT, IMSIC_EIE);
    CSRW(CSR_SIREG, 0xffffffffffffffff);
}

/** !!!nao esta complaint com a spec!!!
 *  O hart destino tem de ser escolhido usando o target reg
 *  Para ja ele esta fixo em 0 para tds as intp
*/
void imsic_send_msi(unsigned long target_cpu_mask, uint32_t data){
    imsic_s_lvl[target_cpu_mask].seteipnum_le = data;
}

void imsic_handle(void){
    uint32_t intp_identity;
    
    intp_identity = (CSRR(CSR_STOPEI) >> EEID);
    printf("out: intp_identity = %d\n", intp_identity);
    while (intp_identity != 0){
        if(intp_identity > 0){
            printf("in: intp_identity = %d\n", intp_identity);
            irq_handle(intp_identity);
        }
        intp_identity = CSRR(CSR_STOPEI) >> EEID;
        CSRW(CSR_STOPEI, 0);
    };
    
}