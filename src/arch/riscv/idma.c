#include "idma.h"

void idma_config_single_transfer(struct idma *dma_ut, uint64_t src_addr, uint64_t dest_addr)
{
  dma_ut->src_addr    = src_addr;   // Source address
  dma_ut->dest_addr   = dest_addr;  // Destination address
  dma_ut->num_bytes   = 8;          // N of bytes to be transferred
  dma_ut->config      = (1<<3);          // iDMA config: Disable decouple, deburst and serialize
}

uint64_t idma_get_last_completed_transac(struct idma *dma_ut){
  return dma_ut->last_transfer_id_complete;
}