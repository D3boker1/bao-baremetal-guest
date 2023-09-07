#ifndef IDMA_H
#define IDMA_H

#include <core.h>
#include <page_tables.h>

#define TEST_PPAGE_BASE (MEM_BASE+(MEM_SIZE/2))

// Base address of the iDMA programming interface
#define IDMA_BASE_ADDR             0x50000000ULL

// Register offsets
#define IDMA_SRC_ADDR_OFF   0x0
#define IDMA_DEST_ADDR_OFF  0x8
#define IDMA_N_BYTES_OFF    0x10
#define IDMA_CONFIG_OFF     0x18
#define IDMA_STATUS_OFF     0x20
#define IDMA_NEXT_ID_OFF    0x28
#define IDMA_DONE_OFF       0x30
#define IDMA_IPSR_OFF       0x38

#define IDMA_DECOUPLE   (1ULL << 0)
#define IDMA_DEBURST    (1ULL << 1)
#define IDMA_SERIALIZE  (1ULL << 2)

struct idma {
  uint64_t src_addr;
  uint64_t dest_addr;
  uint64_t num_bytes;
  uint64_t config;
  uint64_t status;
  uint64_t next_transfer_id;
  uint64_t last_transfer_id_complete;
}__attribute__((__packed__, aligned(PAGE_SIZE)));

void idma_config_single_transfer(struct idma *dma_ut, uint64_t src_addr, uint64_t dest_addr);

static inline uintptr_t get_addr_base(size_t page_index){
  return TEST_PPAGE_BASE+(page_index*PAGE_SIZE);
}

#endif