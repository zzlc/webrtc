/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include <memory>

#include "api/transport/network_types.h"
#include "modules/congestion_controller/goog_cc/probe_controller.h"
#include "rtc_base/logging.h"
#include "system_wrappers/include/clock.h"
#include "test/gmock.h"
#include "test/gtest.h"

using testing::_;
using testing::AtLeast;
using testing::Field;
using testing::Matcher;
using testing::NiceMock;
using testing::Return;

namespace webrtc {
namespace webrtc_cc {
namespace test {

namespace {

constexpr int kMinBitrateBps = 100;
constexpr int kStartBitrateBps = 300;
constexpr int kMaxBitrateBps = 10000;

constexpr int kExponentialProbingTimeoutMs = 5000;

constexpr int kAlrProbeInterval = 5000;
constexpr int kAlrEndedTimeoutMs = 3000;
constexpr int kBitrateDropTimeoutMs = 5000;
}  // namespace

class ProbeControllerTest : public ::testing::Test {
 protected:
  ProbeControllerTest() : clock_(100000000L) {
    probe_controller_.reset(new ProbeController());
  }
  ~ProbeControllerTest() override {}

  void SetNetworkAvailable(bool available) {
    NetworkAvailability msg;
    msg.at_time = Timestamp::ms(NowMs());
    msg.network_available = available;
    probe_controller_->OnNetworkAvailability(msg);
  }

  int64_t NowMs() { return clock_.TimeInMilliseconds(); }

