/**** Notice
 * subsys.h: rscfl source code
 *
 * Copyright 2015-2017 The rscfl owners <lucian.carata@cl.cam.ac.uk>
 *
 * This file is part of the rscfl open-source project: github.com/lc525/rscfl;
 * Its licensing is governed by the LICENSE file at the root of the project.
 **/

#ifndef _RSCFL_SUBSYS_H_
#define _RSCFL_SUBSYS_H_

#include "rscfl/costs.h"

int rscfl_subsys_entry(rscfl_subsys);

void rscfl_subsys_exit(rscfl_subsys);

int get_subsys(rscfl_subsys subsys_id,
               struct subsys_accounting **subsys_acct_ret);

#endif
