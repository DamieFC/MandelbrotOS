#ifndef __LOCK_H__
#define __LOCK_H__

// These are spinlocks. Not mutexs.

#define DECLARE_LOCK(name) static volatile int name

#define LOCK(name)                                                             \
  while (!__sync_bool_compare_and_swap(&name, 0, 1))                           \
    ;

#define UNLOCK(name) __sync_lock_release(&name)

#define MAKE_LOCK(name)                                                        \
  DECLARE_LOCK(name);                                                          \
  LOCK(name);

#endif
