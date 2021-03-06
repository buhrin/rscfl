/**** Notice
 * wct_test.cpp: rscfl source code
 *
 * Copyright 2015-2017 The rscfl owners <lucian.carata@cl.cam.ac.uk>
 *
 * This file is part of the rscfl open-source project: github.com/lc525/rscfl;
 * Its licensing is governed by the LICENSE file at the root of the project.
 **/

#include <errno.h>
#include <fcntl.h>
#include "gtest/gtest.h"
#include <stdio.h>
#include <sys/socket.h>
#include <time.h>

#include <rscfl/costs.h>
#include <rscfl/res_common.h>
#include <rscfl/subsys_list.h>
#include <rscfl/user/res_api.h>

class WCTTest : public testing::Test
{
 protected:
  virtual void SetUp()
  {
    rhdl_ = rscfl_init();
    ASSERT_NE(nullptr, rhdl_);
  }

  rscfl_handle rhdl_;
};

static struct timespec wct_test_get_time(void)
{
  struct timespec ts;
  // We were originally using CLOCK_PROCESS_CPUTIME_ID but were occasionally
  // seeing strange (very small) values. By using CLOCK_MONOTONIC_RAW we're reading
  // a clock more similar to that of the kernel.
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  return ts;
}

/*
 * Tests whether the number of cycles seen in the kernel is less than that seen
 * in user space. when opening a socket.
 */
TEST_F(WCTTest, WallClock_Kernel_LT_User)
{
  ASSERT_EQ(0, rscfl_acct(rhdl_));

  struct timespec val_pre = wct_test_get_time();
  int sockfd = socket(AF_LOCAL, SOCK_RAW, 0);
  EXPECT_GT(sockfd, 0);
  struct timespec val_post = wct_test_get_time();
  rscfl_timespec_diff(&val_post, &val_pre);

  struct accounting acct;
  ASSERT_EQ(0, rscfl_read_acct(rhdl_, &acct));

  // Now add all of the subsystem times
  timespec kernel_time = {0, 0};
  int reduce_err = 0;
  reduce_err = REDUCE_SUBSYS(wc, rhdl_, &acct, 1, &kernel_time,
    [](subsys_accounting *s, rscfl_subsys id){ return &s->cpu.wall_clock_time;},
    rscfl_timespec_add);

  EXPECT_EQ(0, reduce_err);

  EXPECT_EQ(-1, rscfl_timespec_compare(&kernel_time, &val_post)) <<
    "expected (kernel_time) < (val_post) actual: (" <<
    kernel_time.tv_sec << " s, " << kernel_time.tv_nsec <<" ns) vs (" <<
    val_post.tv_sec << " s, " << val_post.tv_nsec <<" ns)";
}
