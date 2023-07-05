#ifndef APB_TIMER
#define APB_TIMER

#include <core.h>
#include <csrs.h>

/** (Testing) APB Timer */
#define TIMER_ADDR        (0x18000000)
#define APB_TIMER_IRQ     (5)
#define COUNTER_OFF       (0x0)
#define CONTROL_OFF       (0x4)
#define COMPARE_OFF       (0x8)
#define RESET_VAL         (0xC350) // for a 1ms timer

struct apb_timer {
    uint32_t counter;
    uint32_t control;
    uint32_t compare;
}__attribute__((__packed__, aligned(0x1000ULL)));

extern volatile struct apb_timer *cva6_timer;

void apb_timer_enable(void);
void apb_timer_disable(void);
void apb_timer_set_cmp(uint32_t new_cpm_val);
uint32_t apb_timer_get_cmp(void);
uint32_t apb_timer_get_counter(void);
void apb_timer_clr_counter(void);


#endif //APB_TIMER