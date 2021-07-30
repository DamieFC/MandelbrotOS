#include <acpi/acpi.h>
#include <asm.h>
#include <boot/stivale2.h>
#include <cpu_locals.h>
#include <drivers/apic.h>
#include <drivers/pit.h>
#include <drivers/serial.h>
#include <fb/fb.h>
#include <lock.h>
#include <mm/kheap.h>
#include <mm/pmm.h>
#include <printf.h>
#include <registers.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/irq.h>
#include <tasking/scheduler.h>

#define ALIVE 0
#define DEAD 1

size_t current_tid = 0;
size_t current_pid = 0;

size_t thread_count = 0;
size_t proc_count = 0;

thread_t *current_thread;
proc_t *processes;

void k_idle() {
  while (1)
    ;
}

proc_t *get_proc_by_pid(size_t pid) {
  for (size_t i = 0; i < proc_count; i++) {
    if (processes->pid == pid)
      return processes;
    processes = processes->next;
  }
  return NULL;
}

thread_t *get_thread_by_pid(size_t tid) {
  thread_t *dup = current_thread;
  for (size_t i = 0; i < thread_count; i++) {
    if (dup->tid == tid)
      return dup;
    dup = dup->next;
  }
  return NULL;
}

size_t create_kernel_thread(uintptr_t addr, char *name, size_t pid) {
  thread_t *thread = kcalloc(sizeof(thread_t));
  proc_t *proc = get_proc_by_pid(pid);

  if (!thread || !proc)
    return -1;

  *thread = (thread_t){
      .tid = current_tid++,
      .exit_state = -1,
      .state = ALIVE,
      .name = name,
      .run_once = 0,
      .running = 0,
      .mother_proc = proc,
      .registers =
          (registers_t){
              .cs = 0x08,
              .ss = 0x10,
              .rip = (uint64_t)addr,
              .rflags = 0x202,
              .rsp = (uint64_t)pcalloc(1) + PAGE_SIZE + PHYS_MEM_OFFSET,
          },
      .next = current_thread->next,
      .prev = current_thread,
  };

  if (!thread->registers.rsp) {
    kfree(thread);
    return -1;
  }

  MAKE_LOCK(add_thread_lock);

  thread_count++;
  thread->next->prev = thread;
  current_thread->next = thread;

  proc->thread_count++;

  proc->tids[proc->thread_count++] = thread->tid;
  size_t *tids_re = kmalloc(sizeof(size_t) * (proc->thread_count + 1));
  memcpy(tids_re, proc->tids, (sizeof(size_t) * proc->thread_count));
  kfree(proc->tids);
  proc->tids = tids_re;

  UNLOCK(add_thread_lock);

  return thread->tid;
}

size_t create_user_thread(uintptr_t addr, char *name, size_t pid) {
  thread_t *thread = kcalloc(sizeof(thread_t));
  proc_t *proc = get_proc_by_pid(pid);

  if (!thread || !proc)
    return -1;

  *thread = (thread_t){
      .tid = current_tid++,
      .exit_state = -1,
      .state = ALIVE,
      .name = name,
      .run_once = 0,
      .running = 0,
      .mother_proc = proc,
      .registers =
          (registers_t){
              .cs = 0x1b,
              .ss = 0x23,
              .rip = (uint64_t)addr,
              .rflags = 0x202,
              .rsp = (uint64_t)pcalloc(1) + PAGE_SIZE + PHYS_MEM_OFFSET,
          },
      .next = current_thread->next,
      .prev = current_thread,
  };

  if (!thread->registers.rsp) {
    kfree(thread);
    return -1;
  }

  MAKE_LOCK(add_thread_lock);
  
  thread_count++;
  thread->next->prev = thread;
  current_thread->next = thread;

  proc->thread_count++;

  proc->tids[proc->thread_count++] = thread->tid;
  size_t *tids_re = kmalloc(sizeof(size_t) * (proc->thread_count + 1));
  memcpy(tids_re, proc->tids, (sizeof(size_t) * proc->thread_count));
  kfree(proc->tids);
  proc->tids = tids_re;

  UNLOCK(add_thread_lock);

  return thread->tid;
}

