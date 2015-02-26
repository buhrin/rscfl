#include "rscfl/kernel/measurement.h"

#include "linux/nmi.h"
#include "linux/time.h"

#include "rscfl/kernel/cpu.h"
#include "rscfl/kernel/perf.h"
#include "rscfl/res_common.h"

/*
 * Some extra, useful counters
 */
static __inline__ ru64 rscfl_get_cycles(void)
{
  unsigned int hi, lo;
  __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return ((ru64)hi << 32) | lo;
}

static struct timespec rscfl_get_timestamp(void)
{
  struct timespec ts;
  getrawmonotonic(&ts);
  return ts;
}

/*
 * This is where we'd init the Perf counters if we were using them.
 */
int rscfl_counters_init(void)
{
  return 0;
}

void rscfl_counters_stop(void)
{
}

int rscfl_counters_update_subsys_vals(struct subsys_accounting *add_subsys,
                                      struct subsys_accounting *minus_subsys)
{
  pid_acct *curr_pid;
  struct accounting *acct;

  u64 cycles = rscfl_get_cycles();
  struct timespec time = rscfl_get_timestamp();

  // Update the WALL CLOCK TIME and CYCLES
  if (add_subsys != NULL) {
    add_subsys->subsys_entries++;
    add_subsys->cpu.cycles += cycles;
    rscfl_timespec_add(&add_subsys->cpu.wall_clock_time, &time);
  }

  if (minus_subsys != NULL) {
    minus_subsys->subsys_exits++;
    minus_subsys->cpu.cycles -= cycles;
    rscfl_timespec_diff(&minus_subsys->cpu.wall_clock_time, &time);
  }

  // Check and see if any scheduling happened underneath us. If there was,
  // update.
  // LOCAL
  if (add_subsys != NULL) {
    curr_pid = CPU_VAR(current_acct);
    acct = curr_pid->probe_data->syscall_acct;
    if (acct != NULL) {
      if (acct->wct_out_temp.tv_sec != 0 || acct->wct_out_temp.tv_nsec
        != 0) {
          rscfl_timespec_add(&add_subsys->sched.wct_out_local, &acct->wct_out_temp);
      }
    }
  }

  // HYPERVISOR

  // Here we'd snapshot the Perf counters, but since they're unused at the
  // moment, we simply return.
  return 0;
}

