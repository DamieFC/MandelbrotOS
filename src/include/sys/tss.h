#ifndef __TSS_H__
#define __TSS_H__

#include <stddef.h>
#include <stdint.h>

typedef struct tss_entry {
  uint32_t reserved;
  uint64_t rsp0;
  uint64_t rsp1;
  uint64_t rsp2;
  uint64_t reserved0;
  uint64_t ist0;
  uint64_t ist1;
  uint64_t ist2;
  uint64_t ist3;
  uint64_t ist4;
  uint64_t ist5;
  uint64_t ist6;
  uint64_t reserved1;
  uint16_t reserved2;
  uint16_t iomap_offset;
} __attribute__((packed)) tss_entry_t;

void tss_reload(uint16_t selector);
void tss_prepare_cpu(size_t cpu);
// int init_tss(size_t cpu_cnt);

#endif
