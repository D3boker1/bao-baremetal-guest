#ifndef ARCH_IRQ_H
#define ARCH_IRQ_H

#define TIMER_IRQ_ID (1029)

#define IRQ_NUM (1030)
#define IRQ_MAX_PRIO (-1)

/** Use this to choose the IRQC */
// #define PLIC    0
// #define APLIC   1
#define IMSIC   2

#ifdef IMSIC
#define IPI_IRQ_ID (1) 
#else
#define IPI_IRQ_ID (1025)
#endif

#endif /* ARCH_IRQ_H */
