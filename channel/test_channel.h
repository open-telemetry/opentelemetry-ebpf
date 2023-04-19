/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <channel/channel.h>
#include <channel/network_channel.h>
#include <common/intake_encoder.h>
#include <scheduling/timer.h>
#include <util/json.h>
#include <util/log.h>

#include <map>
#include <sstream>

#include <uv.h>

namespace channel {

/**
 * A channel intended for use by unit tests.  It implements all Channel methods.  If loop is provided to the constructor then
 * it also implements the NetworkChannel methods.
 */
class TestChannel : public NetworkChannel {
public:
  TestChannel(
      std::optional<std::reference_wrapper<uv_loop_t>> loop = std::nullopt, IntakeEncoder encoder = IntakeEncoder::binary);

  std::error_code send(const u8 *data, int size) override;

  void close() override;
  std::error_code flush() override;

  bool is_open() const override { return true; }

  void connect(Callbacks &callbacks) override;
  in_addr_t const *connected_address() const override;

  u64 get_num_sends();
  u64 get_num_failed_sends();
  std::stringstream &get_ss();

  using MessageCountsType = std::map<std::string, u64>; // map of message name to number sent
  MessageCountsType &get_message_counts();

  using BinaryMessageType = std::vector<u8>;
  using BinaryMessagesType = std::vector<BinaryMessageType>; // vector of messages sent in binary format
  BinaryMessagesType &get_binary_messages();
  void binary_messages_for_each(std::function<void(BinaryMessageType const &)> const &cb);

  using JsonMessageType = nlohmann::json;
  using JsonMessagesType = std::vector<JsonMessageType>; // vector of messages sent in JSON format
  JsonMessagesType &get_json_messages();
  void json_messages_for_each(std::function<void(JsonMessageType const &)> const &cb);

  // Used to specify a function for the TestChannel to call for every render message processed by send().
  void set_sent_msg_cb(std::function<void(nlohmann::json const &)> const &sent_msg_cb_);

private:
  std::optional<std::reference_wrapper<uv_loop_t>> loop_;
  std::unique_ptr<scheduling::Timer> fake_connected_cb_timer_;

  u64 num_sends_ = 0;
  u64 num_failed_sends_ = 0;

  IntakeEncoder encoder_;
  std::stringstream ss_;

  MessageCountsType message_counts_;

  BinaryMessagesType binary_messages_;
  JsonMessagesType json_messages_;

  std::function<void(nlohmann::json const &)> sent_msg_cb_;
};

} // namespace channel
