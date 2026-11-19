#!/usr/bin/env bash
#
# mayhem/test.sh — RUN this repo's OWN functional test suite (already built by mayhem/build.sh).
# exit 0 = pass. EDIT per repo. PATCH-grade oracle: after an agent patches the source, the grader
# rebuilds (build.sh) then runs this. DELETE this file if the repo has no meaningful tests.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

# Runs upstream's googletest suite (built by build.sh into build-tests/, discovered by ctest), minus
# the benchmark label and the targets that do not build in the server-only configuration (build.sh
# BROKEN_TESTS). A ctest "Passed" only counts when that test's gtest output also shows
# "[       OK ]" — the test really ran its assertions — so a no-op binary that exits 0 fails.
export QT_QPA_PLATFORM=offscreen
BUILD="$SRC/build-tests"
[ -f "$BUILD/CTestTestfile.cmake" ] || { echo "build-tests/ missing: run mayhem/build.sh first" >&2; emit_ctrf cmake-ctest 0 1 0; exit 1; }
EXCLUDE='^(server_dev_smoke_test|collator_sort_test|deck_list_model_custom_zones_test|deck_list_model_zone_integration_test|model_contract_test|servers_settings_test|settings_defaults_test|settings_manager_test|settings_migration_test|servatrice_settings_test)_NOT_BUILT$'
log=$(mktemp)
(cd "$BUILD" && ctest -V -j"$MAYHEM_JOBS" -LE bench -E "$EXCLUDE" --timeout 120) > "$log" 2>&1
tail -5 "$log"
# "Test #N: name ....   Passed" lines, and "N: [       OK ] Suite.Case" lines from the test's own output.
total=$(grep -cE '^ *[0-9]+/[0-9]+ Test +#[0-9]+: ' "$log" || true)
ctest_passed_ids=$(grep -E '^ *[0-9]+/[0-9]+ Test +#[0-9]+: .* Passed ' "$log" | sed -E 's/.*Test +#([0-9]+):.*/\1/' | sort -u)
ok_ids=$(grep -E '^[0-9]+: \[ +OK \] ' "$log" | sed -E 's/^([0-9]+):.*/\1/' | sort -u)
skipped=$(grep -E '^ *[0-9]+/[0-9]+ Test +#[0-9]+: ' "$log" | grep -v ' Passed ' | grep -c 'Skipped' || true)
passed=$(comm -12 <(echo "$ctest_passed_ids") <(echo "$ok_ids") | grep -c . || true)
failed=$(( total - passed - skipped ))
[ "$total" -gt 0 ] || failed=1
rm -f "$log"
emit_ctrf cmake-ctest "$passed" "$failed" "$skipped"
