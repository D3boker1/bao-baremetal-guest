/**
 * @file aplic.c
 * @author Jose Martins <jose.martins@bao-project.org>
 * @author Francisco Marques (fmarques_00@protonmail.com)
 * @brief Implements the aplic domain and IDC's functions
 * @version 0.1
 * @date 2022-09-23
 * 
 * @copyright Copyright (c) Bao Project (www.bao-project.org), 2019-
 * 
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 */
#include <aplic.h>
#include <cpu.h>
#include <irq.h>

spinlock_t print_lock = SPINLOCK_INITVAL;

/*==== APLIC debug define ====*/
//#define APLIC_DEBUG 1

/**==== APLIC Offsets ====*/
#define APLIC_DOMAIN_OFF                (0x0000)
#define APLIC_SOURCECFG_OFF             (0x0004)
#define APLIC_MMSIADDRCFG_OFF           (0x1BC0)
#define APLIC_MMSIADDRCFGH_OFF          (0x1BC4)
#define APLIC_SMSIADDRCFG_OFF           (0x1BC8)
#define APLIC_SMSIADDRCFGH_OFF          (0x1BCC)
#define APLIC_SETIP_OFF                 (0x1C00)
#define APLIC_SETIPNUM_OFF              (0x1CDC)
#define APLIC_IN_CLRIP_OFF              (0x1D00)
#define APLIC_CLRIPNUM_OFF              (0x1DDC)
#define APLIC_SETIE_OFF                 (0x1E00)
#define APLIC_SETIENUM_OFF              (0x1EDC)
#define APLIC_CLRIE_OFF                 (0x1F00)
#define APLIC_CLRIENUM_OFF              (0x1FDC)
#define APLIC_SETIPNUM_LE_OFF           (0x2000)
#define APLIC_SETIPNUM_BE_OFF           (0x2004)
#define APLIC_GENMSI_OFF                (0x3000)
#define APLIC_TARGET_OFF                (0x3004)

/**==== IDC Offsets ====*/
#define APLIC_IDC_IDELIVERY_OFF         (0x00)
#define APLIC_IDC_IFORCE_OFF            (0x04)
#define APLIC_IDC_ITHRESHOLD_OFF        (0x08)
#define APLIC_IDC_TOPI_OFF              (0x18)
#define APLIC_IDC_CLAIMI_OFF            (0x1C)

/**==== APLIC fields and masks defines ====*/
#define APLIC_DOMAINCFG_DM              (1U << 2)
#define APLIC_DOMAINCFG_IE              (1U << 8)
#define APLIC_DOMAINCFG_CTRL_MASK       (0x1FF)

#define SRCCFG_D                        (1U << 10)
#define SRCCFG_SM                       (1U << 0) | (1U << 1) | (1U << 2)
#define DOMAINCFG_DM                    (1U << 2)

#define INTP_IDENTITY                   (16)
#define INTP_IDENTITY_MASK              (0x3FF)

#define APLIC_TARGET_HART_IDX_SHIFT     (18)
#define APLIC_TARGET_IPRIO_MASK         (0xFF)
//#define APLIC_MAX_GEI                 (0)
#define APLIC_TARGET_PRIO_DEFAULT       (1)

#define APLIC_DISABLE_IDELIVERY	        (0)
#define APLIC_ENABLE_IDELIVERY	        (1)
#define APLIC_DISABLE_IFORCE	        (0)
#define APLIC_ENABLE_IFORCE	            (1)
#define APLIC_IDC_ITHRESHOLD_EN_ALL     (0)
#define APLIC_IDC_ITHRESHOLD_DISBL_ALL  (1)

#define APLIC_SRCCFG_DEFAULT            APLIC_SRCCFG_DETACH    

/** Sources State*/
#define IMPLEMENTED                     (1)
#define NOT_IMPLEMENTED                 (0)

/**==== APLIC public data ====*/
volatile struct aplic_global* aplic_domain = (void*) APLIC_BASE;
volatile struct aplic_idc* idc = (void*) APLIC_IDC_BASE; 

/**==== APLIC private data ====*/
size_t APLIC_IMPL_INTERRUPTS;
size_t APLIC_IMPL_INTERRUPTS_REGS;
uint32_t impl_src[APLIC_MAX_INTERRUPTS];

