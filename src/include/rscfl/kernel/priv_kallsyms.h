#ifndef _RSCFL_PRIV_KALLSYMS_H_

#include <asm/pgtable_types.h>
#include <linux/mutex.h>
#include <linux/types.h>

#define KPRIV(name) name##_ptr

#ifdef _PRIV_KALLSYMS_IMPL_
  #define _once
#else
  #define _once extern
#endif

#define PRIV_KSYM_TABLE(_) \
_(text_poke)               \
_(text_mutex)              \
_(__vmalloc_node_range)    \
_(HYPERVISOR_shared_info)  \
_(xen_dummy_shared_info)   \
_(xen_evtchn_do_upcall)    \

_once void* (*KPRIV(text_poke))(void *addr, const void *opcode, size_t len);
_once void (*KPRIV(xen_evtchn_do_upcall))(struct pt_regs *regs);
_once struct mutex *KPRIV(text_mutex);
_once void* (*KPRIV(__vmalloc_node_range))(
    unsigned long size, unsigned long align, unsigned long start,
    unsigned long end, gfp_t gfp_mask, pgprot_t prot, int node,
    const void *caller);

_once struct shared_info **KPRIV(HYPERVISOR_shared_info);
_once struct shared_info *KPRIV(xen_dummy_shared_info);

int init_priv_kallsyms(void);


#endif /* _RSCFL_PRIV_KALLSYMS_H_ */
