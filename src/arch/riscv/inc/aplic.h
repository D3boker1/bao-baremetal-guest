/**
 * @file aplic.h
 * @author Jose Martins <jose.martins@bao-project.org>
 * @author Francisco Marques (fmarques_00@protonmail.com)
 * @brief This module gives a set of function to controls the RISC-V APLIC.
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

#ifndef _APLIC_H_
#define _APLIC_H_

#include <core.h>
#include <csrs.h>
#include <spinlock.h>
#include <stdio.h>
#define APLIC_PLAT_IDC_NUM (8)

extern spinlock_t print_lock;

/**==== APLIC Specific types ====*/
typedef unsigned idcid_t;
typedef unsigned irqid_t;

/**==== APLIC defines ====*/
#define APLIC_BASE          (0xd000000)
#define APLIC_IDC_BASE      (APLIC_BASE+0x4000)

#define APLIC_MAX_INTERRUPTS    (1024)
#define APLIC_NUM_SRCCFG_REGS   (APLIC_MAX_INTERRUPTS)
#define APLIC_NUM_TARGET_REGS   (APLIC_MAX_INTERRUPTS)
/** where x = E or P*/
#define APLIC_NUM_CLRIx_REGS    (APLIC_MAX_INTERRUPTS / 32)
#define APLIC_NUM_SETIx_REGS    (APLIC_MAX_INTERRUPTS / 32)
#define APLIC_IDC_OFF           (0x4000)

/**==== Data structures for APLIC devices ====*/
struct aplic_global {
    uint32_t domaincfg;
    uint32_t sourcecfg[APLIC_NUM_SRCCFG_REGS];
    uint32_t mmsiaddrcfg;
    uint32_t mmsiaddrcfgh;
    uint32_t smsiaddrcfg;
    uint32_t smsiaddrcfgh;
    uint32_t setip[APLIC_NUM_SETIx_REGS];
    uint32_t setipnum;
    uint32_t in_clrip[APLIC_NUM_CLRIx_REGS];
    uint32_t clripnum;
    uint32_t setie[APLIC_NUM_SETIx_REGS];
    uint32_t setienum;
    uint32_t clrie[APLIC_NUM_CLRIx_REGS];
    uint32_t clrienum;
    uint32_t setipnum_le;
    uint32_t setipnum_be;
    uint32_t reserved[0x3000 - 0x2008];
    uint32_t genmsi;
    uint32_t target[APLIC_NUM_TARGET_REGS];
};// __attribute__((__packed__, aligned(PAGE_SIZE)));

struct aplic_idc {
    uint32_t idelivery;
    uint32_t iforce;
    uint32_t ithreshold;
    uint32_t topi;
    uint32_t claimi;
};// __attribute__((__packed__, aligned(PAGE_SIZE)));

extern volatile struct aplic_global *aplic_domain;
extern volatile struct aplic_idc *idc;

/**==== Initialization Function ====*/

/**
 * @brief Initialize the aplic domain.
 * 
 */
void aplic_init(void);

/**
 * @brief Initialize the aplic IDCs. 
 * The IDC component is the closest to the cpu.
 * 
 */
void aplic_idc_init(void);

/**==== APLIC Domain registers manipulation functions ====*/

/**
 * @brief Write to aplic domaincfg register.
 * AIA Spec. 0.3.2 section 4.5.1
 * 
 * @param val Value to be written into domaincfg
 */
void aplic_set_domaincfg(uint32_t val);

/**
 * @brief Read from aplic domaincfg register.
 * AIA Spec. 0.3.2 section 4.5.1
 * 
 * @return uint32_t 32 bit value containing domaincfg info.
 */
uint32_t aplic_get_domaincfg(void);

/**
 * @brief Write to aplic's sourcecfg register
 * 
 * @param int_id interruption ID identifies the interrupt to be configured/read.
 * @param val Value to be written into sourcecfg
 */
void aplic_set_sourcecfg(irqid_t int_id, uint32_t val);

/**
 * @brief Configure the source mode for a given interrupt
 * 
 * @param int_id interruption ID identifies the interrupt to be configured/read.
 * @param new_sm Value to be written into sourcecfg.SM field
 * 
 * Possible values for new_sm are (see table 4.2 from AIA Spec. 0.3.2):
 * 
 * +-------+----------+--------------------------------------------------+
 * | Value |   Name   |                   Description                    |
 * +-------+----------+--------------------------------------------------+
 * |     0 | INACT    | Inactive in this domain                          |
 * |     1 | DETACHED | Active, detached from the source wire            |
 * |     4 | EDGE1    | Active, edge-sensitive; asserted on rising edge  |
 * |     5 | EDGE0    | Active, edge-sensitive; asserted on falling edge |
 * |     6 | LEVEL1   | Active, level-sensitive; asserted when high      |
 * |     7 | LEVEL0   | Active, level-sensitive; asserted when low       |
 * +-------+----------+--------------------------------------------------+
 */
