#!/usr/bin/env bash

set -u

MASTER_SRC="master-3.c"
WORKER_SRC="worker-3.c"

REG_FIFO="/tmp/registration-fifo"

N="${1:-8}"
TIMEOUT_SECONDS=8

TEST_NUMBERS=(
    2
    3
    4
    5
    9
    11
    15
    17
    1000003
    1000033
    1000037
    1000039
    1000081
    1000099
    1000117
    1000121
    1234567
    15485863
    15485867
    20000003
    20000033
    32452843
    32452867
    49999991
    50000017
    67867967
    86028121
    99999989
    100000007
    123456789
)

TMPDIR=""

fail() {
    echo "ERRORE: $1"

    if [[ -n "${TMPDIR:-}" && -d "$TMPDIR" ]]; then
        echo
        echo "--- stdout master ---"
        cat "$TMPDIR/master.out" 2>/dev/null || true

        echo
        echo "--- stderr master ---"
        cat "$TMPDIR/master.err" 2>/dev/null || true

        echo
        echo "--- stdout worker ---"
        cat "$TMPDIR"/worker-*.out 2>/dev/null || true

        echo
        echo "--- stderr worker ---"
        cat "$TMPDIR"/worker-*.err 2>/dev/null || true

        echo
        echo "--- output atteso ---"
        cat "$TMPDIR/expected.out" 2>/dev/null || true
    fi

    exit 1
}

ok() {
    echo "OK: $1"
}

cleanup() {
    if [[ -n "${WORKER_BIN:-}" ]]; then
        pkill -f "$WORKER_BIN" 2>/dev/null || true
    fi

    rm -f "$REG_FIFO"
    rm -f /tmp/worker-*-fifo

    # Cleanup for the two most common shm names used in this exercise.
    rm -f /dev/shm/numbers-shm
    rm -f /dev/shm/numbers-shm 2>/dev/null || true

    if [[ -n "${TMPDIR:-}" && -d "$TMPDIR" ]]; then
        rm -rf "$TMPDIR"
    fi
}
trap cleanup EXIT

is_prime() {
    local x="$1"

    if (( x < 2 )); then
        return 1
    fi

    if (( x == 2 )); then
        return 0
    fi

    if (( x % 2 == 0 )); then
        return 1
    fi

    local d=3

    while (( d * d <= x )); do
        if (( x % d == 0 )); then
            return 1
        fi

        d=$((d + 2))
    done

    return 0
}

if ! [[ "$N" =~ ^[1-9][0-9]*$ ]]; then
    fail "N deve essere un intero positivo"
fi