/**==== APLIC private functions ====*/
/**
 * @brief Populate impl_src array with 1's if source i is an active
 * source in this domain
 * 
 * @return size_t total number of implemented interrupts
 */
static size_t aplic_scan_impl_int(void)
{
    size_t count = APLIC_MAX_INTERRUPTS-1;
    for (size_t i = 0; i < APLIC_MAX_INTERRUPTS; i++) {
        aplic_domain->sourcecfg[i] = APLIC_SOURCECFG_SM_DEFAULT;
        impl_src[i] = IMPLEMENTED;
        if (aplic_domain->sourcecfg[i] == APLIC_SOURCECFG_SM_INACTIVE) {
            impl_src[i] = NOT_IMPLEMENTED;
            count -= 1;           
        }
        aplic_domain->sourcecfg[i] = APLIC_SOURCECFG_SM_INACTIVE;
    }
    return count;
}

/**==== APLIC public functions ====*/
/**==== IDC functions ====*/
void aplic_init(void)
{
    aplic_domain->domaincfg = 0;

    /** Clear all pending and enabled bits*/
    for (size_t i = 0; i < APLIC_NUM_CLRIx_REGS; i++) {
        aplic_domain->setie[i] = 0;
        aplic_domain->setip[i] = 0;
    }

    APLIC_IMPL_INTERRUPTS = aplic_scan_impl_int();

    /** Sets the default value of hart index and prio for implemented sources*/
    // for (size_t i = 0; i < APLIC_NUM_TARGET_REGS; i++){
    //     if(impl_src[i] == IMPLEMENTED){
    //         aplic_domain->target[i] = APLIC_TARGET_PRIO_DEFAULT;
    //     }
    // }

    aplic_domain->domaincfg |= APLIC_DOMAINCFG_IE;
}

void aplic_idc_init(void){
    uint32_t idc_index = get_cpuid();

    idc[idc_index].ithreshold = APLIC_IDC_ITHRESHOLD_EN_ALL;  
    idc[idc_index].idelivery = APLIC_ENABLE_IDELIVERY;
    idc[idc_index].iforce = APLIC_DISABLE_IFORCE;
}

inline void aplic_set_domaincfg(uint32_t val)
{
    aplic_domain->domaincfg = val;
}

inline uint32_t aplic_get_domaincfg(void)
{
 return aplic_domain->domaincfg;
}

void aplic_set_sourcecfg(irqid_t int_id, uint32_t val)
{
    uint32_t real_int_id = int_id - 1;
    if(impl_src[real_int_id] == IMPLEMENTED){
        if(!(val & SRCCFG_D) && 
            ((val & SRCCFG_SM) != 2) && ((val & SRCCFG_SM) != 3)){
                aplic_domain->sourcecfg[real_int_id] = val & APLIC_SOURCECFG_SM_MASK;
        }
    }
}

uint32_t aplic_get_sourcecfg(irqid_t int_id)
{
    uint32_t ret = 0;
    uint32_t real_int_id = int_id - 1;
    if(impl_src[real_int_id] == IMPLEMENTED){
        ret = aplic_domain->sourcecfg[real_int_id];
    }
    return ret;
}

void aplic_set_pend_num(irqid_t int_id)
{
    if (impl_src[int_id] == IMPLEMENTED)
    {
        aplic_domain->setipnum = int_id;
    }
}

bool aplic_get_pend(irqid_t int_id)
{
    uint32_t reg_indx = int_id / 32;
    uint32_t intp_to_pend_mask = (1U << (int_id % 32));

    if (impl_src[int_id] == IMPLEMENTED)
    {
        return aplic_domain->setip[reg_indx] & intp_to_pend_mask;
    } else {
        return false;
    }
}

void aplic_set_clripnum(irqid_t int_id)
{
    if (impl_src[int_id] == IMPLEMENTED)
    {
        aplic_domain->clripnum = int_id;
    }
}

bool aplic_get_inclrip(irqid_t int_id)
{
    uint32_t reg_indx = int_id / 32;
    uint32_t intp_to_pend_mask = (1U << (int_id % 32));

    if (impl_src[int_id] == IMPLEMENTED)
    {
        return aplic_domain->in_clrip[reg_indx] & intp_to_pend_mask;
    } else {
        return false;
    }
}

void aplic_set_ienum(irqid_t int_id)
{
    if (impl_src[int_id] == IMPLEMENTED)
    {
        aplic_domain->setienum = int_id;
    }
}

