#ifndef __KERNEL_MAIN_H__
#define __KERNEL_MAIN_H__

#include <boot/stivale2.h>

extern uint8_t stack[];

void kernel_main(struct stivale2_struct *bootloader_info);

#endif
