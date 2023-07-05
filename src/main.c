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
#include <apb_timer.h>

#define TIMER_INTERVAL (TIME_S(1))
#define UART_IRQ_PRIO 1
#ifdef IMSIC
#define UART_CHOSEN_IRQ 3
#else
#define UART_CHOSEN_IRQ 1
#endif
#define PUSH_BUTTON_IRQ     (8)

#define NUM_CPU 1

spinlock_t print_lock = SPINLOCK_INITVAL;

void apb_timer_handler(){
    static uint32_t count = 0;
    
    if(count < 1000){
        apb_timer_disable();
        printf("%d: 0x%lx\n", count, apb_timer_get_counter());
        // printf("%ld\n", apb_timer_get_counter());
        apb_timer_set_cmp(RESET_VAL);
        apb_timer_enable();
        count++;
        asm volatile("sfence.vma\n\t" ::: "memory");
    }
}

void push_button_handler(){
    apb_timer_disable();
    #ifndef IMSIC
    printf("PLIC: %ld\n", apb_timer_get_counter());
    #else
    printf("AIA: %ld\n", apb_timer_get_counter());
    #endif
    apb_timer_clr_counter();
    apb_timer_enable();
    asm volatile("sfence.vma\n\t" ::: "memory");
}

void uart_rx_handler(){
    static int count = 0;
    printf("cpu%d: %s, count: %d\n",get_cpuid(), __func__,  count++);
    uart_clear_rxirq();
}

void ipi_handler(){
    printf("cpu%d: %s\n", get_cpuid(), __func__);
}

void timer_handler(){
    static int targeting_cpu = 0;
    timer_set(TIMER_INTERVAL);
    printf("cpu%d: %s, targeting: %d\n", get_cpuid(), __func__, targeting_cpu);
    #ifdef IMSIC
    irq_send_ipi(targeting_cpu);
    #else
    irq_send_ipi(1ULL<<(targeting_cpu+1));
    #endif
    targeting_cpu++;
    if(targeting_cpu >= NUM_CPU){
        targeting_cpu = 0;
    }
}

void main(void){    
    static volatile bool master_done = false;
    
    if(cpu_is_master()){
        spin_lock(&print_lock);
        printf("Bao bare-metal test guest, %d\n", get_cpuid());
        spin_unlock(&print_lock);

        irq_set_handler(UART_CHOSEN_IRQ, uart_rx_handler);
        // irq_set_handler(TIMER_IRQ_ID, timer_handler);
        // irq_set_handler(IPI_IRQ_ID, ipi_handler);
        // irq_set_handler(PUSH_BUTTON_IRQ, push_button_handler);
        // irq_set_handler(APB_TIMER_IRQ, apb_timer_handler);

        uart_enable_rxirq();

        // timer_set(TIMER_INTERVAL);
        // irq_enable(TIMER_IRQ_ID);
        // irq_set_prio(TIMER_IRQ_ID, 1);

        /** Initialize the APB timer */
        // apb_timer_disable();
        // apb_timer_clr_counter();
        // apb_timer_set_cmp(RESET_VAL);
        // irq_confg(APB_TIMER_IRQ, APB_TIMER_IRQ, get_cpuid(), APLIC_SOURCECFG_SM_EDGE_RISE);
        // apb_timer_enable();

        /** Configure the push button interrupt */
        // irq_confg(PUSH_BUTTON_IRQ, 2, get_cpuid(), APLIC_SOURCECFG_SM_EDGE_RISE);


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

    // #ifndef IMSIC
    // irq_enable(IPI_IRQ_ID);
    // irq_set_prio(IPI_IRQ_ID, 1);
    // #endif

    while(1) wfi();
}