bool aplic_get_ie(irqid_t int_id)
{
    uint32_t reg_indx = int_id / 32;
    uint32_t intp_to_pend_mask = (1U << (int_id % 32));

    if (impl_src[int_id] == IMPLEMENTED)
    {
        return aplic_domain->setie[reg_indx] & intp_to_pend_mask;
    } else {
        return false;
    }
}

void aplic_set_clrienum(irqid_t int_id)
{
    if (impl_src[int_id] == IMPLEMENTED)
    {
        aplic_domain->clrienum = int_id;
    }
}

void aplic_set_target(irqid_t int_id, uint32_t val)
{
    uint32_t real_int_id = int_id - 1;
    uint8_t priority = val & APLIC_TARGET_IPRIO_MASK;
    uint32_t hart_index = (val >> APLIC_TARGET_HART_IDX_SHIFT);
    //uint32_t guest_index = (aplic_domain->target[real_int_id] >> 12) & 0x3F;

    if(impl_src[real_int_id] == IMPLEMENTED){
        /** Evaluate in what delivery mode the domain is configured*/
        /** Direct Mode*/
        if(((aplic_domain->domaincfg & APLIC_DOMAINCFG_CTRL_MASK) & DOMAINCFG_DM) == 0){
            /** Checks priority and hart index range */
            if((priority > 0) && (priority <= APLIC_TARGET_IPRIO_MASK) && 
               (hart_index < APLIC_PLAT_IDC_NUM)){
                aplic_domain->target[real_int_id] = val;
            }
        }
        /** MSI Mode*/
        else{ 
            // if(guest_index >= 0 && guest_index <= APLIC_MAX_GEI){
            //     aplic_domain->target[real_int_id] = val;
            // }
        }
    }
}

uint32_t aplic_get_target(irqid_t int_id)
{
    uint32_t real_int_id = int_id - 1;
    uint32_t ret = 0;
    if(impl_src[real_int_id] == IMPLEMENTED){
        ret = aplic_domain->target[real_int_id];
    }
    return ret;
}

/**==== IDC functions ====*/
void aplic_idc_set_idelivery(idcid_t idc_id, bool en)
{
    if(idc_id < APLIC_PLAT_IDC_NUM) {
        if (en){
            idc[idc_id].idelivery = APLIC_ENABLE_IDELIVERY;
        }else{
            idc[idc_id].idelivery = APLIC_DISABLE_IDELIVERY;
        }
    }
}

bool aplic_idc_get_idelivery(idcid_t idc_id)
{
    if(idc_id < APLIC_PLAT_IDC_NUM) {
        return idc[idc_id].idelivery && APLIC_ENABLE_IDELIVERY;
    } else{
        return false;
    }
}

void aplic_idc_set_iforce(idcid_t idc_id, bool en)
{
    if(idc_id < APLIC_PLAT_IDC_NUM) {
        if (en){
            idc[idc_id].iforce = APLIC_ENABLE_IFORCE;
        }else{
            idc[idc_id].iforce = APLIC_DISABLE_IFORCE;
        }
    }
}

bool aplic_idc_get_iforce(idcid_t idc_id)
{
    if(idc_id < APLIC_PLAT_IDC_NUM) {
        return idc[idc_id].iforce && APLIC_ENABLE_IFORCE;
    } else{
        return false;
    }   
}

void aplic_idc_set_ithreshold(idcid_t idc_id, uint32_t new_th)
{
    if(idc_id < APLIC_PLAT_IDC_NUM) {
        if(new_th <= APLIC_TARGET_IPRIO_MASK){
            idc[idc_id].ithreshold = new_th;
        }
    }
}

uint32_t aplic_idc_get_ithreshold(idcid_t idc_id)
{
    uint32_t ret = 0;
    if(idc_id < APLIC_PLAT_IDC_NUM) {
        ret = idc[idc_id].ithreshold;
    }
    return ret;
}

uint32_t aplic_idc_get_topi(idcid_t idc_id)
{
    uint32_t ret = 0;
    if(idc_id < APLIC_PLAT_IDC_NUM) {
        ret = idc[idc_id].topi;
    }
    return ret;
}

uint32_t aplic_idc_get_claimi(idcid_t idc_id)
{
    uint32_t ret = 0;
    if(idc_id < APLIC_PLAT_IDC_NUM) {
        ret = idc[idc_id].claimi;
    }
    return ret;
}

