#ifndef _PTHREAD_DEBUG
#define _PTHREAD_DEBUG

#define BOGUS_FUN_ERR() ERROR_PRINT("Function (%s) is BOGUS\n", __func__)
#define BOGUS() BOGUS_FUN_ERR()

#define ERROR(fmt, args...)  ERROR_PRINT("pthread: " fmt, ##args)
#define INFO(fmt, args...)   INFO_PRINT("pthread: " fmt, ##args)

#ifdef NAUT_CONFIG_DEBUG_BASE_PTHREAD_COMPAT
#define DEBUG(fmt, args...) DEBUG_PRINT("pthread: " fmt, ##args)
#else
#define DEBUG(fmt, args...)
#endif

#endif

