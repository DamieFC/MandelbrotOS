#include <stdint.h>
#include <sys/gdt.h>

static gdt_pointer_t gdt_ptr;
gdt_t gdt;

void load_gdt() {
  asm volatile("lgdt %0" : : "m"(gdt_ptr) : "memory");
  asm volatile("mov %%rsp, %%rax\n"
               "push $0x10\n"
               "push %%rax\n"
               "pushf\n"
               "push $0x8\n"
               "push $reload_segments\n"
               "iretq\n"
               "reload_segments:\n"
               "mov $0x10, %%ax\n"
               "mov %%ax, %%ds\n"
               "mov %%ax, %%es\n"
               "mov %%ax, %%ss\n"
               "mov %%ax, %%fs\n"
               "mov %%ax, %%gs\n"
               :
               :
               : "rax", "memory");
}

int init_gdt() {
  gdt_ptr.limit = sizeof(gdt_t) - 1;
  gdt_ptr.base = (uint64_t)&gdt;

  gdt.entries[0] = (gdt_entry_t){.base_low = 0x0,
                                 .base_mid = 0x0,
                                 .base_high = 0x0,
                                 .limit_low = 0x0,
                                 .flags = 0x0,
                                 .granularity = 0x0}; // NULL descriptor

  gdt.entries[1] = (gdt_entry_t){.base_low = 0x0,
                                 .base_mid = 0x0,
                                 .base_high = 0x0,
                                 .limit_low = 0x0,
                                 .flags = 0x9a,
                                 .granularity = 0xa0}; // Kernel code descriptor

  gdt.entries[2] = (gdt_entry_t){.base_low = 0x0,
                                 .base_mid = 0x0,
                                 .base_high = 0x0,
                                 .limit_low = 0x0,
                                 .flags = 0x92,
                                 .granularity = 0xa0}; // Kernel data descriptor

  gdt.entries[3] = (gdt_entry_t){.base_low = 0x0,
                                 .base_mid = 0x0,
                                 .base_high = 0x0,
                                 .limit_low = 0x0,
                                 .flags = 0xfa,
                                 .granularity = 0xa0}; // User data descriptor

  gdt.entries[4] = (gdt_entry_t){.base_low = 0x0,
                                 .base_mid = 0x0,
                                 .base_high = 0x0,
                                 .limit_low = 0x0,
                                 .flags = 0xf2,
                                 .granularity = 0xa0}; // User data descriptor

  load_gdt();

  return 0;
}