void aplic_handle(void){
    uint32_t ret = 0;
    uint32_t intp_identity;
    idcid_t idc_id = get_cpuid();
    
    intp_identity = (idc[idc_id].claimi >> INTP_IDENTITY) & INTP_IDENTITY_MASK;
    if(intp_identity > 0){
        irq_handle(intp_identity);
    }
}

/**==== Debug functions ====*/
int debug_aplic_init(void){
    aplic_init();
    spin_lock(&print_lock);
    printf("Number of implemented interrupts: %d\r\n", APLIC_IMPL_INTERRUPTS);
    spin_unlock(&print_lock);
    return 0;
}

int debug_aplic_idc_init(void){
    uint32_t idc_index = get_cpuid();

    aplic_idc_init();

    if( idc[idc_index].ithreshold == APLIC_IDC_ITHRESHOLD_EN_ALL && 
        idc[idc_index].idelivery == APLIC_ENABLE_IDELIVERY &&
        idc[idc_index].iforce == APLIC_DISABLE_IFORCE){
        spin_lock(&print_lock);
        printf("IDC[%d] is good to go\r\n", idc_index);
        spin_unlock(&print_lock);
    }
    return 0;
}

int debug_aplic_check_addrs(void){
    if(&aplic_domain->domaincfg != APLIC_BASE+APLIC_DOMAIN_OFF){
        printf("domaincfg @ %x \r\n", &aplic_domain->domaincfg);
        return -1;
    }

    for(size_t i = 0; i < APLIC_NUM_SRCCFG_REGS; i++){
        if(&aplic_domain->sourcecfg[i] != APLIC_BASE+APLIC_SOURCECFG_OFF+(i*0x4)){
            printf("sourcecfg[%d] @ %x \r\n", i, &aplic_domain->sourcecfg[i]);
            return -1;
        }   
    }

    for(size_t i = 0; i < (APLIC_MAX_INTERRUPTS/32); i++){
        if(&aplic_domain->setip[i] != APLIC_BASE+APLIC_SETIP_OFF+(i*0x4)){
            printf("setip[%d] @ %x \r\n", i, &aplic_domain->setip[i]);
            return -1;
        }   
    }
    if(&aplic_domain->setipnum != APLIC_BASE+APLIC_SETIPNUM_OFF){
        printf("setipnum @ %x \r\n", &aplic_domain->setipnum);
        return -1;
    }
    for(size_t i = 0; i < (APLIC_MAX_INTERRUPTS/32); i++){
        if(&aplic_domain->in_clrip[i] != APLIC_BASE+APLIC_IN_CLRIP_OFF+(i*0x4)){
            printf("in_clrip[%d] @ %x \r\n", i, &aplic_domain->in_clrip[i]);
            return -1;
        }   
    }
    if(&aplic_domain->clripnum != APLIC_BASE+APLIC_CLRIPNUM_OFF){
        printf("clripnum @ %x \r\n", &aplic_domain->clripnum);
        return -1;
    }

    for(size_t i = 0; i < (APLIC_MAX_INTERRUPTS/32); i++){
        if(&aplic_domain->setie[i] != APLIC_BASE+APLIC_SETIE_OFF+(i*0x4)){
            printf("setie[%d] @ %x \r\n", i, &aplic_domain->setie[i]);
            return -1;
        }   
    }
    if(&aplic_domain->setienum != APLIC_BASE+APLIC_SETIENUM_OFF){
        printf("setienum @ %x \r\n", &aplic_domain->setienum);
        return -1;
    }
    for(size_t i = 0; i < (APLIC_MAX_INTERRUPTS/32); i++){
        if(&aplic_domain->clrie[i] != APLIC_BASE+APLIC_CLRIE_OFF+(i*0x4)){
            printf("clrie[%d] @ %x \r\n", i, &aplic_domain->clrie[i]);
            return -1;
        }   
    }
    if(&aplic_domain->clrienum != APLIC_BASE+APLIC_CLRIENUM_OFF){
        printf("clrienum @ %x \r\n", &aplic_domain->clrienum);
        return -1;
    }

    if(&aplic_domain->setipnum_le != APLIC_BASE+APLIC_SETIPNUM_LE_OFF){
        printf("setipnum_le @ %x \r\n", &aplic_domain->setipnum_le);
        return -1;
    }
    if(&aplic_domain->setipnum_be != APLIC_BASE+APLIC_SETIPNUM_BE_OFF){
        printf("setipnum_be @ %x \r\n", &aplic_domain->setipnum_be);
        return -1;
    }

    if(&aplic_domain->genmsi != APLIC_BASE+APLIC_GENMSI_OFF){
        printf("genmsi @ %x \r\n", &aplic_domain->genmsi);
        return -1;
    }

    for(size_t i = 0; i < APLIC_MAX_INTERRUPTS; i++){
        if(&aplic_domain->target[i] != APLIC_BASE+APLIC_TARGET_OFF+(i*0x4)){
            printf("target[%d] @ %x \r\n", i, &aplic_domain->target[i]);
            return -1;
        }   
    }

    for(size_t i = 0; i < APLIC_PLAT_IDC_NUM; i++){
        uint32_t j = 0;
        if(i < APLIC_PLAT_IDC_NUM){
            if(&idc->idelivery != APLIC_IDC_BASE+APLIC_IDC_IDELIVERY_OFF+(j*0x20)){
                printf("IDC %d idelivery @ %x \r\n", i, &idc->idelivery);
                return -1;
            }
            if(&idc->iforce != APLIC_IDC_BASE+APLIC_IDC_IFORCE_OFF+(j*0x20)){
                printf("IDC %d iforce @ %x \r\n", i, &idc->iforce);
                return -1;
            } 
            if(&idc->ithreshold != APLIC_IDC_BASE+APLIC_IDC_ITHRESHOLD_OFF+(j*0x20)){
                printf("IDC %d ithreshold @ %x \r\n", i, &idc->ithreshold);
                return -1;
            }
            if(&idc->topi != APLIC_IDC_BASE+APLIC_IDC_TOPI_OFF+(j*0x20)){
                printf("IDC %d topi @ %x \r\n", i, &idc->topi);
                return -1;
            }
            if(&idc->claimi != APLIC_IDC_BASE+APLIC_IDC_CLAIMI_OFF+(j*0x20)){
                printf("IDC %d claimi @ %x \r\n", i, &idc->claimi);
                return -1;
            }
            j++;
        }
    }

    return 0;
}

