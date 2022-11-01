# Copyright The OpenTelemetry Authors
# SPDX-License-Identifier: Apache-2.0

include_guard()

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
if (NOT OPENSSL_VERSION STREQUAL "1.1.1n")
  message(FATAL_ERROR "OpenSSL must be a specific version (1.1.1n). Build container should already have that set up")
endif()
