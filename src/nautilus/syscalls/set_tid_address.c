#include <nautilus/nautilus.h>

#define SYSCALL_NAME "sys_set_tid_address"
#include "syscall_impl_preamble.h"

uint64_t sys_set_tid_address(uint32_t* tldptr) {
  nk_process_t* current_process = nk_process_current();

  get_cur_thread()->clear_child_tid = tldptr;

  return (uint64_t)nk_get_tid();
}