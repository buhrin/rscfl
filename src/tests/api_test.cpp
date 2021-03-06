/**** Notice
 * api_test.cpp: rscfl source code
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
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <rscfl/costs.h>
#include <rscfl/subsys_list.h>
#include <rscfl/user/res_api.h>

class APITest : public testing::Test
{
 protected:
  virtual void SetUp()
  {
    int bind_err = 0;
    one_acct_ = NULL;
    subsys_agg_ = NULL;

    cfg.monitored_pid = RSCFL_PID_SELF;
    cfg.kernel_agg = 0;

    rhdl_ = rscfl_init(&cfg);
    ASSERT_NE(nullptr, rhdl_);
    ASSERT_EQ(0, rscfl_acct(rhdl_, NULL, ACCT_DEFAULT));

    int sockfd_ = socket(PF_LOCAL, SOCK_RAW, 0);
    EXPECT_LE(0, sockfd_);
    ASSERT_EQ(0, rscfl_read_acct(rhdl_, &acct_));

    local_addr.sun_family = AF_LOCAL;
    strcpy(local_addr.sun_path, "/tmp/rscfl_test_sock");
    unlink(local_addr.sun_path);
    int len = strlen(local_addr.sun_path) +  sizeof(local_addr.sun_family);

    ASSERT_EQ(0, rscfl_acct(rhdl_));
    bind_err = bind(sockfd_, (sockaddr *) &local_addr, len);
    EXPECT_LE(0, bind_err);

    ASSERT_EQ(0, rscfl_read_acct(rhdl_, &acct2_));

    subsys_agg_ = NULL;
    one_acct_   = NULL;

  }

  virtual void TearDown()
  {
    close(sockfd_);
    unlink(local_addr.sun_path);
    rscfl_subsys_free(rhdl_, &acct_);
    rscfl_subsys_free(rhdl_, &acct2_);
    free_subsys_idx_set(one_acct_);
    free_subsys_idx_set(subsys_agg_);
  }

  rscfl_handle rhdl_;
  int sockfd_;
  rscfl_config cfg;
  sockaddr_un local_addr;
  struct accounting acct_;
  struct accounting acct2_;
  subsys_idx_set *subsys_agg_;
  subsys_idx_set *one_acct_;

};


TEST_F(APITest,
       UserGetSubsysforOneAcct)
{
  int no_subsys_in_idx = 0;
  one_acct_ = rscfl_get_subsys(rhdl_, &acct_);
  ASSERT_NE(nullptr, one_acct_);

  // make sure all subsystems were transferred
  EXPECT_EQ(one_acct_->set_size, acct_.nr_subsystems);

  // for non-aggregation subsys_idx_set, check that set_size == max_set_size
  EXPECT_EQ(one_acct_->set_size, one_acct_->max_set_size);

  // check that the index actually contains exactly set_size elements != -1
  for(int i = 0; i < NUM_SUBSYSTEMS; ++i) {
    if(one_acct_->idx[i] != -1) no_subsys_in_idx++;
  }
  EXPECT_EQ(one_acct_->set_size, no_subsys_in_idx);

  // check that the set actually contains subsystems with data
  for(int i = 0; i < one_acct_->set_size; ++i) {
    if (one_acct_->ids[i] != USERSPACE_XEN) {
      EXPECT_NE(0, one_acct_->set[i].cpu.cycles);
    }
  }
}

TEST_F(APITest,
       SubsysIndexAndIdArraysAreConsistent)
{
  int no_subsys_in_idx = 0;
  one_acct_ = rscfl_get_subsys(rhdl_, &acct_);
  ASSERT_NE(nullptr, one_acct_);

  // check that the index and id's array data match
  // constraint: idx(id(i)) == i
  for(int i = 0; i < one_acct_->set_size; ++i) {
    EXPECT_EQ(i, one_acct_->idx[one_acct_->ids[i]]);
  }
}

TEST_F(APITest,
       UserAcctAggregators)
{
  subsys_agg_ = rscfl_get_new_aggregator(7);
  EXPECT_TRUE(subsys_agg_ != NULL);

  int err = rscfl_merge_acct_into(rhdl_, &acct_, subsys_agg_);
  EXPECT_EQ(0, err);

  // sum cycles for all subsystems
  ru64 sum_cycles = 0;
  for(int i = 0; i < subsys_agg_->set_size; ++i) {
    sum_cycles += subsys_agg_->set[i].cpu.cycles;
  }
  EXPECT_LT(0, sum_cycles);

  // after merge, cycles should be greater than before
  err = rscfl_merge_acct_into(rhdl_, &acct2_, subsys_agg_);
  EXPECT_EQ(0, err);

  ru64 sum_cycles2 = 0;
  for(int i = 0; i < subsys_agg_->set_size; ++i) {
    sum_cycles2 += subsys_agg_->set[i].cpu.cycles;
  }
  EXPECT_LT(sum_cycles, sum_cycles2);

}

TEST_F(APITest,
       ReduceAPIProducesCorrectResults)
{
  ru64 kernel_cycles_;
  ru64 kernel_cycles_r_ = 0;
  int r_err = 0;

  // select cpu.cycles from all subsystems of a given acct and reduce
  // them to one value (their sum)
  r_err = REDUCE_SUBSYS(rint, rhdl_, &acct_, 0, &kernel_cycles_r_,
    [](subsys_accounting *s, rscfl_subsys id){ return &s->cpu.cycles; },
    [](ru64 *acct, const ru64 *elem){ *acct += *elem; });

  ASSERT_EQ(0, r_err);
  ASSERT_NE(0, kernel_cycles_r_);

  struct subsys_accounting *subsys;
  rscfl_subsys curr_sub;
  kernel_cycles_ = 0;
  for (int i = 0; i < NUM_SUBSYSTEMS; ++i) {
    curr_sub = (rscfl_subsys)i;
    if ((subsys = rscfl_get_subsys_by_id(rhdl_, &acct_, curr_sub)) != NULL) {
      kernel_cycles_ += subsys->cpu.cycles;
    }
  }

  EXPECT_EQ(kernel_cycles_, kernel_cycles_r_);
}

TEST_F(APITest, MergeHypervisorMinCreditsGivesSmallestValue)
{
  struct subsys_accounting smaller = {0};
  struct subsys_accounting larger = {0};

  smaller.sched.xen_credits_min = 10;
  larger.sched.xen_credits_min = 20;
  rscfl_subsys_merge(&larger, &smaller);
  // We should now have credit 10 as the min number of credits.
  ASSERT_EQ(10, larger.sched.xen_credits_min);
}

TEST_F(APITest, SubsequentGetTokensHaveUniqueValues)
{
  rscfl_token *token_a;
  rscfl_token *token_b;
  ASSERT_EQ(0, rscfl_get_token(rhdl_, &token_a));
  ASSERT_EQ(0, rscfl_get_token(rhdl_, &token_b));

  // Make sure that the tokens have different IDs.
  ASSERT_NE(token_a->id, token_b->id);
}

TEST_F(APITest, MultipleGetTokensSucceedOrFailGracefully)
{
  int rc;
  rscfl_token *token;
  for (int i = 0; i < 20; i++) {
    rc = rscfl_get_token(rhdl_, &token);
    if ((rc != 0 ) && (rc != -EAGAIN)) {
      // Is there a better way to do this?
      ASSERT_TRUE(false);
    }

  }
}

/*
 * We reserve token 0 for times where we don't want to tokenize Rscfl.
 * Make sure that we don't get assigned it
 */
TEST_F(APITest, TokenIDNotZero)
{
  int rc;
  rscfl_token *token;
  for (int i = 0; i < 20; i++) {
    rc = rscfl_get_token(rhdl_, &token);
    ASSERT_EQ(rc, 0);
    ASSERT_NE(0, token->id) << "token id=" << token->id;
  }
}
