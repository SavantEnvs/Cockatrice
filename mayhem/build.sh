#!/usr/bin/env bash
# mayhem/build.sh — builds the Cockatrice fuzz harnesses and the upstream test suite.
#  1) build-fuzz/: upstream CMake build (servatrice + shared libcockatrice_* libs, no GUI client/oracle;
#     the harnesses compile the oracle/client sources they need, see fuzzers.cmake)
#     with $SANITIZER_FLAGS $DEBUG_FLAGS + SanCov; mayhem/fuzzers.cmake is injected through
#     CMAKE_PROJECT_Cockatrice_INCLUDE and adds the harnesses (libFuzzer + -standalone) into
#     build-fuzz/fuzz/.
#  2) build-tests/: the same tree with TEST=ON and upstream's normal Release flags; test.sh runs ctest.
# Both dirs match upstream's `build*` .gitignore entry. Re-runnable and offline (all deps are apt).
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${STANDALONE_FUZZ_MAIN:?STANDALONE_FUZZ_MAIN must be set (the base image provides it)}"
export CC CXX
cd "$SRC"

COMMON=(-G Ninja -DCMAKE_BUILD_TYPE=Release -DWITH_SERVER=1 -DWITH_CLIENT=0 -DWITH_ORACLE=0
        -DWARNING_AS_ERROR=OFF -DUSE_CCACHE=OFF -DUPDATE_TRANSLATIONS=OFF)

# 1) sanitized + coverage-instrumented project and harnesses
FUZZ_CFLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link"
cmake -S . -B build-fuzz "${COMMON[@]}" -DTEST=OFF \
  -DCMAKE_C_FLAGS="$FUZZ_CFLAGS" -DCMAKE_CXX_FLAGS="$FUZZ_CFLAGS" \
  -DCMAKE_EXE_LINKER_FLAGS="$SANITIZER_FLAGS" \
  -DCMAKE_PROJECT_Cockatrice_INCLUDE="$SRC/mayhem/project_hook.cmake" \
  -DFUZZ_ENGINE="$LIB_FUZZING_ENGINE" \
  -DFUZZ_STANDALONE_MAIN="$STANDALONE_FUZZ_MAIN" \
  -DFUZZ_LSAN_OFF="$SRC/mayhem/lsan_off.cc"
cmake --build build-fuzz -j"$MAYHEM_JOBS"

# 2) upstream test suite, normal flags
cmake -S . -B build-tests "${COMMON[@]}" -DTEST=ON \
  -DCMAKE_C_FLAGS="$COVERAGE_FLAGS" -DCMAKE_CXX_FLAGS="$COVERAGE_FLAGS" \
  -DCMAKE_EXE_LINKER_FLAGS="$COVERAGE_FLAGS"
# Upstream's test tree does not fully build in the server-only configuration (these targets need
# libcockatrice_settings/models, which are added only WITH_CLIENT/WITH_ORACLE, or miss the global
# `rng`): build every other test executable under tests/. test.sh excludes exactly these from ctest.
BROKEN_TESTS="server_dev_smoke_test collator_sort_test deck_list_model_custom_zones_test
  deck_list_model_zone_integration_test model_contract_test servers_settings_test settings_defaults_test
  settings_manager_test settings_migration_test servatrice_settings_test load_benchmark"
mapfile -t TEST_TARGETS < <(ninja -C build-tests -t targets all \
  | sed -nE 's#^tests/([^:]*/)?([A-Za-z0-9_]+): CXX_EXECUTABLE_LINKER.*#\2#p' | grep -vxF -f <(tr -s ' \n' '\n' <<<"$BROKEN_TESTS"))
cmake --build build-tests -j"$MAYHEM_JOBS" --target "${TEST_TARGETS[@]}"
