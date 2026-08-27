/*************************************************************************
 * Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * See LICENSE.txt for license information
 ************************************************************************/
//
// Host-only microtests for ncclGroupEndInternal's blocking-vs-non-blocking
// path selection (src/group.cc).
//
// These pin the contract that a non-blocking group (config.blocking == 0) with
// pending async init jobs hands the work to a background thread and returns
// ncclInProgress immediately -- it must NOT run the jobs synchronously on the
// caller. That is exactly the guarantee ncclCommInitRankConfig(blocking=0)
// relies on so a caller can poll / time out / abort when a peer rank never
// shows up.
//
// The suite compiles the hipified src/group.cc directly (via GROUP_CC_PATH) so
// the file-static launch helpers are reachable, and satisfies every external
// group.cc references with the module-organised test doubles in fakes/
// (comm_fakes, collective_stubs, ...). No GPU, no librccl.so, no
// HIP runtime.

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "nccl.h"
#include "comm.h"
#include "group.h"

#include "fakes/comm_fakes.h"    // controllable ncclCommSetAsyncError seam
#include "fakes/nccl_fakes.h"    // g_loadParam, used by param_redirect.h

// Route group.cc's NCCL_PARAM sites through g_loadParam instead of the real
// ncclLoadParam (see param_redirect.h); must precede the unit under test.
#include "fakes/param_redirect.h"

// Pull in the unit under test so its file-static helpers (groupLaunch,
// asyncJobLaunch, groupLaunchNonBlocking, ...) are visible. Must come after the
// headers / fakes it depends on are in scope.
#include GROUP_CC_PATH

namespace {

// A job body that succeeds instantly, standing in for a real communicator-init
// job. The behaviour under test is which *thread* runs this (background vs
// caller), not what the job itself does.
ncclResult_t FakeInitJobSucceeds(struct ncclAsyncJob*) { return ncclSuccess; }

class GroupEndInternalTest : public ::testing::Test {
 protected:
  std::unique_ptr<ncclComm> comm_;
  std::unique_ptr<ncclAsyncJob> job_;
  // Backing storage for comm_->memScoped so ncclGroupCommJoin's push takes the
  // in-hunk fast path instead of spilling to ncclMemoryStack::allocateSpilled
  // (which the host-only link deliberately omits).
  std::vector<char> memScopedBuf_;

  // Records every ncclCommSetAsyncError state pushed to comm_.
  std::vector<ncclResult_t> asyncStates_;

  void SetUp() override {
    ResetCommFakes();

    // Reset the thread-local group state group.cc reads. ncclGroupEndInternal
    // runs on this thread, so the thread-locals it inspects are these.
    ncclGroupDepth = 0;
    ncclGroupError = ncclSuccess;
    for (int type = 0; type < ncclGroupTaskTypeNum; ++type) ncclGroupCommHead[type] = nullptr;
    ncclGroupCommPreconnectHead = nullptr;
    ncclGroupBlocking = -1;
    ncclIntruQueueConstruct(&ncclAsyncJobs);

    comm_ = std::make_unique<ncclComm>();  // value-initialised => zeroed
    comm_->groupJob = nullptr;
    // A comm not currently in a group carries the 0x1 "not joined" sentinel in
    // each groupNext slot; ncclGroupCommJoin keys off it to decide to insert.
    for (int type = 0; type < ncclGroupTaskTypeNum; ++type)
      comm_->groupNext[type] = reinterpret_cast<struct ncclComm*>(0x1);
    // groupLaunch's cleanup pops comm->memScoped (ncclGroupCommLeave), balancing
    // the push ncclGroupCommJoin does; construct it over a fixture-owned buffer
    // so the push allocates in-hunk and never reaches allocateSpilled.
    memScopedBuf_.assign(1 << 16, 0);
    ncclMemoryStackConstruct(&comm_->memScoped);
    comm_->memScoped.topFrame.bumper = reinterpret_cast<uintptr_t>(memScopedBuf_.data());
    comm_->memScoped.topFrame.end =
        reinterpret_cast<uintptr_t>(memScopedBuf_.data() + memScopedBuf_.size());

    g_commSetAsyncError = [this](struct ncclComm*, ncclResult_t state) {
      asyncStates_.push_back(state);
      return ncclSuccess;
    };
  }

