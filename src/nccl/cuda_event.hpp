/*
 * ghex-org
 *
 * Copyright (c) 2014-2025, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <cuda_runtime.h>

#include <oomph/util/moved_bit.hpp>

#include "cuda_error.hpp"

namespace oomph::detail {
struct cuda_event {
  cudaEvent_t m_event;
  oomph::util::moved_bit m_moved;
  bool m_recorded{false};

  cuda_event() {
    OOMPH_CHECK_CUDA_RESULT(cudaEventCreateWithFlags(&m_event, cudaEventDisableTiming));
    // std::cerr << "created a cuda_event with value " << m_event << "\n";
  }
  cuda_event(cuda_event&& other) noexcept = default;
  cuda_event& operator=(cuda_event&& other) noexcept = default;
  cuda_event(const cuda_event&) = delete;
  cuda_event& operator=(const cuda_event&) = delete;
  ~cuda_event() noexcept {
      if (!m_moved) {
          OOMPH_CHECK_CUDA_RESULT_NO_THROW(cudaEventDestroy(m_event));
      }
  }

  void record(cudaStream_t stream) {
    assert(!m_moved);
    OOMPH_CHECK_CUDA_RESULT(cudaEventRecord(m_event, stream));
    m_recorded = true;
  }

  bool is_ready() {
    // std::cerr << "checking if request is ready\n";
    if (m_moved || !m_recorded) {
      return false;
    }

    cudaError_t res = cudaEventQuery(m_event);
    // std::cerr << "request " << m_event << " is in state " << res << "\n";
    if (res == cudaSuccess) {
    	return true;
    } else if (res == cudaErrorNotReady) {
    	return false;
    } else {
    	OOMPH_CHECK_CUDA_RESULT(res);
            return false;
    }
  }

  cudaEvent_t get() {
    assert(!m_moved);
    return m_event;
  }
};

struct group_cuda_event {
  std::shared_ptr<cuda_event> m_event;

  group_cuda_event() : m_event(std::make_shared<cuda_event>()) {}
  group_cuda_event(const group_cuda_event&) = default;
  group_cuda_event& operator=(const group_cuda_event&) = default;
  group_cuda_event(group_cuda_event&&) = default;
  group_cuda_event& operator=(group_cuda_event&&) = default;

  void record(cudaStream_t stream) {
    m_event->record(stream);
  }

  bool is_ready() {
    return m_event->is_ready();
  }

  cudaEvent_t get() {
    return m_event->get();
  }
};
}
