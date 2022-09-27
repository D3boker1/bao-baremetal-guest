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

spinlock_t print_lock = SPINLOCK_INITVAL;

/*==== APLIC debug define ====*/
//#define APLIC_DEBUG 1

/**==== APLIC privite defines ====*/
#define APLIC_DOMAINCFG_DM (1U << 2)
#define APLIC_DOMAINCFG_IE (1U << 8)
#define APLIC_DOMAINCFG_CTRL_MASK (0x1FF)

#define SRCCFG_D (1U << 10)
#define SRCCFG_SM (1U << 0) | (1U << 1) | (1U << 2)
#define DOMAINCFG_DM (1U << 2)

#define INTP_IDENTITY (16)
#define INTP_IDENTITY_MASK (0x3FF)

#define APLIC_TARGET_IPRIO_MASK (0xFF)
//#define APLIC_MAX_GEI 0
#define APLIC_TARGET_DEFAULT 1

#define APLIC_DISABLE_IDELIVERY		0
#define APLIC_ENABLE_IDELIVERY		1
#define APLIC_DISABLE_IFORCE		0
#define APLIC_ENABLE_IFORCE	    	1

#define APLIC_SRCCFG_INACT  0
#define APLIC_SRCCFG_DETACH 1
#define APLIC_SRCCFG_EDGE1  4
#define APLIC_SRCCFG_EDGE0  5
#define APLIC_SRCCFG_LEVEL1 6
#define APLIC_SRCCFG_LEVEL0 7

#define APLIC_SRCCFG_DEFAULT APLIC_SRCCFG_DETACH    

/** Sources State*/
#define IMPLEMENTED 1
#define NOT_IMPLEMENTED 0

/**==== APLIC public data ====*/
volatile struct aplic_global* aplic_domain = (void*) APLIC_BASE;
volatile struct aplic_idc* idc = (void*) APLIC_IDC_BASE; 

/**==== APLIC privite data ====*/
size_t APLIC_IMPL_INTERRUPTS;
size_t APLIC_IMPL_INTERRUPTS_REGS;
uint32_t impl_src[APLIC_MAX_INTERRUPTS];

/**==== APLIC privite functions ====*/

/**
 * @brief Populate impl_src array with 1's if source i is an active
 * source in this domain
 * 
 * @return size_t total number of implemented interrupts
 */
static size_t aplic_scan_impl_int(void)
{
    size_t count = APLIC_MAX_INTERRUPTS-1;
    for (size_t i = 1; i < APLIC_MAX_INTERRUPTS; i++) {
        aplic_domain->setienum = i;
        impl_src[i] = IMPLEMENTED;
        if (aplic_domain->setie[i/32] == 0) {
            impl_src[i] = NOT_IMPLEMENTED;
            count -= 1;           
        }
        aplic_domain->setie[i/32] = 0;
    }
    return count;
}

/**==== APLIC public functions ====*/
void aplic_init(void)
{
    aplic_domain->domaincfg = 0;

    /** Clear all pending and enabled bits*/
    for (size_t i = 0; i < APLIC_NUM_CLRIx_REGS; i++) {
        aplic_domain->in_clrip[i] = 0;
        aplic_domain->clrie[i] = 0;
    }

    /** Sets the defaults configurations to all interrupts*/
    for (size_t i = 1; i < APLIC_MAX_INTERRUPTS; i++) { // APLIC_MAX_INTP
        aplic_domain->sourcecfg[i] = APLIC_SRCCFG_DEFAULT;
        #ifdef APLIC_DEBUG
        if(aplic_domain->sourcecfg[i] == APLIC_SRCCFG_DEFAULT){
            spin_lock(&print_lock);
            printf("SOURCECFG[%d] = %x\r\n", i, aplic_domain->sourcecfg[i]);
            spin_unlock(&print_lock);
        }
        #endif
    }

    APLIC_IMPL_INTERRUPTS = aplic_scan_impl_int();

    /** Sets the default value of hart index and prio for implemented sources*/
    for (size_t i = 1; i < APLIC_MAX_INTERRUPTS; i++){
        if(impl_src[i] == IMPLEMENTED){
            aplic_domain->target[i] = APLIC_TARGET_DEFAULT;
            #ifdef APLIC_DEBUG
            if(aplic_domain->target[i] == APLIC_TARGET_DEFAULT){
                spin_lock(&print_lock);
                printf("TARGET[%d] = %x\r\n", i, aplic_domain->target[i]);
                spin_unlock(&print_lock);
            }
            #endif
        }
    }

    aplic_domain->domaincfg |= APLIC_DOMAINCFG_IE;
}

