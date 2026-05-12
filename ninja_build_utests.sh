#!/usr/bin/env bash

MUSE_APP_BUILD_MODE=dev \
MUSE_BUILD_UNIT_TESTS=ON \
MUSE_COMPILE_USE_UNITY=ON \
bash ./ninja_build.sh -t installdebug