void aplic_set_src_mode(irqid_t int_id, uint32_t new_sm);

/**
 * @brief Read from aplic's sourcecfg register
 * 
 * @param int_id interruption ID identifies the interrupt to be configured/read.
 * @return uint32_t 32 bit value containing interrupt int_id's configuration.
 */
uint32_t aplic_get_sourcecfg(irqid_t int_id);

/**
 * @brief Set a given interrupt as pending, using setip registers.
 * 
 * @param int_id Interrupt to be set as pending
 */
void aplic_set_pend(irqid_t int_id);

/**
 * @brief Set a given interrupt as pending, using setipnum register.
 * This should be faster than aplic_set_pend.
 * 
 * @param int_id Interrupt to be set as pending
 */
void aplic_set_pend_num(irqid_t int_id);

/**
 * @brief Read the pending value of a given interrut
 * 
 * @param int_id interrupt to read from
 * @return true if interrupt is pended
 * @return false if interrupt is NOT pended
 */
bool aplic_get_pend(irqid_t int_id);

/**
 * @brief Clear a pending bit from a inetrrupt writting to in_clrip.
 * 
 * @param int_id interrupt to clear the pending bit from
 */
void aplic_set_inclrip(irqid_t int_id);

/**
 * @brief Clear a pending bit from a inetrrupt writting to in_clripnum.
 * Should be faster than aplic_set_inclrip.
 *  
 * @param int_id interrupt to clear the pending bit from
 */
void aplic_set_clripnum(irqid_t intp_id);

/**
 * @brief Read the current rectified value for a given interrupt
 * 
 * @param int_id interrupt to read from
 * @return true 
 * @return false 
 */
bool aplic_get_inclrip(irqid_t int_id);

/**
 * @brief Enable a given interrupt writting to setie registers
 * 
 * @param int_id Interrupt to be enabled
 */
void aplic_set_ie(irqid_t int_id);

/**
 * @brief Enable a given interrupt writting to setienum register
 * Should be faster than aplic_set_ie 
 * 
 * @param int_id Interrupt to be enabled
 */
void aplic_set_ienum(irqid_t int_id);

/**
 * @brief Read if a given interrupt is enabled
 * 
 * @param intp_id interrupt to evaluate if it is enabled
 * @return uint32_t 
 */
bool aplic_get_ie(irqid_t intp_id);

/**
 * @brief Clear enable bit be writting to clrie register of a given interrupt
 * 
 * @param int_id Interrupt to clear the enable bit
 */
void aplic_set_clrie(irqid_t int_id);

/**
 * @brief Clear enable bit be writting to clrie register of a given interrupt. 
 * It should be faster than aplic_set_clrie 
 * 
 * @param int_id Interrupt to clear the enable bit
 */
void aplic_set_clrienum(irqid_t int_id);

/**
 * @brief Write to register target (see AIA spec 0.3.2 section 4.5.16)
 * 
 * @param int_id Interrupt to configure the target options
 * @param val Value to be written in target register
 * 
 * If domaincfg.DM = 0, target have the format:
 * 
 * +-----------+------------+----------------------------------------+
 * | Bit-Field |    Name    |              Description               |
 * +-----------+------------+----------------------------------------+
 * | 31:28     | Hart Index | Hart to which interrupts will delivery |
 * | 7:0       | IPRIO      | Interrupt priority.                    |
 * +-----------+------------+----------------------------------------+
 * 
 * 
 * If domaincfg.DM = 1, target have the format:
 * 
 * +-----------+-------------+----------------------------------------------------------------+
 * | Bit-Field |    Name     |                          Description                           |
 * +-----------+-------------+----------------------------------------------------------------+
 * | 31:28     | Hart Index  | Hart to which interrupts will delivery                         |
 * | 17:12     | Guest Index | Only if hypervisor extension were implemented.                 |
 * | 10:0      | EIID        | External Interrupt Identity. Specifies the data value for MSIs |
 * +-----------+-------------+----------------------------------------------------------------+
 */
void aplic_set_target(irqid_t int_id, uint32_t val);

/**
 * @brief Read the target configurations for a given interrupt
 * 
 * @param int_id Interrupt to read from
 * @return uint32_t value with the requested data
 */
uint32_t aplic_get_target(irqid_t int_id);

/**
 * @brief Set priority for a given interrupt.
 * 
 * @param int_id Interrupt to set a new value of priority
 * @param val priority. It is a value with IPRIOLEN. 
 * 
 * IPRIOLEN is in range 1 to 8 and is an implementation specific.
 * 
 * If val = 0, then the priority will be set to 1 instead.
 * 
 * Since this is a field of target, and only a valid field when 
 * domaincfg.DM = 0, using this function as no effect if domaincfg.DM = 1
 */
