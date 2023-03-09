#ifndef _IMSIC_H_
#define _IMSIC_H_

#include <core.h>
#include <csrs.h>
#include <stdlib.h>
#include <stdio.h>

struct imsic_global {
    uint32_t seteipnum_le;
    uint32_t seteipnum_be;
} __attribute__((__packed__, aligned(0x1000ULL)));

extern volatile struct imsic_global *imsic_s_lvl;

#define NR_INTP 1024
typedef unsigned idcid_t;

void imsic_init(void);
void imsic_handle(void);
void imsic_send_msi(unsigned long target_cpu_mask, uint32_t data);


#endif //_IMSIC_H_