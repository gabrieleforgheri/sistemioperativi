#!/usr/bin/env bash

set -u

MASTER_SRC="master-2.c"
WORKER_SRC="worker-2.c"
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
        echo "--- worker stdout files ---"
        cat "$TMPDIR"/worker-*.out 2>/dev/null || true

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
MASTER_BIN="$TMPDIR/master-2"
WORKER_BIN="$TMPDIR/worker-2"

cleanup() {
    rm -f "$REG_FIFO"
    rm -f /tmp/worker-*-fifo
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

rm -f "$REG_FIFO"
rm -f /tmp/worker-*-fifo

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

echo "Starting master for $N workers..."
timeout "$TIMEOUT_SECONDS" "$MASTER_BIN" "$N" >"$TMPDIR/master.out" 2>"$TMPDIR/master.err" &
MASTER_TIMEOUT_PID=$!

fifo_ready=0

for _ in $(seq 1 50); do
    if [[ -p "$REG_FIFO" ]]; then
        fifo_ready=1
        break
    fi

    sleep 0.1
done

if [[ "$fifo_ready" -ne 1 ]]; then
    kill "$MASTER_TIMEOUT_PID" 2>/dev/null || true
    fail "master did not create $REG_FIFO"
fi

echo "Launching $N workers..."

worker_timeout_pids=()

for i in $(seq 1 "$N"); do
    expected_pid_file="$TMPDIR/expected-worker-$i.pid"

    # This wrapper writes its own PID before exec().
    # After exec(), the worker keeps the same PID.
    timeout "$TIMEOUT_SECONDS" bash -c \
        'echo "$BASHPID" > "$1"; exec "$0"' \
        "$WORKER_BIN" "$expected_pid_file" \
        >"$TMPDIR/worker-$i.out" 2>"$TMPDIR/worker-$i.err" &

    worker_timeout_pids+=("$!")
done

for pid in "${worker_timeout_pids[@]}"; do
    wait "$pid"
    worker_rc=$?

    if [[ "$worker_rc" -eq 124 ]]; then
        fail "a worker timed out"
    fi

    if [[ "$worker_rc" -ne 0 ]]; then
        fail "a worker exited with code $worker_rc"
    fi
done

wait "$MASTER_TIMEOUT_PID"
master_rc=$?

if [[ "$master_rc" -eq 124 ]]; then
    fail "master timed out before serving $N workers"
fi

if [[ "$master_rc" -ne 0 ]]; then
    fail "master exited with code $master_rc"
fi

registration_count=$(grep -c "registration request received from PID" "$TMPDIR/master.out" || true)
sent_count=$(grep -c "sent index" "$TMPDIR/master.out" || true)

if [[ "$registration_count" -ne "$N" ]]; then
    fail "expected $N master registration messages, found $registration_count"
fi

if [[ "$sent_count" -ne "$N" ]]; then
    fail "expected $N master index-send messages, found $sent_count"
fi

if [[ -s "$TMPDIR/master.err" ]]; then
    fail "master wrote unexpected output to stderr"
fi

for i in $(seq 1 "$N"); do
    if [[ -s "$TMPDIR/worker-$i.err" ]]; then
        fail "worker $i wrote unexpected output to stderr"
    fi
done

ALL_WORKERS_OUT="$TMPDIR/all-workers.out"
cat "$TMPDIR"/worker-*.out > "$ALL_WORKERS_OUT"

worker_line_count=$(grep -Ec '^Worker [0-9]+ - index [0-9]+$' "$ALL_WORKERS_OUT" || true)

if [[ "$worker_line_count" -ne "$N" ]]; then
    fail "expected $N correctly formatted worker output lines, found $worker_line_count"
fi

for i in $(seq 1 "$N"); do
    expected_pid_file="$TMPDIR/expected-worker-$i.pid"

    if [[ ! -f "$expected_pid_file" ]]; then
        fail "missing expected PID file for worker $i"
    fi

    expected_pid="$(cat "$expected_pid_file")"

    if ! grep -q "^Worker $expected_pid - index [0-9][0-9]*$" "$ALL_WORKERS_OUT"; then
        fail "worker output does not contain expected PID $expected_pid"
    fi

    if ! grep -q "PID $expected_pid" "$TMPDIR/master.out"; then
        fail "master output does not contain expected PID $expected_pid"
    fi
done

INDEX_FILE="$TMPDIR/indices.txt"
sed -n 's/^Worker [0-9][0-9]* - index \([0-9][0-9]*\)$/\1/p' "$ALL_WORKERS_OUT" | sort -n > "$INDEX_FILE"

for expected_index in $(seq 0 $((N - 1))); do
    occurrences=$(grep -xc "$expected_index" "$INDEX_FILE" || true)

    if [[ "$occurrences" -ne 1 ]]; then
        fail "expected index $expected_index exactly once, found $occurrences occurrences"
    fi
done

extra_indices=$(awk -v max="$((N - 1))" '$1 < 0 || $1 > max { print }' "$INDEX_FILE")

if [[ -n "$extra_indices" ]]; then
    fail "found unexpected indices: $extra_indices"
fi

pass "step 2 validated: $N workers registered, received indices 0..$((N - 1)), printed correct output, and terminated"

echo
echo "--- master stdout ---"
cat "$TMPDIR/master.out"

echo
echo "--- workers stdout ---"
sort -n -k5 "$ALL_WORKERS_OUT"