void aplic_idc_init(void){
    uint32_t idc_index = aplic_plat_idc_to_id((struct aplic_idc_id){get_cpuid(), PRIV_S});
    idc[idc_index].ithreshold = 0;
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
    if(impl_src[int_id] == IMPLEMENTED){
        if(!(val && SRCCFG_D) && 
            ((val && SRCCFG_SM) != 2) && ((val && SRCCFG_SM) != 3)){
                aplic_domain->sourcecfg[int_id] = val;
        }
    }
}

uint32_t aplic_get_sourcecfg(irqid_t int_id)
{
    uint32_t ret = 0;
    if(impl_src[int_id] == IMPLEMENTED){
        ret = aplic_domain->sourcecfg[int_id];
    }
    return ret;
}

void aplic_set_src_mode(irqid_t int_id, uint32_t new_sm)
{
    uint32_t val = aplic_domain->sourcecfg[int_id];
    new_sm &= SRCCFG_SM; 
    val &= ~SRCCFG_SM;
    val |= new_sm;
    aplic_domain->sourcecfg[int_id] = val;
}

void aplic_set_pend(irqid_t int_id)
{
    uint32_t reg_indx = int_id / 32;
    uint32_t intp_to_pend_mask = (1U << (int_id % 32));

    if(impl_src[int_id] == IMPLEMENTED){
        aplic_domain->setip[reg_indx] |= intp_to_pend_mask; 
    }
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

void aplic_set_inclrip(irqid_t int_id)
{
    uint32_t reg_indx = int_id / 32;
    uint32_t intp_to_pend_mask = (1U << (int_id % 32));

    if(impl_src[int_id] == IMPLEMENTED){
        aplic_domain->setip[reg_indx] |= intp_to_pend_mask;
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

void aplic_set_ie(irqid_t int_id)
{
    uint32_t reg_indx = int_id / 32;
    uint32_t intp_to_pend_mask = (1U << (int_id % 32));

    if(impl_src[int_id] == IMPLEMENTED){
        aplic_domain->setie[reg_indx] |= intp_to_pend_mask; 
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

void aplic_set_clrie(irqid_t int_id)
{
    uint32_t reg_indx = int_id / 32;
    uint32_t intp_to_pend_mask = (1U << (int_id % 32));

    if(impl_src[int_id] == IMPLEMENTED){
        aplic_domain->clrie[reg_indx] |= intp_to_pend_mask; 
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
    uint8_t priority = val & APLIC_TARGET_IPRIO_MASK;
    uint32_t hart_index = (val >> 18);
    //uint32_t guest_index = (aplic_domain->target[int_id] >> 12) & 0x3F;

    if(impl_src[int_id] == IMPLEMENTED){
        /** Evaluate in what delivery mode the domain is configured*/
        /** Direct Mode*/
        if(((aplic_domain->domaincfg & APLIC_DOMAINCFG_CTRL_MASK) & DOMAINCFG_DM) == 0){
            /** Checks priority and hart index range */
            if(priority > 0 && priority <= APLIC_TARGET_IPRIO_MASK && 
               aplic_idc_valid((idcid_t)hart_index)){
                aplic_domain->target[int_id] = val;
            }
        }
        /** MSI Mode*/
        else{ 
            // if(guest_index >= 0 && guest_index <= APLIC_MAX_GEI){
            //     aplic_domain->target[int_id] = val;
            // }
        }
    }
}

uint32_t aplic_get_target(irqid_t int_id)
{
    uint32_t ret = 0;
    if(impl_src[int_id] == IMPLEMENTED){
        ret = aplic_domain->target[int_id];
    }
    return ret;
}

inline void aplic_set_prio(irqid_t int_id, uint8_t val)
{
    uint32_t aux = aplic_domain->target[int_id] & ~APLIC_TARGET_IPRIO_MASK | val;
    // POSSO???
    aplic_set_target(int_id, aux);
}

void aplic_idc_set_idelivery(idcid_t idc_id, bool en)
{
    if(aplic_idc_valid(idc_id)) {
        if (en){
            idc[idc_id].idelivery = APLIC_ENABLE_IDELIVERY;
        }else{
            idc[idc_id].idelivery = APLIC_DISABLE_IDELIVERY;
        }
    }
}

bool aplic_idc_get_idelivery(idcid_t idc_id)
{
    if(aplic_idc_valid(idc_id)) {
        return idc[idc_id].idelivery && APLIC_ENABLE_IDELIVERY;
    } else{
        return false;
    }
}

void aplic_idc_set_iforce(idcid_t idc_id, bool en)
{
    if(aplic_idc_valid(idc_id)) {
        if (en){
            idc[idc_id].iforce = APLIC_ENABLE_IFORCE;
        }else{
            idc[idc_id].iforce = APLIC_DISABLE_IFORCE;
        }
    }
}

bool aplic_idc_get_iforce(idcid_t idc_id)
{
    if(aplic_idc_valid(idc_id)) {
        return idc[idc_id].iforce && APLIC_ENABLE_IFORCE;
    } else{
        return false;
    }   
}

void aplic_idc_set_ithreshold(idcid_t idc_id, uint32_t new_th)
{
    if(aplic_idc_valid(idc_id)) {
        if(new_th <= APLIC_TARGET_IPRIO_MASK){
            idc[idc_id].ithreshold = new_th;
        }
    }
}

uint32_t aplic_idc_get_ithreshold(idcid_t idc_id)
{
    uint32_t ret = 0;
    if(aplic_idc_valid(idc_id)) {
        ret = idc[idc_id].ithreshold;
    }
    return ret;
}

uint32_t aplic_idc_get_topi(idcid_t idc_id)
{
    uint32_t ret = 0;
    if(aplic_idc_valid(idc_id)) {
        ret = idc[idc_id].topi;
    }
    return ret;
}

uint32_t aplic_idc_get_claimi(idcid_t idc_id)
{
    uint32_t ret = 0;
    if(aplic_idc_valid(idc_id)) {
        ret = idc[idc_id].claimi;
    }
    return ret;
}

bool aplic_idc_valid(idcid_t idc_id) {
    struct aplic_idc_id idc = aplic_plat_id_to_idc(idc_id);
    return (idc_id < APLIC_PLAT_IDC_NUM) && (idc.mode <= PRIV_S);
}

uint32_t aplic_handle(idcid_t idc_id){
    uint32_t ret = 0;
    if(aplic_idc_valid(idc_id)) {
        ret = idc[idc_id].claimi;
    } 
    return ret;
}

/**
 * Context organization is spec-out by the vendor, this is the default 
 * mapping found in sifive's plic.
 */
__attribute__((weak))
int aplic_plat_idc_to_id(struct aplic_idc_id idc){
    if(idc.mode != PRIV_M && idc.mode != PRIV_S) return -1;
    return (idc.hart_id*2) + (idc.mode == PRIV_M ? 0 : 1);
}

__attribute__((weak))
struct aplic_idc_id aplic_plat_id_to_idc(idcid_t idc_id){
    struct aplic_idc_id idc;
    if(idc_id < APLIC_PLAT_IDC_NUM){
        idc.hart_id = idc_id/2;
        idc.mode = (idc_id%2) == 0 ? PRIV_M : PRIV_S; 
    } else {
        return (struct aplic_idc_id){-1};
    }
    return idc;
}

/**==== Debug functions ====*/

int debug_aplic_init(void){
    aplic_init();
    spin_lock(&print_lock);
    printf("Number of implemented interrupts: %d\r\n", APLIC_IMPL_INTERRUPTS);
    spin_unlock(&print_lock);
    return 0;
}

int debug_aplic_init_idc(void){
    aplic_idc_init();
    spin_lock(&print_lock);
    printf("APLIC: cpu %d successfully initialize his IDC at APLIC\r\n", get_cpuid());
    spin_unlock(&print_lock);
    return 0;
}

int debug_aplic_handle(){
    uint32_t claimi_aux = aplic_handle(get_cpuid());
    if(claimi_aux == 0){
        spin_lock(&print_lock);
        printf("No inetrrupt to handle\r\n");
        spin_unlock(&print_lock);
        return -1;
    }
    uint32_t intp_priority = claimi_aux & APLIC_TARGET_IPRIO_MASK;
    uint32_t intp_identity = (claimi_aux >> INTP_IDENTITY) & INTP_IDENTITY_MASK;
    spin_lock(&print_lock);
    printf("APLIC: IDC %d successfully catch interrupt %d with priority %d\r\n",
                            get_cpuid(), intp_identity, intp_priority);
    spin_unlock(&print_lock);
    return 0;
}