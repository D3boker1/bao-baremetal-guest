#ifndef _IMSIC_H_
#define _IMSIC_H_

#include <core.h>
#include <csrs.h>

#define IMSICS_BASE 0x28000000
#define IMSIC_NUM_FILES 1

struct imsic_intp_file_hw
{
    uint32_t seteipnum_le;
    uint32_t seteipnum_be;
}__attribute__((__packed__, aligned(0x1000ULL)));

struct imsic_global {
    struct imsic_intp_file_hw s_file;
    struct imsic_intp_file_hw vs_file;
} __attribute__((__packed__, aligned(0x1000ULL)));

extern volatile struct imsic_global *imsic;

#define NR_INTP 1024

void imsic_init(void);
void imsic_handle(void);
void imsic_send_msi(unsigned long target_cpu_mask, uint32_t data);


#endif //_IMSIC_H_