  void TearDown() override {
    // If a non-blocking init spawned a background job, join it and release the
    // group job before the fixture (and its comm) go away.
    if (comm_ && comm_->groupJob) {
      ncclGroupJobComplete(comm_->groupJob);
      comm_->groupJob = nullptr;
    }
    ResetCommFakes();
  }

  // Queue a single pending async job owned by comm_ and enter a group whose
  // blocking mode is `blocking`, mirroring the state ncclCommInitRankConfig
  // leaves behind just before ncclGroupEnd.
  void EnterGroupWithOnePendingJob(int blocking) {
    comm_->config.blocking = blocking;

    job_ = std::make_unique<ncclAsyncJob>();
    job_->func = FakeInitJobSucceeds;
    job_->comm = comm_.get();
    job_->state = ncclGroupJobRunning;

    ncclGroupStartInternal();  // ncclGroupDepth = 1
    ncclGroupBlocking = blocking;
    ncclIntruQueueEnqueue(&ncclAsyncJobs, job_.get());
  }

  // Enter a group carrying only a comm on a task chain -- no pending async init
  // jobs, no preconnect. This is the collective-only group shape: work to launch
  // lives on ncclGroupCommHead, not on the ncclAsyncJobs queue. The comm joins
  // the (lighter) symmetric-register chain, whose launch over empty task queues
  // is a no-op, so the group's path selection can be observed without standing
  // up the full collective launch pipeline.
  void EnterGroupWithOneCommHeadNoAsyncJob(int blocking) {
    comm_->config.blocking = blocking;

    ncclGroupStartInternal();  // ncclGroupDepth = 1
    ncclGroupCommJoin(comm_.get(), ncclGroupTaskTypeSymRegister);
    ncclGroupBlocking = blocking;
  }
};

// The fix: a non-blocking group carrying pending init work must return
// ncclInProgress (handing the work to a background thread) rather than running
// it on the caller. Before the fix, ncclGroupEndInternal tested the already-
// drained ncclAsyncJobs queue, mis-selected the blocking path, and ran the jobs
// synchronously -- which for a missing peer rank blocks forever.
TEST_F(GroupEndInternalTest, NonBlockingGroupWithPendingJob_ReturnsInProgress) {
  EnterGroupWithOnePendingJob(/*blocking=*/0);

  EXPECT_EQ(ncclInProgress, ncclGroupEndInternal());
}

// The non-blocking path also publishes a group job on the communicator, which
// is what lets the caller subsequently poll or abort it.
TEST_F(GroupEndInternalTest, NonBlockingGroupWithPendingJob_PublishesGroupJobOnComm) {
  EnterGroupWithOnePendingJob(/*blocking=*/0);

  (void)ncclGroupEndInternal();

  EXPECT_NE(nullptr, comm_->groupJob);
}

// A non-blocking group whose only work is a comm on a task chain (no pending
// async init jobs, no preconnect) must also hand off to a background thread and
// return ncclInProgress -- the path selection depends only on config.blocking,
// not on whether the work happens to sit on the async-jobs queue. RCCL long
// carried an extra `&& (preconnectHead || !asyncJobs.empty())` term on this gate
// that upstream NCCL dropped at v2.30.7; with it, this comm-head-only group
// wrongly fell through to the blocking path and ran synchronously on the caller.
TEST_F(GroupEndInternalTest, NonBlockingGroupWithCommHeadOnly_ReturnsInProgress) {
  EnterGroupWithOneCommHeadNoAsyncJob(/*blocking=*/0);

  EXPECT_EQ(ncclInProgress, ncclGroupEndInternal());
}

// And, as with the async-job case, the non-blocking comm-head path publishes a
// group job on the communicator so the caller can poll or abort it.
TEST_F(GroupEndInternalTest, NonBlockingGroupWithCommHeadOnly_PublishesGroupJobOnComm) {
  EnterGroupWithOneCommHeadNoAsyncJob(/*blocking=*/0);

  (void)ncclGroupEndInternal();

  EXPECT_NE(nullptr, comm_->groupJob);
}

// Contrast: a blocking group runs its jobs on the caller and reports the final
// result (ncclSuccess here), leaving no group job to poll.
TEST_F(GroupEndInternalTest, BlockingGroupWithPendingJob_RunsSynchronouslyAndReturnsSuccess) {
  EnterGroupWithOnePendingJob(/*blocking=*/1);

  EXPECT_EQ(ncclSuccess, ncclGroupEndInternal());
  EXPECT_EQ(nullptr, comm_->groupJob);
}

}  // namespace
