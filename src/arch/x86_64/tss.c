#include <mm/kheap.h>
#include <mm/pmm.h>
#include <printf.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/gdt.h>
#include <sys/tss.h>

tss_entry_t tss[256];

void tss_prepare_cpu(size_t cpu) {
  memset(&tss[cpu], 0, sizeof(tss_entry_t));

  tss[cpu].rsp0 = (uint64_t)pmalloc(1) + PAGE_SIZE + PHYS_MEM_OFFSET;
  tss[cpu].ist0 = (uint64_t)pmalloc(1) + PAGE_SIZE + PHYS_MEM_OFFSET;
  tss[cpu].iomap_offset = sizeof(tss_entry_t) - 1;

  gdt.tss_entries[cpu] = (tss_gdt_entry_t) {
    .length = 103,
    .base_low = (uint16_t)(uint64_t)&tss[cpu],
    .base_mid = (uint8_t)((uint64_t)&tss[cpu] >> 16),
    .base_high = (uint8_t)((uint64_t)&tss[cpu] >> 24),
    .flags1 = 0b10001001,
    .flags2 = 0,
    .base_upper = (uint32_t)((uint64_t)&tss[cpu] >> 32),
  };

  asm volatile("cli\n"
               "ltr %%ax\n"
               :
               : "a"((5 + cpu) << 3));

  load_gdt();
}

/* int init_tss(size_t cpu_cnt) { */
/* for (size_t i = 0; i < cpu_cnt; i++) { */
/* uint64_t tss_base = (uint64_t)&tss[i]; */

/* gdt.tss_entries[i] = */
/* (tss_gdt_entry_t){.limit = 104, */
/* .base_low = (uint16_t)tss_base, */
/* .base_mid = (uint8_t)(tss_base >> 16), */
/* .flags1 = 0b10001001, */
/* .flags2 = 0, */
/* .base_high = (uint8_t)(tss_base >> 24), */
/* .base_upper = (uint32_t)(tss_base >> 32), */
/* .reserved = 0}; */
/* } */

/* return 0; */
/* } */
