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
#include <imsic.h>
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
#define PLAT_NUM_DMAS 4

spinlock_t print_lock = SPINLOCK_INITVAL;

struct idma *dma_ut[PLAT_NUM_DMAS];

static inline void touchread(uintptr_t addr) {
  asm volatile("" ::: "memory");
  volatile uint64_t x = *(volatile uint64_t *)addr;
}

static inline void touchwrite(uintptr_t addr) {
  *(volatile uint64_t *)addr = 0x0;
}

static inline void fence_i() {
    asm volatile("fence.i" ::: "memory");
}

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

        #ifdef DMA_INFO
        for (size_t i = 0; i < PLAT_NUM_DMAS; i++){
            printf("idma %d last transaction ID: %ld\n", i, idma_get_last_completed_transac(dma_ut[i]));            
        }
        #endif
        
        printf("%ld\n", apb_timer_get_counter());

        #ifdef COUNT_HW_INTERF
        printf("%ld\n", aplic_get_counter());
        aplic_reset_counter();
        #endif
        
        #ifdef FLUSH_CACHE
        fence_i();
        #endif

        apb_timer_set_cmp(RESET_VAL);
        apb_timer_enable();
    }
}

//===============================================
//                      iDMA
//===============================================
void idma_start_interference(uint64_t idma_base_addr, size_t idma_index){
/** Instantiate and map the DMA */
  dma_ut[idma_index] = (void*)idma_base_addr+(idma_index*PAGE_SIZE);

  /** set iDMA source and destiny adresses */
  uintptr_t idma_src_addr = get_addr_base(0+idma_index);
  uintptr_t idma_dest_addr = get_addr_base(1+idma_index);

  /** Write known values to memory */
  /** Source memory position has 0xdeadbeef */
  *((volatile uint64_t*) idma_src_addr) = 0xdeadbeef;
  /** Clear the destiny memory position */
  *((volatile uint64_t*) idma_dest_addr) = 0x00;

  idma_config_single_transfer(dma_ut[idma_index], idma_src_addr, idma_dest_addr);

  // Check if iDMA was set up properly and init transfer
  uint64_t trans_id = dma_ut[idma_index]->next_transfer_id;
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
        aplic_start_interf_0();
        aplic_start_interf_1();
        idma_start_interference(IDMA_BASE_ADDR, 0);
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
