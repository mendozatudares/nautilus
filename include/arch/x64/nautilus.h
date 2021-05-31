#ifdef __NAUTILUS_H__

#define DEBUG_PRINT(fmt, args...)					\
do {									\
    if (__cpu_state_get_cpu()) {					\
	int _p=preempt_is_disabled();					\
	preempt_disable();						\
	struct nk_thread *_t = get_cur_thread();				\
 	nk_vc_log_wrap("CPU %d (%s%s %lu \"%s\"): DEBUG: " fmt,		\
		       my_cpu_id(),					\
		       in_interrupt_context() ? "I" :"",		\
		       _p ? "" : "P",					\
		       _t ? _t->tid : 0,				\
		       _t ? _t->is_idle ? "*idle*" : _t->name[0]==0 ? "*unnamed*" : _t->name : "*none*", \
		       ##args);						\
	preempt_enable();						\
    } else {								\
	int _p=preempt_is_disabled();					\
	preempt_disable();						\
 	nk_vc_log_wrap("CPU ? (%s%s): DEBUG: " fmt,			\
		       in_interrupt_context() ? "I" :"",		\
		       _p ? "" : "P",					\
		       ##args);						\
	preempt_enable();						\
    }									\
} while (0)

#define ERROR_PRINT(fmt, args...)					\
do {									\
    if (__cpu_state_get_cpu()) {					\
	int _p=preempt_is_disabled();					\
	preempt_disable();						\
	struct nk_thread *_t = get_cur_thread();				\
 	nk_vc_log_wrap("CPU %d (%s%s %lu \"%s\"): ERROR at %s(%lu): " fmt,		\
		       my_cpu_id(),					\
		       in_interrupt_context() ? "I" :"",		\
		       _p ? "" : "P",					\
		       _t ? _t->tid : 0,					\
		       _t ? _t->is_idle ? "*idle*" : _t->name[0]==0 ? "*unnamed*" : _t->name : "*none*", \
	               __FILE__,__LINE__,                               \
		       ##args);						\
	preempt_enable();						\
    } else {								\
	int _p=preempt_is_disabled();					\
	preempt_disable();						\
 	nk_vc_log_wrap("CPU ? (%s%s): ERROR at %s(%lu): " fmt,			\
		       in_interrupt_context() ? "I" :"",		\
		       _p ? "" : "P",					\
                       __FILE__,__LINE__,                               \
		       ##args);						\
	preempt_enable();						\
    }									\
} while (0)


#define WARN_PRINT(fmt, args...)					\
do {									\
    if (__cpu_state_get_cpu()) {					\
	int _p=preempt_is_disabled();					\
	preempt_disable();						\
	struct nk_thread *_t = get_cur_thread();				\
 	nk_vc_log_wrap("CPU %d (%s%s %lu \"%s\"): WARNING : " fmt,	        \
		       my_cpu_id(),					\
		       in_interrupt_context() ? "I" :"",		\
		       _p ? "" : "P",					\
		       _t ? _t->tid : 0,					\
		       _t ? _t->is_idle ? "*idle*" : _t->name[0]==0 ? "*unnamed*" : _t->name : "*none*", \
		       ##args);						\
	preempt_enable();						\
    } else {								\
	int _p=preempt_is_disabled();					\
	preempt_disable();						\
 	nk_vc_log_wrap("CPU ? (%s%s): WARNING: " fmt,			\
		       in_interrupt_context() ? "I" :"",		\
		       _p ? "" : "P",					\
		       ##args);						\
	preempt_enable();						\
    }									\
} while (0)

#define INFO_PRINT(fmt, args...)					\
do {									\
    if (__cpu_state_get_cpu()) {					\
	int _p=preempt_is_disabled();					\
	preempt_disable();						\
	struct nk_thread *_t = get_cur_thread();				\
 	nk_vc_log_wrap("CPU %d (%s%s %lu \"%s\"): " fmt,	        \
		       my_cpu_id(),					\
		       in_interrupt_context() ? "I" :"",		\
		       _p ? "" : "P",					\
		       _t ? _t->tid : 0,					\
		       _t ? _t->is_idle ? "*idle*" : _t->name[0]==0 ? "*unnamed*" : _t->name : "*none*", \
		       ##args);						\
	preempt_enable();						\
    } else {								\
	int _p=preempt_is_disabled();					\
	preempt_disable();						\
 	nk_vc_log_wrap("CPU ? (%s%s): " fmt,       			\
		       in_interrupt_context() ? "I" :"",		\
		       _p ? "" : "P",					\
		       ##args);						\
	preempt_enable();						\
    }									\
} while (0)

#endif
