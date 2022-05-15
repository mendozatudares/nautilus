
#ifndef _OS_SUPPORT_H_
#define _OS_SUPPORT_H_

// Platform specific one must be included first
#include "nk/pte_osal.h"

#include <autoconf.h>
#include <nautilus/nautilus.h>

#define printf(...) nk_vc_printf(__VA_ARGS__)

#include "pte_generic_osal.h"



#endif // _OS_SUPPORT_H