if (( N > ${#TEST_NUMBERS[@]} )); then
    fail "questo validatore supporta al massimo ${#TEST_NUMBERS[@]} worker"
fi

if [[ ! -f "$MASTER_SRC" ]]; then
    fail "file sorgente master mancante: $MASTER_SRC"
fi

if [[ ! -f "$WORKER_SRC" ]]; then
    fail "file sorgente worker mancante: $WORKER_SRC"
fi

TMPDIR="$(mktemp -d)"
MASTER_BIN="$TMPDIR/master-3"
WORKER_BIN="$TMPDIR/worker-3"

rm -f "$REG_FIFO"
rm -f /tmp/worker-*-fifo
rm -f /dev/shm/numbers-shm
rm -f /dev/shm/numbers-shm 2>/dev/null || true

echo "Uso master: $MASTER_SRC"
echo "Uso worker: $WORKER_SRC"

echo "Compilo $MASTER_SRC..."
gcc -Wall -Wextra -o "$MASTER_BIN" "$MASTER_SRC" -lrt >"$TMPDIR/master-gcc.out" 2>"$TMPDIR/master-gcc.err"

if [[ $? -ne 0 ]]; then
    echo "--- stderr compilatore master ---"
    cat "$TMPDIR/master-gcc.err"
    fail "compilazione di $MASTER_SRC fallita"
fi

echo "Compilo $WORKER_SRC..."
gcc -Wall -Wextra -o "$WORKER_BIN" "$WORKER_SRC" -lrt >"$TMPDIR/worker-gcc.out" 2>"$TMPDIR/worker-gcc.err"

if [[ $? -ne 0 ]]; then
    echo "--- stderr compilatore worker ---"
    cat "$TMPDIR/worker-gcc.err"
    fail "compilazione di $WORKER_SRC fallita"
fi

echo "Creo numbers.txt di test..."
: > "$TMPDIR/numbers.txt"
: > "$TMPDIR/expected.out"

for i in $(seq 0 $((N - 1))); do
    number="${TEST_NUMBERS[$i]}"
    echo "$number" >> "$TMPDIR/numbers.txt"

    if is_prime "$number"; then
        echo "$number is prime" >> "$TMPDIR/expected.out"
    else
        echo "$number is not prime" >> "$TMPDIR/expected.out"
    fi
done

echo "Avvio il master con $N worker attesi..."
(
    cd "$TMPDIR" || exit 1
    timeout "$TIMEOUT_SECONDS" "$MASTER_BIN" "$N" >"$TMPDIR/master.out" 2>"$TMPDIR/master.err"
) &
MASTER_PID=$!

fifo_ready=0

for _ in $(seq 1 50); do
    if [[ -p "$REG_FIFO" ]]; then
        fifo_ready=1
        break
    fi

    if ! kill -0 "$MASTER_PID" 2>/dev/null; then
        break
    fi

    sleep 0.1
done

if [[ "$fifo_ready" -ne 1 ]]; then
    wait "$MASTER_PID" 2>/dev/null || true
    fail "il master non ha creato $REG_FIFO"
fi

echo "Avvio $N worker..."

worker_pids=()

for i in $(seq 1 "$N"); do
    expected_pid_file="$TMPDIR/expected-worker-$i.pid"

    (
        cd "$TMPDIR" || exit 1

        # Il wrapper salva il proprio PID prima di exec().
        # Dopo exec(), il worker mantiene lo stesso PID.
        timeout "$TIMEOUT_SECONDS" bash -c \
            'echo "$BASHPID" > "$1"; exec "$0"' \
            "$WORKER_BIN" "$expected_pid_file" \
            >"$TMPDIR/worker-$i.out" 2>"$TMPDIR/worker-$i.err"
    ) &

    worker_pids+=("$!")
done

master_finished=0
for _ in $(seq 1 $((TIMEOUT_SECONDS * 10 + 10))); do
    if ! kill -0 "$MASTER_PID" 2>/dev/null; then
        master_finished=1
        break
    fi

    sleep 0.1
done

if [[ "$master_finished" -ne 1 ]]; then
    kill "$MASTER_PID" 2>/dev/null || true
    fail "il master è andato in timeout"
fi

wait "$MASTER_PID"
master_rc=$?

if [[ "$master_rc" -eq 124 ]]; then
    for pid in "${worker_pids[@]}"; do
        kill "$pid" 2>/dev/null || true
    done
    pkill -f "$WORKER_BIN" 2>/dev/null || true
    fail "il master è andato in timeout"
fi

if [[ "$master_rc" -ne 0 ]]; then
    for pid in "${worker_pids[@]}"; do
        kill "$pid" 2>/dev/null || true
    done
    pkill -f "$WORKER_BIN" 2>/dev/null || true

    if grep -q "shm_open: Invalid argument" "$TMPDIR/master.err"; then
        fail "il master è fallito su shm_open: nome della shared memory probabilmente non valido per shm_open"
    fi

    fail "il master è terminato con codice $master_rc"
fi

for pid in "${worker_pids[@]}"; do
    wait "$pid"
    worker_rc=$?

    if [[ "$worker_rc" -eq 124 ]]; then
        fail "un worker è andato in timeout, probabilmente non ha ricevuto SIGUSR1"
    fi

    if [[ "$worker_rc" -ne 0 ]]; then
        fail "un worker è terminato con codice $worker_rc"
    fi
done

if [[ -s "$TMPDIR/master.err" ]]; then
    fail "il master ha scritto su stderr"
fi

for i in $(seq 1 "$N"); do
    if [[ -s "$TMPDIR/worker-$i.err" ]]; then
        fail "il worker $i ha scritto su stderr"
    fi
done

registration_count=$(grep -c "registration request received from PID" "$TMPDIR/master.out" || true)
index_count=$(grep -c "sent index" "$TMPDIR/master.out" || true)
signal_count=$(grep -c "sent SIGUSR1 to worker" "$TMPDIR/master.out" || true)

if [[ "$registration_count" -ne "$N" ]]; then
    fail "attesi $N messaggi di registrazione, trovati $registration_count"
fi

if [[ "$index_count" -ne "$N" ]]; then
    fail "attesi $N messaggi di invio indice, trovati $index_count"
fi

if [[ "$signal_count" -ne "$N" ]]; then
    fail "attesi $N messaggi di invio SIGUSR1, trovati $signal_count"
fi

for i in $(seq 1 "$N"); do
    expected_pid_file="$TMPDIR/expected-worker-$i.pid"

    if [[ ! -f "$expected_pid_file" ]]; then
        fail "file con PID atteso mancante per il worker $i"
    fi

    expected_pid="$(cat "$expected_pid_file")"

    if ! grep -q "PID $expected_pid" "$TMPDIR/master.out"; then
        fail "l'output del master non contiene il PID atteso $expected_pid"
    fi

    if ! grep -q "sent SIGUSR1 to worker $expected_pid" "$TMPDIR/master.out"; then
        fail "il master non conferma l'invio di SIGUSR1 al worker $expected_pid"
    fi
done

ALL_WORKERS_OUT="$TMPDIR/all-workers.out"
cat "$TMPDIR"/worker-*.out > "$ALL_WORKERS_OUT"

worker_result_count=$(grep -Ec '^[0-9]+ is( not)? prime$' "$ALL_WORKERS_OUT" || true)

if [[ "$worker_result_count" -ne "$N" ]]; then
    fail "attese $N righe di output dei worker, trovate $worker_result_count"
fi

SORTED_EXPECTED="$TMPDIR/expected.sorted"
SORTED_ACTUAL="$TMPDIR/actual.sorted"

sort "$TMPDIR/expected.out" > "$SORTED_EXPECTED"
sort "$ALL_WORKERS_OUT" > "$SORTED_ACTUAL"

if ! diff -u "$SORTED_EXPECTED" "$SORTED_ACTUAL" > "$TMPDIR/diff.out"; then
    echo "--- differenze tra output atteso e output reale ---"
    cat "$TMPDIR/diff.out"
    fail "l'output dei worker non corrisponde ai risultati attesi del test di primalità"
fi

if [[ -e "$REG_FIFO" ]]; then
    fail "$REG_FIFO esiste ancora dopo la terminazione del master"
fi

ok "step 3 validato: $N worker ricevono SIGUSR1, leggono la shared memory e stampano i risultati corretti"

echo
echo "--- stdout master ---"
cat "$TMPDIR/master.out"

echo
echo "--- stdout worker ---"
sort "$ALL_WORKERS_OUT"
