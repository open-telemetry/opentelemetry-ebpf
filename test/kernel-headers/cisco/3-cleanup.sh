#!/bin/bash
# Copyright The OpenTelemetry Authors
# SPDX-License-Identifier: Apache-2.0

set -xe
vagrant destroy -f || true
[ -e .vagrant ] && rm -rf .vagrant
