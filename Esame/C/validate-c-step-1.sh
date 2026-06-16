#!/usr/bin/env bash

set -u

MASTER_SRC="master-1.c"
WORKER_SRC="worker-1.c"
REG_FIFO="/tmp/registration-fifo"

N="${1:-5}"
TIMEOUT_SECONDS=5

fail() {
    echo "FAIL: $1"
    if [[ -d "${TMPDIR:-}" ]]; then
        echo
        echo "--- master stdout ---"
        cat "$TMPDIR/master.out" 2>/dev/null || true
        echo
        echo "--- master stderr ---"
        cat "$TMPDIR/master.err" 2>/dev/null || true
        echo
        echo "--- worker stderr files ---"
        cat "$TMPDIR"/worker-*.err 2>/dev/null || true
    fi
    exit 1
}

pass() {
    echo "PASS: $1"
}

if ! [[ "$N" =~ ^[1-9][0-9]*$ ]]; then
    fail "N must be a positive integer"
fi

if [[ ! -f "$MASTER_SRC" ]]; then
    fail "missing $MASTER_SRC"
fi

if [[ ! -f "$WORKER_SRC" ]]; then
    fail "missing $WORKER_SRC"
fi

TMPDIR="$(mktemp -d)"
MASTER_BIN="$TMPDIR/master-1-simple"
WORKER_BIN="$TMPDIR/worker-1-simple"

cleanup() {
    rm -f "$REG_FIFO"
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

rm -f "$REG_FIFO"

echo "Compiling $MASTER_SRC..."
gcc -Wall -Wextra -o "$MASTER_BIN" "$MASTER_SRC" >"$TMPDIR/master-gcc.out" 2>"$TMPDIR/master-gcc.err"
if [[ $? -ne 0 ]]; then
    echo "--- compiler stderr ---"
    cat "$TMPDIR/master-gcc.err"
    fail "compilation of $MASTER_SRC failed"
fi

echo "Compiling $WORKER_SRC..."
gcc -Wall -Wextra -o "$WORKER_BIN" "$WORKER_SRC" >"$TMPDIR/worker-gcc.out" 2>"$TMPDIR/worker-gcc.err"
if [[ $? -ne 0 ]]; then
    echo "--- compiler stderr ---"
    cat "$TMPDIR/worker-gcc.err"
    fail "compilation of $WORKER_SRC failed"
fi

echo "Starting master for $N registration requests..."
timeout "$TIMEOUT_SECONDS" "$MASTER_BIN" "$N" >"$TMPDIR/master.out" 2>"$TMPDIR/master.err" &
MASTER_PID=$!

# Wait until the master has created the registration FIFO.
fifo_ready=0
for _ in $(seq 1 50); do
    if [[ -p "$REG_FIFO" ]]; then
        fifo_ready=1
        break
    fi
    sleep 0.1
done

if [[ "$fifo_ready" -ne 1 ]]; then
    kill "$MASTER_PID" 2>/dev/null || true
    fail "master did not create $REG_FIFO"
fi

echo "Launching $N workers..."

worker_launcher_pids=()

for i in $(seq 1 "$N"); do
    expected_pid_file="$TMPDIR/expected-worker-$i.pid"

    # This small bash wrapper writes its own PID before exec().
    # After exec(), the worker keeps the same PID, so this is the PID
    # that the worker should send to the master.
    timeout "$TIMEOUT_SECONDS" bash -c \
        'echo "$BASHPID" > "$1"; exec "$0"' \
        "$WORKER_BIN" "$expected_pid_file" \
        >"$TMPDIR/worker-$i.out" 2>"$TMPDIR/worker-$i.err" &

    worker_launcher_pids+=("$!")
done

for pid in "${worker_launcher_pids[@]}"; do
    wait "$pid"
    rc=$?

    if [[ "$rc" -ne 0 ]]; then
        fail "a worker failed or timed out, exit code $rc"
    fi
done

wait "$MASTER_PID"
master_rc=$?

if [[ "$master_rc" -eq 124 ]]; then
    fail "master timed out before receiving $N registration requests"
fi

if [[ "$master_rc" -ne 0 ]]; then
    fail "master exited with code $master_rc"
fi

received_count=$(grep -c "registration request received from PID" "$TMPDIR/master.out" || true)

if [[ "$received_count" -ne "$N" ]]; then
    fail "expected $N registration messages, found $received_count"
fi

for i in $(seq 1 "$N"); do
    expected_pid_file="$TMPDIR/expected-worker-$i.pid"

    if [[ ! -f "$expected_pid_file" ]]; then
        fail "missing expected PID file for worker $i"
    fi

    expected_pid="$(cat "$expected_pid_file")"

    if ! grep -q "PID $expected_pid" "$TMPDIR/master.out"; then
        fail "master output does not contain registration from expected worker PID $expected_pid"
    fi
done

if [[ -s "$TMPDIR/master.err" ]]; then
    fail "master wrote unexpected output to stderr"
fi

for i in $(seq 1 "$N"); do
    if [[ -s "$TMPDIR/worker-$i.err" ]]; then
        fail "worker $i wrote unexpected output to stderr"
    fi
done

pass "master received exactly $N valid registration requests and terminated correctly"

echo
echo "--- master stdout ---"
cat "$TMPDIR/master.out"
