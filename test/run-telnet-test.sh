#!/usr/bin/env bash
#
# run-telnet-test.sh - Orchestrate an EwokOS telnet stress test in QEMU.
#
# Boots EwokOS headless in QEMU, waits for telnetd readiness on
# 127.0.0.1:2323, runs the telnet-stress.exp expect harness, then ALWAYS
# tears down the QEMU instance it started.
#
# Usage:
#   run-telnet-test.sh [--build] [iterations] [per_timeout]
#
# Options:
#   --build        Build the system + kernel before booting.
#
# Positional (optional):
#   iterations     Number of stress iterations (default 10).
#   per_timeout    Per-iteration timeout in seconds (default 30).
#
# Notes:
#   - Never uses rm -rf or destructive commands.
#   - Only kills the specific QEMU PID it started.
#

set -u

# ---- configuration ----
EWOK_ROOT="/Users/misa/work/ewokos"
KERNEL_DIR="${EWOK_ROOT}/machine.virt/kernel"
SYSTEM_DIR="${EWOK_ROOT}/machine.virt/system"
TEST_DIR="${EWOK_ROOT}/test"
EXP_SCRIPT="${TEST_DIR}/telnet-stress.exp"
QEMU_LOG="/tmp/ewokos-qemu.log"

HOST="127.0.0.1"
PORT="2323"

KERNEL_IMG="${KERNEL_DIR}/kernel8.img"
ROOTFS="${EWOK_ROOT}/system/build/virt/root_aarch64.ext2"

# boot / readiness tuning
BOOT_GRACE=8       # seconds to wait before first probe
MAX_WAIT=150       # max seconds to wait for telnet login readiness
PROBE_INTERVAL=3   # seconds between probes
TELNET_PROBE_TO=5  # seconds to let the telnet handshake produce a banner

# ---- argument parsing ----
DO_BUILD=0
CYCLES=10
ITERATIONS=10
PER_TIMEOUT=30

POS=()
for arg in "$@"; do
    case "$arg" in
        --build)
            DO_BUILD=1
            ;;
        --help|-h)
            echo "Usage: $0 [--build] [cycles] [iterations] [per_timeout]"
            exit 0
            ;;
        *)
            POS+=("$arg")
            ;;
    esac
done

if [ "${#POS[@]}" -ge 1 ]; then CYCLES="${POS[0]}"; fi
if [ "${#POS[@]}" -ge 2 ]; then ITERATIONS="${POS[1]}"; fi
if [ "${#POS[@]}" -ge 3 ]; then PER_TIMEOUT="${POS[2]}"; fi

# short pause between cycles so the guest can reap the closed connection before
# reconnecting; kept short so a teardown-related hang still surfaces as a
# timeout rather than being masked.
CYCLE_PAUSE=1

# ---- teardown handling ----
QEMU_PID=""

teardown() {
    if [ -n "${QEMU_PID}" ] && kill -0 "${QEMU_PID}" 2>/dev/null; then
        echo ">>> Tearing down QEMU (pid ${QEMU_PID})"
        # kill children first, then the process itself
        pkill -TERM -P "${QEMU_PID}" 2>/dev/null || true
        kill -TERM "${QEMU_PID}" 2>/dev/null || true
        # give it a moment to exit gracefully
        for _ in 1 2 3 4 5; do
            if kill -0 "${QEMU_PID}" 2>/dev/null; then
                sleep 1
            else
                break
            fi
        done
        if kill -0 "${QEMU_PID}" 2>/dev/null; then
            echo ">>> QEMU still alive, sending KILL"
            pkill -KILL -P "${QEMU_PID}" 2>/dev/null || true
            kill -KILL "${QEMU_PID}" 2>/dev/null || true
        fi
    fi
}
trap teardown EXIT INT TERM

