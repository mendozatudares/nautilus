#include <nautilus/nautilus.h>
#include "pthread.h"
#include "implement.h"

pte_thread_t * nk_get_pte_thread_t(){

  nk_thread_t *cur = get_cur_thread();
  return *((pte_thread_t **) cur->hwtls+8);

}