size_t create_proc(char *name) {
  proc_t *proc = kcalloc(sizeof(proc_t));

  if (!proc)
    return -1;

  proc->thread_count = 0;
  proc->name = name;
  proc->pid = current_pid++;
  proc->tids = kmalloc(sizeof(size_t));

  if (!proc->tids) {
    kfree(proc);
    return -1;
  }

  MAKE_LOCK(proc_lock);

  proc_count++;
  proc->next = processes->next;
  proc->next->prev = proc;
  proc->prev = processes;
  processes->next = proc;

  UNLOCK(proc_lock);

  return proc->pid;
}

void kill_thread(size_t tid) {
  thread_t *cp = current_thread;

  MAKE_LOCK(thread_kill_lock);

  for (size_t i = 0; i < thread_count; i++) {
    if (cp->tid == tid) {
      cp->mother_proc->thread_count--;

      kfree((void *)cp->registers.rsp);
      kfree(cp);

      cp->prev->next = cp->next;
      cp->next->prev = cp->prev;

      thread_count--;

      break;
    }
    cp = cp->next;
  }

  UNLOCK(thread_kill_lock);
}

void kill_proc(size_t pid) {
  MAKE_LOCK(proc_kill_lock);

  for (size_t i = 0; i < proc_count; i++) {
    if (processes->pid == pid) {
      for (size_t j = 0; j < processes->thread_count; j++)
        kill_thread(processes->tids[i]);

      kfree(processes->tids);
      kfree(processes);

      processes->prev->next = processes->next;
      processes->next->prev = processes->prev;

      proc_count--;

      break;
    }
    processes->prev->next = processes->next;
  }

  UNLOCK(proc_kill_lock);
}

void schedule(uint64_t rsp) {
  MAKE_LOCK(sched_lock);
  lapic_eoi();

  current_thread->running = 0;

  if (current_thread->run_once)
    memcpy(&current_thread->registers, (registers_t *)rsp, sizeof(registers_t));
  else
    current_thread->run_once = 1;

  thread_t *old_thread = current_thread;

  for (size_t i = 0; i < thread_count; i++) {
    current_thread = current_thread->next;
    if (current_thread->running == 0 && current_thread != old_thread &&
        current_thread->state == ALIVE)
      goto run_thread;
  }

  current_thread = old_thread;

run_thread:
  current_thread->running = 1;
  UNLOCK(sched_lock);

  asm volatile("mov %0, %%rsp\n"
               "pop %%r15\n"
               "pop %%r14\n"
               "pop %%r13\n"
               "pop %%r12\n"
               "pop %%r11\n"
               "pop %%r10\n"
               "pop %%r9\n"
               "pop %%r8\n"
               "pop %%rbp\n"
               "pop %%rdi\n"
               "pop %%rsi\n"
               "pop %%rdx\n"
               "pop %%rcx\n"
               "pop %%rbx\n"
               "pop %%rax\n"
               "add $16, %%rsp\n"
               "iretq\n"
               :
               : "r"(&current_thread->registers)
               : "memory");
}

void scheduler_init(uintptr_t addr) {
  processes = kcalloc(sizeof(proc_t));
  current_thread = kcalloc(sizeof(thread_t));

  *processes = (proc_t){
      .name = "kproc",
      .thread_count = 1,
      .tids = kmalloc(sizeof(size_t) * 2),
      .pid = current_pid++,
  };

  *current_thread = (thread_t){
      .tid = current_tid++,
      .exit_state = -1,
      .state = ALIVE,
      .name = "k_init",
      .run_once = 0,
      .running = 0,
      .mother_proc = processes,
      .registers =
          (registers_t){
              .cs = 0x08,
              .ss = 0x10,
              .rip = (uint64_t)addr,
              .rflags = 0x202,
              .rsp = (uint64_t)pcalloc(1) + PAGE_SIZE + PHYS_MEM_OFFSET,
          },
      .next = NULL,
  };

  proc_count++;
  processes->next = processes;
  processes->prev = processes;
  processes->tids[0] = current_thread->tid;

  thread_count++;
  current_thread->next = current_thread;
  current_thread->prev = current_thread;

  asm volatile("sti");
  irq_install_handler(SCHEDULE_REG - 32, schedule);

  while (1)
    ;
}
