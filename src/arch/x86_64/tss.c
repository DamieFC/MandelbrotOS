#include <mm/kheap.h>
#include <mm/pmm.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/gdt.h>
#include <sys/tss.h>
#include <printf.h>

tss_entry_t tss[256];

void tss_prepare_cpu(size_t cpu) {
  memset(&tss[cpu], 0, sizeof(tss_entry_t));

  tss[cpu].rsp[0] = (uint64_t)pmalloc(1) + PAGE_SIZE + PHYS_MEM_OFFSET;
  tss[cpu].ist[0] = tss[cpu].rsp[0];
  tss[cpu].iomap_offset = sizeof(tss_entry_t);

  asm volatile("cli\n"
               "ltr %%ax\n"
               :
               : "a"((5 + cpu) * 8));
}

int init_tss(size_t cpu_cnt) {
  for (size_t i = 0; i < cpu_cnt; i++) {
    uint64_t tss_base = (uint64_t)&tss[i];

    gdt.tss_entries[i] =
        (tss_gdt_entry_t){.limit_low = sizeof(tss_gdt_entry_t) & 0xffff,
                          .base_low = tss_base & 0xffff,
                          .base_mid = (tss_base & 0xff0000) >> 16,
                          .flags = 0x89,
                          .limit_high = (sizeof(tss_gdt_entry_t) & 0xf0000) >> 16,
                          .base_high = (tss_base & 0xff000000) >> 24,
                          .base_upper = tss_base >> 32,
                          .reserved = 0};
  }

  return 0;
}