void aplic_set_prio(irqid_t int_id, uint8_t val);

/**==== APLIC IDC's registers manipulation functions ====*/

/**
 * @brief Enable/Disable the delivering for a given idc
 * 
 * @param idc_id IDC to enbale/disable the delivering
 * @param en if 0: interrupts delivery disable; if 1: interrupts delivery enable
 */
void aplic_idc_set_idelivery(idcid_t idc_id, bool en);

/**
 * @brief Read if for a given idc the interrupts are being delivered.
 * 
 * @param idc_id IDC to read.
 * @return true Interrupt delivery is enabled
 * @return false Interrupt delivery is disable
 */
bool aplic_idc_get_idelivery(idcid_t idc_id);

/**
 * @brief Useful for testing. Seting this register forces an interrupt to
 * be asserted to the corresponding hart
 * 
 * @param idc_id IDC to force an interruption
 * @param en value to be written
 */
void aplic_idc_set_iforce(idcid_t idc_id, bool en);

/**
 * @brief Read if for a given IDC was forced an interrupt
 * 
 * @param idc_id IDC to test
 * @return true if an interrupt were forced
 * @return false if an interrupt were NOT forced
 */
bool aplic_idc_get_iforce(idcid_t idc_id);

/**
 * @brief Write a new value of threshold for a given IDC 
 * 
 * @param idc_id IDC to set a new value for threshold
 * @param new_th the new value of threshold. It is a value with IPRIOLEN. 
 * 
 * IPRIOLEN is in range 1 to 8 and is an implementation specific.
 * setting threshold for a nonzero value P, inetrrupts with priority of P of higher
 * DO NOT contribute to signaling interrupts to the hart.
 * If 0, all enabled interrupts can contibute to signaling inetrrupts to the hart.
 * Writting a value higher than APLIC minimum priority (maximum number)
 * takes no effect.
 */
void aplic_idc_set_ithreshold(idcid_t idc_id, uint32_t new_th);

/**
 * @brief Read the current value of threshold for a given IDC.
 * 
 * @param idc_id IDC to read the threshold value
 * @return uint32_t value with the threshold
 */
uint32_t aplic_idc_get_ithreshold(idcid_t idc_id);

/**
 * @brief Indicates the current highest-priority pending-and-enabled interrupt
 * targeted to this this hart.
 * 
 * @param idc_id IDC to read the highest-priority
 * @return uint32_t returns the interrupt identity and interrupt priority.
 * 
 * Formart:
 * 
 * +-----------+--------------------+------------------------+
 * | Bit-Field |        Name        |      Description       |
 * +-----------+--------------------+------------------------+
 * | 25:16     | Interrupt Identity | Equal to source number |
 * | 7:0       | Interrupt priority | Interrupt priority     |
 * +-----------+--------------------+------------------------+
 * 
 */
uint32_t aplic_idc_get_topi(idcid_t idc_id);

/**
 * @brief As the same value as topi. However, reading claimi has the side effect
 * of clearing the pending bit for the reported inetrrupt identity.
 * 
 * @param idc_id IDC to read and clear the pending-bit the highest-priority
 * @return uint32_t returns the interrupt identity and interrupt priority.
 */
uint32_t aplic_idc_get_claimi(idcid_t idc_id);

bool aplic_idc_valid(idcid_t idc_id);

/**
 * @brief Handles an incomming interrupt in irq controller.
 * 
 * @param idc_id IDC index from where to read the claimi register 
 * @return uint32_t with interrupt identity (25:16) and priority (7:0)
 */
uint32_t aplic_handle(idcid_t idc_id);

struct aplic_idc_id {
    int hart_id;
    int mode;
};

int aplic_plat_idc_to_id(struct aplic_idc_id idc);
struct aplic_idc_id aplic_plat_id_to_idc(idcid_t idc_id);

/**==== Debug functions ====*/

/**
 * @brief Initialize the aplic.
 * On success prints: "APLIC: successfully initialized"
 * 
 * @return int 0 on success; negative value if not;
 */
int debug_aplic_init(void);

/**
 * @brief Initialize the aplic idc for the a cpu.
 * Each cpu will execute on their own this function and will initialize
 * the IDC structure for its hart.
 * 
 * @return int int 0 on success; negative value if not;
 */
int debug_aplic_init_idc(void);

/**
 * @brief Catch an interrupt at APLIC.
 * On success prints: "APLIC: interrupt catched"
 * 
 * @return int 0 on success; negative value if not;
 */
int debug_aplic_handle(void);

#endif //_APLIC_H_