int debug_aplic_handle(void){
    uint32_t intp_identity;
    uint32_t intp_priority;
    idcid_t idc_id = get_cpuid();
    uint32_t claimi_aux = aplic_idc_get_claimi(idc_id);
    aplic_handle();

    if(claimi_aux == 0){
        spin_lock(&print_lock);
        printf("Spurious External Interrupt\r\n");
        spin_unlock(&print_lock);
        return -1;
    }

    intp_priority = claimi_aux & APLIC_TARGET_IPRIO_MASK;
    intp_identity = (claimi_aux >> INTP_IDENTITY) & INTP_IDENTITY_MASK;

    spin_lock(&print_lock);
    printf("IDC[%d] successfully catch interrupt %d with priority %d\r\n",
                            idc_id, intp_identity, intp_priority);
    spin_unlock(&print_lock);

    return 0;
}

int debug_aplic_config(unsigned id, unsigned prio, unsigned hart_indx, unsigned src_mode){
    uint32_t aux = 0;

    aplic_set_sourcecfg(id, src_mode);
    aplic_set_target(id, (hart_indx << APLIC_TARGET_HART_IDX_SHIFT) | (prio));
    /** Needs to be after setting the source mode. */
    aplic_set_ienum(id);

    aux = aplic_get_sourcecfg(id);
    if(aux != src_mode){
        spin_lock(&print_lock);
        printf("ERROR: set sourcecfg[%d].SM = %d\r\n", id, aplic_domain->sourcecfg[id-1]);
        spin_unlock(&print_lock);
        return -1;
    }

    aux = aplic_get_target(id);
    if(aux != ((hart_indx << APLIC_TARGET_HART_IDX_SHIFT) | (prio))){
        spin_lock(&print_lock);
        printf("ERROR: target[%d]: hart_indx = %d; prio = %d\r\n", id, hart_indx, prio);
        spin_unlock(&print_lock);
        return -1;
    }

    if(!aplic_get_ie(id)){
        spin_lock(&print_lock);
        printf("ERROR: set IRQ[%d] enable\r\n", id);
        spin_unlock(&print_lock);
        return -1;
    }

    return 0;
}