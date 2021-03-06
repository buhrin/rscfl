/**** Notice
 * xen.h: rscfl source code
 *
 * Copyright 2015-2017 The rscfl owners <lucian.carata@cl.cam.ac.uk>
 *
 * This file is part of the rscfl open-source project: github.com/lc525/rscfl;
 * Its licensing is governed by the LICENSE file at the root of the project.
 **/

#ifndef _RSCFL_XEN_H_
#define _RSCFL_XEN_H_

#include "rscfl/costs.h"

#ifdef _RSCFL_XEN_IMPL_
  #define _once
#else
  #define _once extern
#endif

#define NUM_XEN_PAGES 8
#define XEN_EVENTS_PER_PAGE 150

// TODO(oc243): We currently only use 1000 events rather than all of the
// available ones, due to Xen's setup.
#define CURRENT_XEN_NUM_EVENTS (NUM_XEN_PAGES * XEN_EVENTS_PER_PAGE)

/*
 * Data structures shared by rscfl-enabled xen
 */
struct sched_event
{
  uint64_t cycles;
  uint8_t credit;
  short guard;

  char is_yield;

  char is_block;
  char is_unblock;

  char sched_in;
  char sched_out;
};
typedef struct sched_event sched_event_t;

struct shared_sched_info
{
  uint64_t sched_out;
  uint16_t sched_tl;
  uint16_t sched_hd;
  unsigned long rscfl_page_phys[NUM_XEN_PAGES];
};

_once short disable_xen;
_once ru64 no_evtchn_events;
_once char *rscfl_pages[NUM_XEN_PAGES];

#undef _once

int xen_scheduler_init(void);
int xen_buffer_hd(void);
uint64_t xen_current_sched_out(void);
void xen_clear_current_sched_out(void);

#endif
