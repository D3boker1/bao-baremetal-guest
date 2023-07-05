
#include <apb_timer.h>

volatile struct apb_timer *cva6_timer = (void*) TIMER_ADDR;

void apb_timer_enable(void){
    cva6_timer->control = 1;
}

void apb_timer_disable(void){
    cva6_timer->control = 0;
}

void apb_timer_set_cmp(uint32_t new_cpm_val){
    cva6_timer->compare = new_cpm_val;
}
void apb_timer_clr_counter(){
    cva6_timer->counter = 0;
}
uint32_t apb_timer_get_cmp(void){
    return cva6_timer->compare;
}

uint32_t apb_timer_get_counter(void){
    return cva6_timer->counter; 
}
