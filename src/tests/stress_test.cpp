/**** Notice
 * stress_test.cpp: rscfl source code
 *
 * Copyright 2015-2017 The rscfl owners <lucian.carata@cl.cam.ac.uk>
 *
 * This file is part of the rscfl open-source project: github.com/lc525/rscfl;
 * Its licensing is governed by the LICENSE file at the root of the project.
 **/

#include <errno.h>
#include "gtest/gtest.h"
#include <stdio.h>
#include <sys/socket.h>

#include <rscfl/costs.h>
#include <rscfl/user/res_api.h>

class StressTest : public testing::Test
{

 protected:
  virtual void SetUp()
  {
    cfg.monitored_pid = RSCFL_PID_SELF;
    cfg.kernel_agg = 0;

    rhdl_ = rscfl_init(&cfg);
    ASSERT_NE(nullptr, rhdl_);
  }

  rscfl_handle rhdl_;
  rscfl_config cfg;
  struct accounting acct_;
};


// Try creating, and closing 1000 sockets, and accounting for all syscalls.
// We use ASSERTS as when the test fails with an EXPECT, we don't want to flood
// stdout.
TEST_F(StressTest, TestAcctForAThousandSocketOpens)
{
  int sock_fd;
  for (int i = 0; i < 1000; i++) {
    // Account for opening a socket.
    ASSERT_EQ(0, rscfl_acct(rhdl_));
    sock_fd = socket(AF_LOCAL, SOCK_RAW, 0);
    ASSERT_GT(sock_fd, 0);
    // Ensure that we are able to read back the struct accounting.
    ASSERT_EQ(0, rscfl_read_acct(rhdl_, &acct_))
        << "Failed at accounting for socket creation at attempt" << i;
    rscfl_subsys_free(rhdl_, &acct_);

    // Account for closing the socket again.
    ASSERT_EQ(0, rscfl_acct(rhdl_));
    close(sock_fd);
    ASSERT_EQ(0, rscfl_read_acct(rhdl_, &acct_))
        << "Failed at closing socket at attempt " << i;
    rscfl_subsys_free(rhdl_, &acct_);
  }
}
