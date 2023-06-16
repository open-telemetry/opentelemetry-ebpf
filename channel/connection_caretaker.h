/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <channel/callbacks.h>
#include <channel/channel.h>
#include <common/client_type.h>
#include <generated/ebpf_net/ingest/writer.h>
#include <util/aws_instance_metadata.h>
#include <util/curl_engine.h>
#include <util/gcp_instance_metadata.h>

#include <uv.h>

#include <chrono>
#include <functional>
#include <map>

namespace channel {

// ConnectionCaretaker handles common tasks of agent->server connection.
//
// Current implementation does followings:
//   1. Sends back initial metadata, including agent version, and configuration labels.
//   2. Sends back heartbeat signal to server periodically.
//
// This class is NOT thread-safe.
class ConnectionCaretaker {
public:
  using config_labels_t = std::map<std::string, std::string>;

  // Constructor
  //
  // |config_data|: Configuration labels, read from a yaml file.
  // |loop|: Libuv event loop.
  // |channel|: Underline channel connecting agent and server.
  // |heartbeat_interval|: How often a heartbeat signal is sent back to server.
  // |flush_cb|: Callback to flush any downstream buffer.
  ConnectionCaretaker(
      std::string_view hostname,
      ClientType client_type,
      config_labels_t const &config_labels,
      uv_loop_t *loop,
      ebpf_net::ingest::Writer &writer,
      std::chrono::milliseconds metadata_timeout,
      std::chrono::milliseconds heartbeat_interval,
      std::function<void()> flush_cb,
      std::function<void(bool)> set_compression_cb,
      std::function<void()> on_connected_cb);

  ~ConnectionCaretaker();

  // Note, this function triggers metadata to be sent back, and starts
  // heartbeat signal.
  void set_connected();

  // Note, this function stops heartbeat timer implicitly
  void set_disconnected();

  // Sends one heartbeat. It is public so that timer callback can use it.
  void send_heartbeat();

private:
  // Sends following information:
  //   agent version and any config labels.
  // TODO: Send agent type as well.
  void send_metadata_header();
  void start_heartbeat();
  void stop_heartbeat();

  void flush();

  std::string_view const hostname_;
  ClientType const client_type_;

  const config_labels_t config_labels_;

  uv_loop_t *loop_ = nullptr; // not owned

  std::optional<AwsMetadata> aws_metadata_;
  std::optional<GcpInstanceMetadata> gcp_metadata_;

  const std::chrono::milliseconds heartbeat_interval_;

  std::function<void()> flush_cb_;
  std::function<void(bool)> set_compression_cb_;
  std::function<void()> on_connected_cb_;

  uv_timer_t heartbeat_timer_;

  ebpf_net::ingest::Writer &writer_;
};

} // namespace channel
