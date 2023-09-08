#ifndef ARCH_IRQ_H
#define ARCH_IRQ_H

#include <irq.h>

// #define APLIC 1
// #define IMSIC 1
// #define INTERF_ON 1

#ifdef IMSIC
#define IPI_IRQ_ID (1)
#else
#define IPI_IRQ_ID (1025)
#endif

#define TIMER_IRQ_ID (1029)

#define IRQ_NUM (1030)
#define IRQ_MAX_PRIO (-1)

#endif /* ARCH_IRQ_H */
