#ifndef ARCH_IRQ_H
#define ARCH_IRQ_H

#ifdef IMSIC
#define IPI_IRQ_ID (1)
#else
#define IPI_IRQ_ID (1025) //1025
#endif

#define TIMER_IRQ_ID (1029)

#define IRQ_NUM (1030)
#define IRQ_MAX_PRIO (-1)

#endif /* ARCH_IRQ_H */
