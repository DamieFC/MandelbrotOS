#ifndef __GDT_H__
#define __GDT_H__

#include <stdint.h>

typedef struct gdt_pointer {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed)) gdt_pointer_t;

typedef struct gdt_entry {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_mid;
  uint8_t flags;
  uint8_t granularity;
  uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct tss_gdt_entry {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_mid;
  uint8_t flags;
  uint8_t limit_high;
  uint8_t base_high;
  uint32_t base_upper;
  uint32_t reserved;
} __attribute__((packed)) tss_gdt_entry_t;

typedef struct gdt {
  gdt_entry_t entries[5];
  tss_gdt_entry_t tss_entries[256];
} __attribute__((packed)) gdt_t;

extern gdt_t gdt;

void load_gdt();
int init_gdt();

#endif