# ---- optional build ----
if [ "${DO_BUILD}" -eq 1 ]; then
    echo ">>> Building EwokOS ..."
    ( cd "${SYSTEM_DIR}" && make x && make sd && cd ../kernel && make ) || {
        echo "FAIL: build failed"
        exit 3
    }
fi

# ---- sanity checks ----
if [ ! -f "${KERNEL_IMG}" ]; then
    echo "FAIL: kernel image not found: ${KERNEL_IMG} (use --build)"
    exit 3
fi
if [ ! -f "${ROOTFS}" ]; then
    echo "FAIL: rootfs not found: ${ROOTFS} (use --build)"
    exit 3
fi
if [ ! -f "${EXP_SCRIPT}" ]; then
    echo "FAIL: expect script not found: ${EXP_SCRIPT}"
    exit 3
fi

# ---- launch QEMU headless in background ----
# NOTE: the guest boot init starts services SEQUENTIALLY (framebuffer/ramfb,
# keyboard, mouse, sound) BEFORE telnetd. If those virtio devices are absent,
# the corresponding init step fails ("Virtio-input init failed") and the
# sequential boot never reaches telnetd. So we keep the FULL device set from
# the known-working `make run`, but stay headless via `-display none`.
echo ">>> Launching QEMU headless (log: ${QEMU_LOG}) ..."
: > "${QEMU_LOG}"

(
    cd "${KERNEL_DIR}" && \
    exec qemu-system-aarch64 -M virt,highmem=off,accel=hvf -cpu host -m 512 \
        -display none -serial mon:stdio -device ramfb \
        -smp 4 \
        -kernel kernel8.img \
        -drive file="${ROOTFS}",format=raw,id=blk0,if=none \
        -device virtio-blk-device,drive=blk0 \
        -netdev user,id=net0,hostfwd=tcp:127.0.0.1:2323-:23 \
        -device virtio-net-device,netdev=net0 \
        -device virtio-tablet-device -device virtio-keyboard-device \
        -audiodev coreaudio,id=audio0 -device virtio-sound-device,audiodev=audio0
) >"${QEMU_LOG}" 2>&1 < /dev/null &
QEMU_PID=$!

echo ">>> QEMU pid=${QEMU_PID}"

# ---- readiness probe: has the guest telnetd produced a login banner? ----
# The QEMU user-mode hostfwd opens the host port IMMEDIATELY at launch (before
# telnetd is actually listening), so a plain `nc -z` port check is a FALSE
# POSITIVE. Instead we require the guest to actually emit a "login:" banner,
# either on the serial console (primary) or over a short telnet handshake
# (secondary).
telnet_banner_seen() {
    # open a short telnet session and look for the login banner
    printf '' | ( telnet "${HOST}" "${PORT}" 2>/dev/null & tpid=$!; \
                  sleep "${TELNET_PROBE_TO}"; kill "${tpid}" 2>/dev/null ) \
        | grep -q "login:"
}

# ---- wait for telnet readiness ----
echo ">>> Waiting boot grace period (${BOOT_GRACE}s) ..."
sleep "${BOOT_GRACE}"

echo ">>> Waiting for guest telnetd login readiness on ${HOST}:${PORT} (max ${MAX_WAIT}s) ..."
ready=0
deadline=$((SECONDS + MAX_WAIT))
while [ "${SECONDS}" -lt "${deadline}" ]; do
    # bail out early if QEMU died
    if ! kill -0 "${QEMU_PID}" 2>/dev/null; then
        echo "FAIL: QEMU exited prematurely (see ${QEMU_LOG})"
        exit 4
    fi

    # primary signal: guest prints the login banner on the serial console
    if grep -q "login:" "${QEMU_LOG}" 2>/dev/null; then
        echo ">>> login banner detected in serial log (after $((MAX_WAIT - (deadline - SECONDS)))s)"
        ready=1
        break
    fi

    # secondary signal: telnet handshake yields a login banner
    if telnet_banner_seen; then
        echo ">>> login banner detected via telnet handshake (after $((MAX_WAIT - (deadline - SECONDS)))s)"
        ready=1
        break
    fi

    sleep "${PROBE_INTERVAL}"
