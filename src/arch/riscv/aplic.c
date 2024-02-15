/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <aplic.h>
#include <fences.h>
#include <cpu.h>
#include <irq.h>

/** APLIC fields and masks defines */
#define APLIC_DOMAINCFG_CTRL_MASK      (0x1FF)

#define DOMAINCFG_DM                   (1U << 2)

#define INTP_IDENTITY                  (16)
#define INTP_IDENTITY_MASK             (0x3FF)

#define APLIC_DISABLE_IDELIVERY        (0)
#define APLIC_ENABLE_IDELIVERY         (1)
#define APLIC_DISABLE_IFORCE           (0)
#define APLIC_ENABLE_IFORCE            (1)
#define APLIC_IDC_ITHRESHOLD_EN_ALL    (0)
#define APLIC_IDC_ITHRESHOLD_DISBL_ALL (1)

volatile struct aplic_control_hw* aplic_control = (void*) APLIC_BASE;
volatile struct aplic_idc_hw* aplic_idc = (void*) APLIC_IDC_BASE; 

void aplic_init(void)
{
    aplic_control->domaincfg = 0;
    #ifdef IMSIC
    aplic_control->domaincfg |= APLIC_DOMAINCFG_DM;
    #endif
    
    /** Clear all pending and enabled bits*/
    for (size_t i = 0; i < APLIC_NUM_CLRIx_REGS; i++) {
        aplic_control->setip[i] = 0;
        aplic_control->setie[i] = 0;
    }

    /** Sets the default value of target and sourcecfg */
    for (size_t i = 0; i < APLIC_NUM_TARGET_REGS; i++) {
        aplic_control->sourcecfg[i] = APLIC_SOURCECFG_SM_INACTIVE;
        #ifdef IMSIC
        aplic_control->target[i] = i+1;
        #else
        aplic_control->target[i] = APLIC_TARGET_MIN_PRIO;
        #endif
    }
    aplic_control->domaincfg |= APLIC_DOMAINCFG_IE;
}

void aplic_idc_init(void)
{
    idcid_t idc_id = get_cpuid();
    aplic_idc[idc_id].ithreshold = APLIC_IDC_ITHRESHOLD_EN_ALL;
    aplic_idc[idc_id].iforce = APLIC_DISABLE_IFORCE;
    aplic_idc[idc_id].idelivery = APLIC_ENABLE_IDELIVERY;
}

void aplic_set_sourcecfg(irqid_t intp_id, uint32_t val)
{
    aplic_control->sourcecfg[intp_id - 1] = val & APLIC_SOURCECFG_SM_MASK;
}

void aplic_set_enbl(irqid_t intp_id)
{
    aplic_control->setienum = intp_id;
}

void aplic_set_target_prio(irqid_t intp_id, uint8_t prio)
{
    aplic_control->target[intp_id - 1] &= ~(APLIC_TARGET_IPRIO_MASK);
    aplic_control->target[intp_id - 1] |= (prio & APLIC_TARGET_IPRIO_MASK);
}

void aplic_set_target_hart(irqid_t intp_id, cpuid_t hart)
{
    aplic_control->target[intp_id - 1] &=
        ~(APLIC_TARGET_HART_IDX_MASK << APLIC_TARGET_HART_IDX_SHIFT);
    aplic_control->target[intp_id - 1] |= hart << APLIC_TARGET_HART_IDX_SHIFT;
}

static irqid_t aplic_idc_get_claimi_intpid(idcid_t idc_id)
{
    return (aplic_idc[idc_id].claimi >> IDC_CLAIMI_INTP_ID_SHIFT) & IDC_CLAIMI_INTP_ID_MASK;
}

void aplic_handle(void)
{
    idcid_t idc_id = get_cpuid();
    irqid_t intp_identity = aplic_idc_get_claimi_intpid(idc_id);

    if (intp_identity != 0) {
        irq_handle(intp_identity);
    }
}