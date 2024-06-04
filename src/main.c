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
#include <idma.h>
#include <fences.h>

#define TIMER_INTERVAL (TIME_S(1))

#define USE_TIMER_IRQ 
#define USE_IPI_IRQ  // requires the timer
#define USE_UART_IRQ
#define UART_HART 1
#define UART_ID_IMSIC 2 // this number can be anything

#define NUM_TRANSFERS   (16)
#define NUM_PAGES       (8)
#define NUM_DEVICES     (1)

// iDMA
#define DMA_BASE(idx)           (0x50000000 + (idx * 0x1000))

#define DMA_SRC_ADDR(idx)       (DMA_BASE(idx) + DMA_FRONTEND_SRC_ADDR_REG_OFFSET)
#define DMA_DST_ADDR(idx)       (DMA_BASE(idx) + DMA_FRONTEND_DST_ADDR_REG_OFFSET)
#define DMA_NUMBYTES_ADDR(idx)  (DMA_BASE(idx) + DMA_FRONTEND_NUM_BYTES_REG_OFFSET)
#define DMA_CONF_ADDR(idx)      (DMA_BASE(idx) + DMA_FRONTEND_CONF_REG_OFFSET)
#define DMA_STATUS_ADDR(idx)    (DMA_BASE(idx) + DMA_FRONTEND_STATUS_REG_OFFSET)
#define DMA_NEXTID_ADDR(idx)    (DMA_BASE(idx) + DMA_FRONTEND_NEXT_ID_REG_OFFSET)
#define DMA_DONE_ADDR(idx)      (DMA_BASE(idx) + DMA_FRONTEND_DONE_REG_OFFSET)

#define DMA_TRANSFER_SIZE (1 * sizeof(uint64_t))

#define DMA_CONF_DECOUPLE 0
#define DMA_CONF_DEBURST 0
#define DMA_CONF_SERIALIZE 0

volatile uint64_t* dma_src[NUM_DEVICES]   = {
    (volatile uint64_t *) DMA_SRC_ADDR(0)
};

volatile uint64_t* dma_dst[NUM_DEVICES]   = {
    (volatile uint64_t *) DMA_DST_ADDR(0)
};

volatile uint64_t* dma_num_bytes[NUM_DEVICES] = {
    (volatile uint64_t *) DMA_NUMBYTES_ADDR(0)
};

volatile uint64_t* dma_conf[NUM_DEVICES]  = {
    (volatile uint64_t *) DMA_CONF_ADDR(0)
};

volatile uint64_t* dma_nextid[NUM_DEVICES]    = {
    (volatile uint64_t *) DMA_NEXTID_ADDR(0)
};

volatile uint64_t* dma_done[NUM_DEVICES]  = {
    (volatile uint64_t *) DMA_DONE_ADDR(0)
};

// Pages
uint64_t src_0[4096 / sizeof(uint64_t)] __attribute__ ((aligned (4096)));
uint64_t dst_0[4096 / sizeof(uint64_t)] __attribute__ ((aligned (4096)));
uint64_t src_1[4096 / sizeof(uint64_t)] __attribute__ ((aligned (4096)));
uint64_t dst_1[4096 / sizeof(uint64_t)] __attribute__ ((aligned (4096)));
uint64_t src_2[4096 / sizeof(uint64_t)] __attribute__ ((aligned (4096)));
uint64_t dst_2[4096 / sizeof(uint64_t)] __attribute__ ((aligned (4096)));
uint64_t src_3[4096 / sizeof(uint64_t)] __attribute__ ((aligned (4096)));
uint64_t dst_3[4096 / sizeof(uint64_t)] __attribute__ ((aligned (4096)));
uint64_t src_4[4096 / sizeof(uint64_t)] __attribute__ ((aligned (4096)));
uint64_t dst_4[4096 / sizeof(uint64_t)] __attribute__ ((aligned (4096)));
uint64_t src_5[4096 / sizeof(uint64_t)] __attribute__ ((aligned (4096)));
uint64_t dst_5[4096 / sizeof(uint64_t)] __attribute__ ((aligned (4096)));
uint64_t src_6[4096 / sizeof(uint64_t)] __attribute__ ((aligned (4096)));
uint64_t dst_6[4096 / sizeof(uint64_t)] __attribute__ ((aligned (4096)));
uint64_t src_7[4096 / sizeof(uint64_t)] __attribute__ ((aligned (4096)));
uint64_t dst_7[4096 / sizeof(uint64_t)] __attribute__ ((aligned (4096)));

uint64_t* src[NUM_PAGES] = {
    src_0,
    src_1,
    src_2,
    src_3,
    src_4,
    src_5,
    src_6,
    src_7
};

