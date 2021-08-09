section .data
  string: db "Hello from ring 3! Going to run a restricted command."

section .text

extern user
extern printf

global jmp
global tss_reload

jmp:
  cli

  mov ax, (4 * 8) | 3
  mov ds, ax
  mov es, ax 
  mov fs, ax 
  mov gs, ax

  mov rax, rsp
  push (4 * 8) | 3
  push rax
  pushfq
  push (3 * 8) | 3
  lea rcx, [rel print]
  push rcx
  iretq

print:
  mov rdi, string
  call printf
  ret

tss_reload:
  ; push rbp
  ; mov rbp, rsp
  ; pushfq
  ; cli
  ; ltr di
  ; popfq
  ; pop rbp
  ret
