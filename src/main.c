/** 
 * Bao, a Lightweight Static Partitioning Hypervisor 
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *      Sandro Pinto <sandro.pinto@bao-project.org>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details. 
 *
 */

#include <core.h>
#include <stdlib.h>
#include <stdio.h>
#include <cpu.h>
#include <wfi.h>
#include <spinlock.h>
#include <plat.h>
#include <irq.h>
#include <aplic.h>
#include <uart.h>
#include <timer.h>
#include <fences.h>
#include <idma.h>
#include <apb_timer.h>

#define TIMER_INTERVAL (TIME_S(1))
#define UART_IRQ_PRIO 1
#ifdef IMSIC
#define UART_CHOSEN_IRQ 3
#else
#define UART_CHOSEN_IRQ UART_IRQ_ID
#endif

#define NUM_CPU 1

spinlock_t print_lock = SPINLOCK_INITVAL;
//===============================================
//                IRQs handlers
//===============================================

/** We have this UART Rx handler for debug and testing purposes */
void uart_rx_handler(){
    static int count = 0;
    printf("cpu%d: %s, count: %d\n",get_cpuid(), __func__,  count++);
    uart_clear_rxirq();
}

void apb_timer_handler(){
    static uint32_t count = 0;
    
    if(count < 1000){
        apb_timer_disable();
        count++;
        // printf("%d: 0x%lx\n", count, apb_timer_get_counter());
        printf("%ld\n", apb_timer_get_counter()-RESET_VAL);
        apb_timer_set_cmp(RESET_VAL);
        apb_timer_enable();
        asm volatile("sfence.vma\n\t" ::: "memory");
    }
}

//===============================================
//                      iDMA
//===============================================
void idma_start_interference(void){
/** Instantiate and map the DMA */
  struct idma *dma_ut = (void*)IDMA_BASE_ADDR;

  /** set iDMA source and destiny adresses */
  uintptr_t idma_src_addr = get_addr_base(0);
  uintptr_t idma_dest_addr = get_addr_base(1);

  /** Write known values to memory */
  /** Source memory position has 0xdeadbeef */
  *((volatile uint64_t*) idma_src_addr) = 0xdeadbeef;
  /** Clear the destiny memory position */
  *((volatile uint64_t*) idma_dest_addr) = 0x00;

  idma_config_single_transfer(dma_ut, idma_src_addr, idma_dest_addr);

  // Check if iDMA was set up properly and init transfer
  uint64_t trans_id = dma_ut->next_transfer_id;
  if (!trans_id){
    printf("iDMA misconfigured\r\n");
  }

  // Poll transfer status
  // while (read64((uintptr_t)&dma_ut->last_transfer_id_complete) != trans_id);
}

void main(void){    
    static volatile bool master_done = false;
    int interference_choice;

    if(cpu_is_master()){
        spin_lock(&print_lock);
        printf("Interrupt Latency Study, %d\n", get_cpuid());
        spin_unlock(&print_lock);

        #ifdef INTERF_ON
        idma_start_interference();
        #endif

        irq_set_handler(UART_CHOSEN_IRQ, uart_rx_handler);
        uart_enable_rxirq();

        irq_set_handler(APB_TIMER_IRQ, apb_timer_handler);
        /** Initialize the APB timer */
        apb_timer_disable();
        apb_timer_clr_counter();
        apb_timer_set_cmp(RESET_VAL);
        irq_confg(APB_TIMER_IRQ, APB_TIMER_IRQ, get_cpuid(), APLIC_SOURCECFG_SM_EDGE_RISE);
        apb_timer_enable();

        master_done = true;
    }

    while(!master_done);

    /**==== CPU related IRQC Initialization ====*/
    irq_cpu_init();
    spin_lock(&print_lock);
    printf("cpu %d up\n", get_cpuid());
    spin_unlock(&print_lock);

    /**==== Interrupt configuration ====*/
    if(get_cpuid() == 0){
        /** APLIC config */
        #ifndef IMSIC
        irq_confg(UART_IRQ_ID, UART_IRQ_PRIO, get_cpuid(), APLIC_SOURCECFG_SM_EDGE_RISE);
        #else
        irq_confg(UART_IRQ_ID, UART_CHOSEN_IRQ, get_cpuid(), APLIC_SOURCECFG_SM_EDGE_RISE);
        #endif
    }

    while(1) wfi();
}