done

if [ "${ready}" -ne 1 ]; then
    echo "FAIL: guest telnetd did not become ready within ${MAX_WAIT}s"
    echo "----- last lines of ${QEMU_LOG} -----"
    tail -n 40 "${QEMU_LOG}" 2>/dev/null || true
    exit 4
fi

# small settle delay so telnetd fully accepts logins
sleep 2

# ---- run the expect stress harness across multiple reconnect cycles ----
# Each cycle opens a FRESH telnet connection, logs in, runs the inner
# cat/dump loop N times, then exits cleanly. This stresses connection
# teardown + re-accept in the guest.
echo ">>> Running ${CYCLES} reconnect cycles, each ${ITERATIONS} iterations (${PER_TIMEOUT}s per-iteration timeout)"

OVERALL_RC=0
FAIL_CYCLE=""
FAIL_DETAIL=""

for cycle in $(seq 1 "${CYCLES}"); do
    echo ">>> --- CYCLE ${cycle}/${CYCLES}: connecting ---"

    # bail early if QEMU died between cycles
    if ! kill -0 "${QEMU_PID}" 2>/dev/null; then
        echo "CYCLE ${cycle}/${CYCLES}: FAIL (QEMU exited)"
        OVERALL_RC=4
        FAIL_CYCLE="${cycle}"
        FAIL_DETAIL="QEMU exited"
        break
    fi

    # capture output so we can surface which inner iteration hung
    CYCLE_OUT="$(expect "${EXP_SCRIPT}" "${HOST}" "${PORT}" "${ITERATIONS}" "${PER_TIMEOUT}" 2>&1)"
    EXP_RC=$?
    echo "${CYCLE_OUT}"

    if [ "${EXP_RC}" -eq 0 ]; then
        echo "CYCLE ${cycle}/${CYCLES}: PASS (${ITERATIONS}/${ITERATIONS} iterations)"
    else
        # scrape which iteration hung (expect prints "ITERATION i/N ... HUNG")
        HUNG_LINE="$(printf '%s\n' "${CYCLE_OUT}" | grep -E 'HUNG|FAIL:' | head -n1)"
        HUNG_ITER="$(printf '%s\n' "${CYCLE_OUT}" | grep -oE 'iteration [0-9]+' | head -n1 | grep -oE '[0-9]+')"
        echo "CYCLE ${cycle}/${CYCLES}: FAIL (expect exit code ${EXP_RC})"
        [ -n "${HUNG_LINE}" ] && echo "    detail: ${HUNG_LINE}"
        OVERALL_RC="${EXP_RC}"
        FAIL_CYCLE="${cycle}"
        if [ -n "${HUNG_ITER}" ]; then
            FAIL_DETAIL="iteration ${HUNG_ITER} hung"
        elif [ "${EXP_RC}" -eq 2 ]; then
            FAIL_DETAIL="connect/login failure"
        else
            FAIL_DETAIL="expect exit ${EXP_RC}"
        fi
        echo "----- last lines of ${QEMU_LOG} -----"
        tail -n 40 "${QEMU_LOG}" 2>/dev/null || true
        break
    fi

    # short settle pause before reconnecting for the next cycle
    if [ "${cycle}" -lt "${CYCLES}" ]; then
        sleep "${CYCLE_PAUSE}"
    fi
done

# ---- final summary ----
echo "======================================"
if [ "${OVERALL_RC}" -eq 0 ]; then
    echo "RESULT: PASS (${CYCLES}/${CYCLES} cycles, each ${ITERATIONS}/${ITERATIONS} iterations)"
else
    echo "RESULT: FAIL at cycle ${FAIL_CYCLE} (${FAIL_DETAIL})"
fi
echo "======================================"

# teardown runs via trap on EXIT
exit "${OVERALL_RC}"
