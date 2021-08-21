#ifndef __LOCK_H__
#define __LOCK_H__

#include <stdint.h>

// Based on lyre-os' spinlock system. 

typedef struct {
  uint32_t waiting_refcount;
  uint32_t bits;
} lock_t;

#define LOCKED_READ(VAR)                                                       \
  ({                                                                           \
    typeof(VAR) ret = 0;                                                       \
    asm volatile("lock xadd %0, %1" : "+r"(ret), "+m"(VAR) : : "memory");      \
    ret;                                                                       \
  })

#define LOCKED_WRITE(VAR, VAL)                                                 \
  ({                                                                           \
    typeof(VAR) ret = VAL;                                                     \
    asm volatile("lock xchg %0, %1" : "+r"(ret), "+m"(VAR) : : "memory");      \
    ret;                                                                       \
  })

#define LOCKED_INC(VAR)                                                        \
  ({                                                                           \
    int ret;                                                                   \
    asm volatile("lock incl %1" : "=@ccnz"(ret) : "m"(VAR) : "memory");        \
    ret;                                                                       \
  })

#define LOCKED_DEC(VAR)                                                        \
  ({                                                                           \
    int ret;                                                                   \
    asm volatile("lock decl %1" : "=@ccnz"(ret) : "m"(VAR) : "memory");        \
    ret;                                                                       \
  })

#define SPINLOCK_ACQUIRE(LOCK)                                                 \
  ({                                                                           \
    int ret;                                                                   \
    LOCKED_INC((LOCK).waiting_refcount);                                       \
    asm volatile("1: lock btsl $0, %0\n\t"                                     \
                 "jnc 1f\n\t"                                                  \
                 "btl $1, %0\n\t"                                              \
                 "jc 1f\n\t"                                                   \
                 "pause\n\t"                                                   \
                 "jmp 1b\n\t"                                                  \
                 "1:"                                                          \
                 : "+m"((LOCK).bits), "=@ccc"(ret)                             \
                 :                                                             \
                 : "memory");                                                  \
    LOCKED_DEC((LOCK).waiting_refcount);                                       \
    !ret;                                                                      \
  })

#define LOCK_ACQUIRE(LOCK)                                                     \
  ({                                                                           \
    int ret;                                                                   \
    asm volatile("lock btsl $0, %0"                                            \
                 : "+m"((LOCK).bits), "=@ccc"(ret)                             \
                 :                                                             \
                 : "memory");                                                  \
    !ret;                                                                      \
  })

#define LOCK_RELEASE(LOCK)                                                     \
  ({ asm volatile("lock btrl $0, %0" : "+m"((LOCK).bits) : : "memory"); })

#define DECLARE_LOCK(name) static volatile lock_t name

#define LOCK(name) SPINLOCK_ACQUIRE(name)

#define UNLOCK(name) LOCK_RELEASE(name)

#define MAKE_LOCK(name)                                                        \
  DECLARE_LOCK(name);                                                          \
  LOCK(name);

#endif
