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
#include <uart.h>
#include <timer.h>

#define TIMER_INTERVAL (TIME_S(1))

spinlock_t print_lock = SPINLOCK_INITVAL;

void uart_rx_handler(){
    spin_lock(&print_lock);
    printf("cpu%d: %s\n",get_cpuid(), __func__);
    spin_unlock(&print_lock);
    uart_clear_rxirq();
}

void main(void){

    static volatile bool master_done = false;

    if(cpu_is_master()){
        spin_lock(&print_lock);
        printf("Bao bare-metal test guest 0\n");
        spin_unlock(&print_lock);

        irq_set_handler(UART_IRQ_ID, uart_rx_handler);
        uart_enable_rxirq();

        master_done = true;
    }
    irq_cpu_init();

    /**==== Interrupt configuration ====*/
    if(get_cpuid() == 0)
    {
        irq_enable(UART_IRQ_ID,  get_cpuid());
        irq_set_prio(UART_IRQ_ID, 1);
    }
    
    while(!master_done);
    spin_lock(&print_lock);
    printf("cpu %d up\n", get_cpuid());
    spin_unlock(&print_lock);

    while(1) wfi();
}