  SimulatedClock clock_;
  std::unique_ptr<ProbeController> probe_controller_;
};

TEST_F(ProbeControllerTest, InitiatesProbingAtStart) {
  probe_controller_->SetBitrates(kMinBitrateBps, kStartBitrateBps,
                                 kMaxBitrateBps, NowMs());
  EXPECT_GE(probe_controller_->GetAndResetPendingProbes().size(), 2u);
}

TEST_F(ProbeControllerTest, ProbeOnlyWhenNetworkIsUp) {
  SetNetworkAvailable(false);
  probe_controller_->SetBitrates(kMinBitrateBps, kStartBitrateBps,
                                 kMaxBitrateBps, NowMs());
  EXPECT_EQ(probe_controller_->GetAndResetPendingProbes().size(), 0u);
  SetNetworkAvailable(true);
  EXPECT_GE(probe_controller_->GetAndResetPendingProbes().size(), 2u);
}

TEST_F(ProbeControllerTest, InitiatesProbingOnMaxBitrateIncrease) {
  probe_controller_->SetBitrates(kMinBitrateBps, kStartBitrateBps,
                                 kMaxBitrateBps, NowMs());
  // Long enough to time out exponential probing.
  clock_.AdvanceTimeMilliseconds(kExponentialProbingTimeoutMs);
  probe_controller_->SetEstimatedBitrate(kStartBitrateBps, NowMs());
  probe_controller_->Process(NowMs());
  EXPECT_GE(probe_controller_->GetAndResetPendingProbes().size(), 2u);

  probe_controller_->SetBitrates(kMinBitrateBps, kStartBitrateBps,
                                 kMaxBitrateBps + 100, NowMs());

  EXPECT_EQ(
      probe_controller_->GetAndResetPendingProbes()[0].target_data_rate.bps(),
      kMaxBitrateBps + 100);
}

TEST_F(ProbeControllerTest, InitiatesProbingOnMaxBitrateIncreaseAtMaxBitrate) {
  probe_controller_->SetBitrates(kMinBitrateBps, kStartBitrateBps,
                                 kMaxBitrateBps, NowMs());
  // Long enough to time out exponential probing.
  clock_.AdvanceTimeMilliseconds(kExponentialProbingTimeoutMs);
  probe_controller_->SetEstimatedBitrate(kStartBitrateBps, NowMs());
  probe_controller_->Process(NowMs());
  EXPECT_GE(probe_controller_->GetAndResetPendingProbes().size(), 2u);

  probe_controller_->SetEstimatedBitrate(kMaxBitrateBps, NowMs());
  probe_controller_->SetBitrates(kMinBitrateBps, kStartBitrateBps,
                                 kMaxBitrateBps + 100, NowMs());
  EXPECT_EQ(
      probe_controller_->GetAndResetPendingProbes()[0].target_data_rate.bps(),
      kMaxBitrateBps + 100);
}

TEST_F(ProbeControllerTest, TestExponentialProbing) {
  probe_controller_->SetBitrates(kMinBitrateBps, kStartBitrateBps,
                                 kMaxBitrateBps, NowMs());
  probe_controller_->GetAndResetPendingProbes();

  // Repeated probe should only be sent when estimated bitrate climbs above
  // 0.7 * 6 * kStartBitrateBps = 1260.
  probe_controller_->SetEstimatedBitrate(1000, NowMs());
  EXPECT_EQ(probe_controller_->GetAndResetPendingProbes().size(), 0u);

  probe_controller_->SetEstimatedBitrate(1800, NowMs());
  EXPECT_EQ(
      probe_controller_->GetAndResetPendingProbes()[0].target_data_rate.bps(),
      2 * 1800);
}

TEST_F(ProbeControllerTest, TestExponentialProbingTimeout) {
  probe_controller_->SetBitrates(kMinBitrateBps, kStartBitrateBps,
                                 kMaxBitrateBps, NowMs());
  probe_controller_->GetAndResetPendingProbes();
  // Advance far enough to cause a time out in waiting for probing result.
  clock_.AdvanceTimeMilliseconds(kExponentialProbingTimeoutMs);
  probe_controller_->Process(NowMs());

  probe_controller_->SetEstimatedBitrate(1800, NowMs());
  EXPECT_EQ(probe_controller_->GetAndResetPendingProbes().size(), 0u);
}

TEST_F(ProbeControllerTest, RequestProbeInAlr) {
  probe_controller_->SetBitrates(kMinBitrateBps, kStartBitrateBps,
                                 kMaxBitrateBps, NowMs());
  probe_controller_->SetEstimatedBitrate(500, NowMs());
  EXPECT_GE(probe_controller_->GetAndResetPendingProbes().size(), 2u);

  probe_controller_->SetAlrStartTimeMs(clock_.TimeInMilliseconds());
  clock_.AdvanceTimeMilliseconds(kAlrProbeInterval + 1);
  probe_controller_->Process(NowMs());
  probe_controller_->SetEstimatedBitrate(250, NowMs());
  probe_controller_->RequestProbe(NowMs());

  std::vector<ProbeClusterConfig> probes =
      probe_controller_->GetAndResetPendingProbes();
  EXPECT_EQ(probes.size(), 1u);
  EXPECT_EQ(probes[0].target_data_rate.bps(), 0.85 * 500);
}

TEST_F(ProbeControllerTest, RequestProbeWhenAlrEndedRecently) {
  probe_controller_->SetBitrates(kMinBitrateBps, kStartBitrateBps,
                                 kMaxBitrateBps, NowMs());
  probe_controller_->SetEstimatedBitrate(500, NowMs());
  EXPECT_EQ(probe_controller_->GetAndResetPendingProbes().size(), 2u);

  probe_controller_->SetAlrStartTimeMs(rtc::nullopt);
  clock_.AdvanceTimeMilliseconds(kAlrProbeInterval + 1);
  probe_controller_->Process(NowMs());
  probe_controller_->SetEstimatedBitrate(250, NowMs());
  probe_controller_->SetAlrEndedTimeMs(clock_.TimeInMilliseconds());
  clock_.AdvanceTimeMilliseconds(kAlrEndedTimeoutMs - 1);
  probe_controller_->RequestProbe(NowMs());

  std::vector<ProbeClusterConfig> probes =
      probe_controller_->GetAndResetPendingProbes();
  EXPECT_EQ(probes.size(), 1u);
  EXPECT_EQ(probes[0].target_data_rate.bps(), 0.85 * 500);
}

TEST_F(ProbeControllerTest, RequestProbeWhenAlrNotEndedRecently) {
  probe_controller_->SetBitrates(kMinBitrateBps, kStartBitrateBps,
                                 kMaxBitrateBps, NowMs());
  probe_controller_->SetEstimatedBitrate(500, NowMs());
  EXPECT_EQ(probe_controller_->GetAndResetPendingProbes().size(), 2u);

  probe_controller_->SetAlrStartTimeMs(rtc::nullopt);
  clock_.AdvanceTimeMilliseconds(kAlrProbeInterval + 1);
  probe_controller_->Process(NowMs());
  probe_controller_->SetEstimatedBitrate(250, NowMs());
  probe_controller_->SetAlrEndedTimeMs(clock_.TimeInMilliseconds());
  clock_.AdvanceTimeMilliseconds(kAlrEndedTimeoutMs + 1);
  probe_controller_->RequestProbe(NowMs());
  EXPECT_EQ(probe_controller_->GetAndResetPendingProbes().size(), 0u);
}

TEST_F(ProbeControllerTest, RequestProbeWhenBweDropNotRecent) {
  probe_controller_->SetBitrates(kMinBitrateBps, kStartBitrateBps,
                                 kMaxBitrateBps, NowMs());
  probe_controller_->SetEstimatedBitrate(500, NowMs());
  EXPECT_EQ(probe_controller_->GetAndResetPendingProbes().size(), 2u);

  probe_controller_->SetAlrStartTimeMs(clock_.TimeInMilliseconds());
  clock_.AdvanceTimeMilliseconds(kAlrProbeInterval + 1);
  probe_controller_->Process(NowMs());
  probe_controller_->SetEstimatedBitrate(250, NowMs());
  clock_.AdvanceTimeMilliseconds(kBitrateDropTimeoutMs + 1);
  probe_controller_->RequestProbe(NowMs());
  EXPECT_EQ(probe_controller_->GetAndResetPendingProbes().size(), 0u);
}

TEST_F(ProbeControllerTest, PeriodicProbing) {
  probe_controller_->EnablePeriodicAlrProbing(true);
  probe_controller_->SetBitrates(kMinBitrateBps, kStartBitrateBps,
                                 kMaxBitrateBps, NowMs());
  probe_controller_->SetEstimatedBitrate(500, NowMs());
  EXPECT_EQ(probe_controller_->GetAndResetPendingProbes().size(), 2u);

  int64_t start_time = clock_.TimeInMilliseconds();

  // Expect the controller to send a new probe after 5s has passed.
  probe_controller_->SetAlrStartTimeMs(start_time);
  clock_.AdvanceTimeMilliseconds(5000);
  probe_controller_->Process(NowMs());
  probe_controller_->SetEstimatedBitrate(500, NowMs());

  std::vector<ProbeClusterConfig> probes =
      probe_controller_->GetAndResetPendingProbes();
  EXPECT_EQ(probes.size(), 1u);
  EXPECT_EQ(probes[0].target_data_rate.bps(), 1000);

  // The following probe should be sent at 10s into ALR.
  probe_controller_->SetAlrStartTimeMs(start_time);
  clock_.AdvanceTimeMilliseconds(4000);
  probe_controller_->Process(NowMs());
  probe_controller_->SetEstimatedBitrate(500, NowMs());
  EXPECT_EQ(probe_controller_->GetAndResetPendingProbes().size(), 0u);

  probe_controller_->SetAlrStartTimeMs(start_time);
  clock_.AdvanceTimeMilliseconds(1000);
  probe_controller_->Process(NowMs());
  probe_controller_->SetEstimatedBitrate(500, NowMs());
  EXPECT_EQ(probe_controller_->GetAndResetPendingProbes().size(), 1u);
}

TEST_F(ProbeControllerTest, PeriodicProbingAfterReset) {
  probe_controller_.reset(new ProbeController());
  int64_t alr_start_time = clock_.TimeInMilliseconds();

  probe_controller_->SetAlrStartTimeMs(alr_start_time);
  probe_controller_->EnablePeriodicAlrProbing(true);
  probe_controller_->SetBitrates(kMinBitrateBps, kStartBitrateBps,
                                 kMaxBitrateBps, NowMs());
  probe_controller_->Reset(NowMs());

  clock_.AdvanceTimeMilliseconds(10000);
  probe_controller_->Process(NowMs());
  EXPECT_EQ(probe_controller_->GetAndResetPendingProbes().size(), 2u);

  probe_controller_->SetBitrates(kMinBitrateBps, kStartBitrateBps,
                                 kMaxBitrateBps, NowMs());
  EXPECT_EQ(probe_controller_->GetAndResetPendingProbes().size(), 2u);

  // Make sure we use |kStartBitrateBps| as the estimated bitrate
  // until SetEstimatedBitrate is called with an updated estimate.
  clock_.AdvanceTimeMilliseconds(10000);
  probe_controller_->Process(NowMs());
  EXPECT_EQ(
      probe_controller_->GetAndResetPendingProbes()[0].target_data_rate.bps(),
      kStartBitrateBps * 2);
}

TEST_F(ProbeControllerTest, TestExponentialProbingOverflow) {
  const int64_t kMbpsMultiplier = 1000000;
  probe_controller_->SetBitrates(kMinBitrateBps, 10 * kMbpsMultiplier,
                                 100 * kMbpsMultiplier, NowMs());

  probe_controller_->SetEstimatedBitrate(60 * kMbpsMultiplier, NowMs());

  // Verify that probe bitrate is capped at the specified max bitrate.
  EXPECT_EQ(
      probe_controller_->GetAndResetPendingProbes()[2].target_data_rate.bps(),
      100 * kMbpsMultiplier);

  // Verify that repeated probes aren't sent.
  probe_controller_->SetEstimatedBitrate(100 * kMbpsMultiplier, NowMs());
  EXPECT_EQ(probe_controller_->GetAndResetPendingProbes().size(), 0u);
}

}  // namespace test
}  // namespace webrtc_cc
}  // namespace webrtc
