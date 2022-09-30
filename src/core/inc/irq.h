#ifndef IRQ_H
#define IRQ_H

#include <core.h>
#include <arch/irq.h>
#include <spinlock.h>

//#define APLIC 1

typedef void (*irq_handler_t)(unsigned id);

extern spinlock_t print_lock;

void irq_init(void);
void irq_cpu_init(void);
void irq_confg(unsigned id, unsigned prio, unsigned hart_indx, unsigned src_mode);
void irq_handle(unsigned id);
void irq_set_handler(unsigned id, irq_handler_t handler);
void irq_enable(unsigned id);
void irq_set_prio(unsigned id, unsigned prio);
void irq_send_ipi(unsigned long target_cpu_mask);

#endif // IRQ_H
