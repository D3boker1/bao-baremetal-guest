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

#define TIMER_INTERVAL (TIME_S(1))
#define UART_IRQ_PRIO 1

void uart_rx_handler(){
    static uint32_t i = 0;
    printf("cpu%d: %s x %d\n",get_cpuid(), __func__, i);
    uart_clear_rxirq();
    i++;
}

void main(void){

    static volatile bool master_done = false;

    if(cpu_is_master()){
        spin_lock(&print_lock);
        #ifndef APLIC
        printf("PLIC: Bao bare-metal test guest\n");
        #endif
        #ifdef APLIC
        printf("APLIC: Bao bare-metal test guest\n");
        #endif
        spin_unlock(&print_lock);
        
        //debug_aplic_check_addrs();
        irq_init();
        
        /**==== Populate handler ====*/
        irq_set_handler(UART_IRQ_ID, uart_rx_handler);
        
        /**==== Initialize the UART ====*/
        uart_enable_rxirq();

        master_done = true;
    }
    
    while(!master_done);
    /**==== IDC Initialization ====*/
    irq_cpu_init();
    spin_lock(&print_lock);
    printf("cpu %d up\n", get_cpuid());
    spin_unlock(&print_lock);

    /**==== External Interrupt configuration ====*/
    if(get_cpuid() == 3){
        irq_confg(UART_IRQ_ID, UART_IRQ_PRIO, get_cpuid(), APLIC_SOURCECFG_SM_EDGE_RISE);
    }

    while(1) wfi();
}