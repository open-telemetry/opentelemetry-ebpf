/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel_symbols.h"

#include <linux/bpf.h>

#include <bcc/BPFTable.h>
#include <bcc/bpf_module.h>

#include <collector/kernel/perf_reader.h>
#include <util/logger.h>

#include <optional>
#include <string>
#include <vector>

/**
 * ProbeAlternatives encapsulates multiple alternatives to attempt when attaching a probe.  Alternatives may be needed due to
 * differences in kernel versions or builds.
 */
struct ProbeAlternatives {
  struct FuncAndKfunc {
    std::string func_name;
    std::string k_func_name;
  };

  ProbeAlternatives(std::string desc, std::vector<FuncAndKfunc> func_names)
      : desc(std::move(desc)), func_names(std::move(func_names))
  {}

  std::string desc;
  std::vector<FuncAndKfunc> func_names;
};

/**
 * Handles the creation of probes and provides info for cleanup to signal
 * handler
 */
class ProbeHandler {
  friend class KernelCollectorTest;

public:
  /**
   * c'tor
   */
  ProbeHandler(logging::Logger &log);

  /**
   * Loads the list of available kernel symbols from /proc/kallsyms.
   * This list is then used to determine if a kernel function can be instrumented.
   */
  void load_kernel_symbols();

  /**
   * Clears the list of kernel symbols.
   * Used to free up memory after all probes are started.
   */
  void clear_kernel_symbols();

  int start_bpf_module(std::string full_program, ebpf::BPFModule &bpf_module, PerfContainer &perf);

  /**
   * BPF table helpers
   **/
  ebpf::BPFHashTable<u32, u32> get_hash_table(ebpf::BPFModule &bpf_module, const std::string &name);
  ebpf::BPFProgTable get_prog_table(ebpf::BPFModule &bpf_module, const std::string &name);
  ebpf::BPFStackTable get_stack_table(ebpf::BPFModule &bpf_module, const std::string &name);

  /**
   * Register tail call in table
   */
  int register_tail_call(
      ebpf::BPFModule &bpf_module, const std::string &prog_array_name, int index, const std::string &func_name);

  /**
   * Starts a kprobe
   * on failure logs error and increments num_failed_probes_
   * @returns 0 on success, negative value on failure
   */
  int start_probe(
      ebpf::BPFModule &bpf_module,
      const std::string &func_name,
      const std::string &k_func_name,
      const std::string &event_id_suffix = std::string());

  /**
   * Starts a kretprobe
   * on failure logs error and increments num_failed_probes_
   * @returns 0 on success, negative value on failure
   */
  int start_kretprobe(
      ebpf::BPFModule &bpf_module,
      const std::string &func_name,
      const std::string &k_func_name,
      const std::string &event_id_suffix = std::string());

  /**
   * Starts a kprobe from the provided alternatives.
   * Probes are attempted in order until one succeeds.
   * If all alternatives fail an error is logged and num_failed_probes_ is incremented.
   * @returns string containing the k_func_name of the probe that was attached on success, empty string on failure
   */
  std::string start_probe(
      ebpf::BPFModule &bpf_module,
      const ProbeAlternatives &probe_alternatives,
      const std::string &event_id_suffix = std::string());

  /**
   * Starts a kretprobe from the provided alternatives.
   * Probes are attempted in order until one succeeds.
   * If all alternatives fail an error is logged and num_failed_probes_ is incremented.
   * @returns string containing the k_func_name of the probe that was attached on success, empty string on failure
   */
  std::string start_kretprobe(
      ebpf::BPFModule &bpf_module,
      const ProbeAlternatives &probe_alternatives,
      const std::string &event_id_suffix = std::string());

  /**
   * Handles the cleanup of probes on exit
   */
  void cleanup_probes();

  /**
   * Clean up all the registered tail calls
   */
  void cleanup_tail_calls(ebpf::BPFModule &bpf_module);

  /**
   * Cleans up a single probe
   */
  void cleanup_probe(const std::string &k_func_name);

  /**
   * Cleans up a single kretprobe
   */
  void cleanup_kretprobe(const std::string &k_func_name);

#if DEBUG_ENABLE_STACKTRACE
  /**
   * Gets a stack trace and removes it from the list
   */
  std::string get_stack_trace(ebpf::BPFModule &bpf_module, s32 kernel_stack_id, s32 user_stack_id, u32 tgid);
#endif

protected:
  /**
   * Common code to start a kprobe or kretprobe
   * @returns 0 on success, negative value on failure
   */
  int start_probe_common(
      ebpf::BPFModule &bpf_module,
      bool is_kretprobe,
      const std::string &func_name,
      const std::string &k_func_name,
      const std::string &event_id_suffix);

  /**
   * Common code to start a kprobe or kretprobe from the provided alternatives
   * @returns string containing the k_func_name of the probe that was attached on success, empty string on failure
   */
  std::string start_probe_common(
      ebpf::BPFModule &bpf_module,
      bool is_kretprobe,
      const ProbeAlternatives &probe_alternatives,
      const std::string &event_id_suffix = std::string());

  /**
   * Common code to clean up a single probe
   */
  void cleanup_probe_common(const std::string &probe_name);

  /**
   * Returns the file descriptor for a table declared in bpf
   */
  int get_bpf_table_descriptor(ebpf::BPFModule &bpf_module, const char *table_name);

  /**
   * Sets up memory mapping for perf rings
   */
  int setup_mmap(int cpu, int events_fd, PerfContainer &perf, bool is_data, u32 n_bytes, u32 n_watermark_bytes);

private:
  static constexpr char probe_prefix_[] = "ebpf_net_p_";
  static constexpr char kretprobe_prefix_[] = "ebpf_net_r_";

  logging::Logger &log_;

  struct TailCallTuple {
    TailCallTuple(const std::string &table, const std::string &func, int fd, int index)
        : table_(table), func_(func), fd_(fd), index_(index)
    {}
    std::string table_;
    std::string func_;
    int fd_;
    int index_;
  };
  std::vector<int> fds_;
  std::vector<int> probes_;
  std::vector<TailCallTuple> tail_calls_;
  std::vector<std::string> probe_names_;
  size_t num_failed_probes_; // number of kprobes, kretprobes, and tail_calls that failed to attach
  size_t stack_trace_count_;

  std::optional<KernelSymbols> kernel_symbols_;
};