uint64_t* dst[NUM_PAGES] = {
    dst_0,
    dst_1,
    dst_2,
    dst_3,
    dst_4,
    dst_5,
    dst_6,
    dst_7
};

spinlock_t print_lock = SPINLOCK_INITVAL;

void uart_rx_handler(){
    spin_lock(&print_lock);
    printf("cpu%d: %s\n",get_cpuid(), __func__);
    spin_unlock(&print_lock);
    uart_clear_rxirq();
}

void timer_handler(){
    spin_lock(&print_lock);
    printf("cpu%d: %s\n", get_cpuid(), __func__);
    spin_unlock(&print_lock);
    timer_set(TIMER_INTERVAL);

    #ifdef USE_IPI_IRQ
        #ifdef IMSIC
            irq_send_ipi(get_cpuid() ^ 1);
        #else
            irq_send_ipi(1ull << (get_cpuid() + 1));
        #endif
    #endif
}

void ipi_handler(){
    static volatile int count = 0;

    spin_lock(&print_lock);
    printf("cpu%d: %s\n", get_cpuid(), __func__);
    spin_unlock(&print_lock);
    
    #ifdef IMSIC
        if(count == 1) {
            count = 0;
            return;
        }
        
        count++;
        irq_send_ipi(get_cpuid() ^ 1);
    #else
        irq_send_ipi(1ull << (get_cpuid() + 1));
    #endif
}

void main(void){

    static volatile bool master_done = false;

    if(cpu_is_master()){
        spin_lock(&print_lock);
        printf("Bao bare-metal test guest %d\n", get_cpuid());
        spin_unlock(&print_lock);

        #ifdef USE_UART_IRQ
            #ifndef IMSIC
            irq_set_handler(UART_IRQ_ID, uart_rx_handler);
            #else
            irq_set_handler(UART_ID_IMSIC, uart_rx_handler);
            #endif
            uart_enable_rxirq();
        #endif

        #ifdef USE_TIMER_IRQ
            irq_set_handler(TIMER_IRQ_ID, timer_handler);
            timer_set(TIMER_INTERVAL);
            irq_enable(TIMER_IRQ_ID, get_cpuid());
        #endif

        #ifdef USE_IPI_IRQ
            irq_set_handler(IPI_IRQ_ID, ipi_handler);
        #endif

        master_done = true;
    }
    irq_cpu_init();

    /**==== Interrupt configuration ====*/
    if(get_cpuid() == UART_HART)
    {
        #ifdef USE_UART_IRQ
            irq_enable(UART_IRQ_ID, get_cpuid());
            #ifndef IMSIC
            irq_set_prio(UART_IRQ_ID, 1);
            #else
            irq_set_prio(UART_IRQ_ID, UART_ID_IMSIC);
            #endif
        #endif
    }

    while(!master_done);
    spin_lock(&print_lock);
    printf("cpu %d up\n", get_cpuid());
    spin_unlock(&print_lock);

    uint64_t transfer_id;

    for (size_t i = 0; i < NUM_TRANSFERS; i++)
    {
        // Get sequential indexes
        size_t dev_idx = i % (NUM_DEVICES);
        size_t idx = i % (NUM_PAGES);

        // Set Src and Dst
        src[idx][0] = i * 0x11;
        dst[idx][0] = 0;

        spin_lock(&print_lock);        
        printf("Dev: %d\nSrc addr: %lx\nDst addr: %lx\n", dev_idx, src[idx], dst[idx]);
        spin_unlock(&print_lock);

        fence_i();

        //# Setup memory transfer
        *dma_src[dev_idx]       = (uint64_t)(&(src[idx][0]));
        *dma_dst[dev_idx]       = (uint64_t)(&(dst[idx][0]));
        *dma_num_bytes[dev_idx] = DMA_TRANSFER_SIZE;
        *dma_conf[dev_idx]      = (DMA_CONF_DECOUPLE  << DMA_FRONTEND_CONF_DECOUPLE_BIT) |
                                  (DMA_CONF_DEBURST   << DMA_FRONTEND_CONF_DEBURST_BIT) |
                                  (DMA_CONF_SERIALIZE << DMA_FRONTEND_CONF_SERIALIZE_BIT);

        //# Trigger transfer
        transfer_id = *dma_nextid[dev_idx];

        spin_lock(&print_lock);
        printf("Transfer ID: %d\r\n", transfer_id);
        spin_unlock(&print_lock);

        //# Poll done register
        while (*dma_done[dev_idx] != transfer_id)
            ;

        //# Check results
        if (src[idx][0] != dst[idx][0])
        {
            spin_lock(&print_lock);
            printf("Src and Dst do not match\r\n");
            spin_unlock(&print_lock);
            break;
        }
    }

    while(1) wfi();
}
