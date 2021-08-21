#include <stdint.h>
#include <sys/gdt.h>

static gdt_entry_t gdt[8];
static gdt_pointer_t gdt_ptr;

__attribute__ ((optimize((3)))) void load_gdt() {
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
  gdt_ptr.limit = sizeof(gdt) - 1;
  gdt_ptr.base = (uint64_t)gdt;

  gdt[0] = (gdt_entry_t){.low = 0x0,
                         .mid = 0x0,
                         .high = 0x0,
                         .limit = 0x0,
                         .access = 0x0,
                         .granularity = 0x0}; // NULL descriptor

  gdt[1] = (gdt_entry_t){.low = 0x0,
                         .mid = 0x0,
                         .high = 0x0,
                         .limit = 0x0,
                         .access = 0x9a,
                         .granularity = 0x20}; // Kernel code descriptor

  gdt[2] = (gdt_entry_t){.low = 0x0,
                         .mid = 0x0,
                         .high = 0x0,
                         .limit = 0x0,
                         .access = 0x92,
                         .granularity = 0x0}; // Kernel data descriptor

  gdt[3] = (gdt_entry_t){.low = 0x0,
                         .mid = 0x0,
                         .high = 0x0,
                         .limit = 0x0,
                         .access = 0xfa,
                         .granularity = 0x20}; // User data descriptor

  gdt[4] = (gdt_entry_t){.low = 0x0,
                         .mid = 0x0,
                         .high = 0x0,
                         .limit = 0x0,
                         .access = 0xf2,
                         .granularity = 0x0}; // User data descriptor

  load_gdt();

  return 0;
}
