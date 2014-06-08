#include "res_kernel/stap_shim.h"
#include "res_common.h"
#include "costs.h"
#include <linux/rwlock_types.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/mm.h>

#define BUF_SIZE 4096  // need to think about this

struct syscall_acct_list_t {
  struct accounting *acct;
  unsigned long syscall_id;
  pid_t pid;
  int syscall_nr;
  struct syscall_acct_list_t *next;
};

struct free_accounting_pool {
  struct accounting *item;
  struct free_accounting_pool *next;
};

typedef struct syscall_acct_list_t syscall_acct_list_t;

static syscall_acct_list_t *syscall_acct_list;
static struct free_accounting_pool *acct_pool_free, *acct_pool_used;
static long syscall_id_c;
static char *buf;

static rwlock_t lock = __RW_LOCK_UNLOCKED(lock);
static spinlock_t free_acct_lock = __SPIN_LOCK_UNLOCKED(free_acct_lock);

static int rscfl_mmap(struct file *, struct vm_area_struct *);

static struct class *rscfl_class;

static struct file_operations fops =
{
  .mmap = rscfl_mmap,
};


static inline void return_to_pool (struct accounting *acct)
{
  struct free_accounting_pool *tmp;
  spin_lock(&free_acct_lock);
  BUG_ON(!(acct_pool_used));
  tmp = acct_pool_used;
  acct_pool_used = acct_pool_used->next;
  acct_pool_free = tmp;
  tmp->item = acct;
  spin_unlock(&free_acct_lock);
}


/**
 * Get memory to store the accounting in. Prefer reusing memory rather than
 * kmalloc-ing more.
 **/
static inline struct accounting * fetch_from_pool(void)
{
  struct accounting *acct;
  struct free_accounting_pool *tmp;
  spin_lock(&free_acct_lock);
  if (acct_pool_free) {
    tmp = acct_pool_free;
    acct = acct_pool_free->item;
    acct_pool_free = acct_pool_free->next;
    tmp->next = acct_pool_used;
    acct_pool_used = tmp;
    spin_unlock(&free_acct_lock);
  }
  else {
    tmp = kzalloc(sizeof(struct free_accounting_pool), GFP_KERNEL);
    if (!tmp) {
      spin_unlock(&free_acct_lock);
      return NULL;
    }
    tmp->next = acct_pool_used;
    acct_pool_used = tmp;
    /**
     * No need to lock on elements of the pool.
     */
    acct = (struct accounting *) kzalloc(sizeof(struct accounting),
					 GFP_KERNEL);
    if (!acct) {
      spin_unlock(&free_acct_lock);
      kfree(tmp);
      return NULL;
    }
    spin_unlock(&free_acct_lock);
  }
  return acct;
}

static int rscfl_mmap(struct file *filp, struct vm_area_struct *vma)
{
  unsigned long page;
  unsigned long pos;
  unsigned long size = (unsigned long)vma->vm_end-vma->vm_start;
  unsigned long start = (unsigned long)vma->vm_start;

  if (size > BUF_SIZE)
    return -EINVAL;

  buf = kmalloc(BUF_SIZE, GFP_KERNEL);
  if (!buf) {
    return -1;
  }

  pos = (unsigned long)buf;

  while (size) {
    page = virt_to_phys((void *)pos);
    /* fourth argument is the protection of the map. you might
     * want to use vma->vm_page_prot instead.
     */
    if (remap_pfn_range(vma, start, page >> PAGE_SHIFT, PAGE_SIZE, PAGE_SHARED))
      return -EAGAIN;
    start+=PAGE_SIZE;
    pos+=PAGE_SIZE;
    size-=PAGE_SIZE;
  }
  return 0;
}


int _create_shared_mem(void)
{
  int rc;
  rc = register_chrdev(90, RSCFL_DRIVER, &fops);
  rscfl_class = class_create(THIS_MODULE, RSCFL_DRIVER);
  device_create(rscfl_class, NULL, MKDEV(90, 0), NULL, RSCFL_DRIVER);
  if (rc < 0) {
    return rc;
  }

  return 0;
}

int _rscfl_shim_init(void)
{
  return _create_shared_mem();
}


int _clean_debugfs(void)
{
  device_destroy(rscfl_class, MKDEV(90, 0));
  class_unregister(rscfl_class);
  class_destroy(rscfl_class);
  unregister_chrdev(90, "rscfl2");
  return 0;
}

int _fill_struct(long cycles, long wall_clock_time, struct accounting *acct)
{
  debugk("_fill_struct\n");
  acct->cpu.cycles = cycles;
  acct->cpu.wall_clock_time = wall_clock_time;
  return 0;
}

/**
 * if finalised then all synchronous effects associated with acct have finished.
 * We therefore assume that there will be no further writes to it, and return
 * it to the pool.
 */
int _update_relay(struct accounting *acct, int finalised)
{
  struct free_accounting_pool *head;
  debugk("_update_relay\n");
  memcpy(buf, acct, sizeof(struct accounting));
  if (finalised) {
    // return the struct accounting to the free accounting pool
    return_to_pool(acct);
  }
  return 0;
}


/**
 * if syscall_nr==-1 then we account for the next syscall, independent of which
 * syscall is executed.
 **/
struct accounting * _should_acct(pid_t pid, int syscall_nr)
{
  syscall_acct_list_t *e;
  struct accounting *ret;

  read_lock(&lock);
  e = syscall_acct_list;
  while (e) {
    if ((e->pid == pid) &&
  ((syscall_nr == -1) || (e->syscall_nr == syscall_nr))) {
      ret = e->acct;
      ret->syscall_id.pid = pid;
      read_unlock(&lock);
      return ret;
    }
    e = e->next;
  }
  read_unlock(&lock);
  return NULL;
}

int acct_next(pid_t pid, int syscall_nr)
{
  syscall_acct_list_t *to_acct = (syscall_acct_list_t *)
    kzalloc(sizeof(syscall_acct_list_t), GFP_KERNEL);
  if (!to_acct) {
    return -1;
  }
  to_acct->syscall_id = syscall_id_c++;
  to_acct->pid = pid;
  to_acct->syscall_nr = syscall_nr;
  to_acct->next = syscall_acct_list;
  to_acct->acct = fetch_from_pool();

  if (!to_acct->acct) {
    kfree(to_acct);
    return -1;
  }
  write_lock(&lock);
  syscall_acct_list = to_acct;
  write_unlock(&lock);
  return 0;
}

/**
 * if syscall_nr==-1 then all resource consumption requests for the given pid
 * are cleared.
 *
 * if pid==-1 then syscall_nr will be cleared regardless of its associated pid
 *
 * if pid==-1 && syscall_nr==-1 then the resource consumption list is cleared
 **/
int _clear_acct_next(pid_t pid, int syscall_nr)
{
  syscall_acct_list_t *entry;
  syscall_acct_list_t *prev = NULL;
  syscall_acct_list_t *next;
  int rc = -1;

  read_lock(&lock);
  entry = syscall_acct_list;

  while (entry) {
    if (((syscall_nr == -1) || (syscall_nr == entry->syscall_nr)) &&
        ((pid == -1) || (pid = entry->pid)))
    {
      if (prev) {
        prev->next = entry->next;
      } else {
        syscall_acct_list = entry->next;
      }
      next = entry->next;
      kfree(entry);
      if (syscall_nr > 0) {
        read_unlock(&lock);
        return 0;
      }
      rc = 0;

      entry = next;

    } else {
      prev = entry;
      entry = entry->next;
    }
  }
  read_unlock(&lock);
  return rc;
}
