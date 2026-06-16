#!/usr/bin/env bash

set -u

MASTER_SRC="master-4.c"
WORKER_SRC="worker-4.c"
REG_FIFO="/tmp/registration-fifo"
SHM_FILE="/dev/shm/numbers-shm"

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

fail() {
    echo "ERRORE: $1"

    if [[ -d "${TMPDIR:-}" ]]; then
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
        echo "--- output atteso dei risultati ---"
        cat "$TMPDIR/expected-results.out" 2>/dev/null || true
    fi

    exit 1
}

ok() {
    echo "OK: $1"
}

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
    fail "file sorgente mancante: $MASTER_SRC"
fi

if [[ ! -f "$WORKER_SRC" ]]; then
    fail "file sorgente mancante: $WORKER_SRC"
fi

TMPDIR="$(mktemp -d)"
MASTER_BIN="$TMPDIR/master-4"
WORKER_BIN="$TMPDIR/worker-4"

cleanup() {
    rm -f "$REG_FIFO"
    rm -f /tmp/worker-*-fifo
    rm -f "$SHM_FILE"
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

rm -f "$REG_FIFO"
rm -f /tmp/worker-*-fifo
rm -f "$SHM_FILE"

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
: > "$TMPDIR/expected-results.out"

for i in $(seq 0 $((N - 1))); do
    number="${TEST_NUMBERS[$i]}"
    echo "$number" >> "$TMPDIR/numbers.txt"

    if is_prime "$number"; then
        echo "$number is prime" >> "$TMPDIR/expected-results.out"
    else
        echo "$number is not prime" >> "$TMPDIR/expected-results.out"
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

    sleep 0.1
done

if [[ "$fifo_ready" -ne 1 ]]; then
    kill "$MASTER_PID" 2>/dev/null || true
    fail "il master non ha creato $REG_FIFO"
fi

echo "Avvio $N worker..."

worker_pids=()

for i in $(seq 1 "$N"); do
    expected_pid_file="$TMPDIR/expected-worker-$i.pid"

    (
        cd "$TMPDIR" || exit 1

        # Il wrapper salva il proprio PID prima di eseguire exec().
        # Dopo exec(), il programma worker mantiene lo stesso PID.
        timeout "$TIMEOUT_SECONDS" bash -c \
            'echo "$BASHPID" > "$1"; exec "$0"' \
            "$WORKER_BIN" "$expected_pid_file" \
            >"$TMPDIR/worker-$i.out" 2>"$TMPDIR/worker-$i.err"
    ) &

    worker_pids+=("$!")
done

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

wait "$MASTER_PID"
master_rc=$?

if [[ "$master_rc" -eq 124 ]]; then
    fail "il master è andato in timeout"
fi

if [[ "$master_rc" -ne 0 ]]; then
    fail "il master è terminato con codice $master_rc"
fi

if [[ -s "$TMPDIR/master.err" ]]; then
    fail "il master ha scritto su stderr"
fi

for i in $(seq 1 "$N"); do
    if [[ -s "$TMPDIR/worker-$i.err" ]]; then
        fail "il worker $i ha scritto su stderr"
    fi

    if [[ -s "$TMPDIR/worker-$i.out" ]]; then
        fail "il worker $i ha scritto su stdout, ma nello step 4 i worker devono restare silenziosi"
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

if grep -q "^ERROR:" "$TMPDIR/master.out"; then
    fail "il master ha rilevato almeno un errore nei valori isprime"
fi

if grep -q "was not processed" "$TMPDIR/master.out"; then
    fail "almeno un valore isprime è rimasto a -1"
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

ACTUAL_RESULTS="$TMPDIR/actual-results.out"

grep -E '^[0-9]+ is( not)? prime$' "$TMPDIR/master.out" > "$ACTUAL_RESULTS" || true

result_count=$(wc -l < "$ACTUAL_RESULTS")

if [[ "$result_count" -ne "$N" ]]; then
    fail "attese $N righe di risultato stampate dal master, trovate $result_count"
fi

SORTED_EXPECTED="$TMPDIR/expected-results.sorted"
SORTED_ACTUAL="$TMPDIR/actual-results.sorted"

sort "$TMPDIR/expected-results.out" > "$SORTED_EXPECTED"
sort "$ACTUAL_RESULTS" > "$SORTED_ACTUAL"

if ! diff -u "$SORTED_EXPECTED" "$SORTED_ACTUAL" > "$TMPDIR/diff.out"; then
    echo "--- differenze tra risultati attesi e risultati reali ---"
    cat "$TMPDIR/diff.out"
    fail "i risultati stampati dal master non corrispondono ai risultati attesi"
fi

if [[ -e "$REG_FIFO" ]]; then
    fail "$REG_FIFO esiste ancora dopo la terminazione del master"
fi

ok "step 4 validato: $N worker aggiornano isprime nella shared memory, non stampano nulla, e il master stampa i risultati corretti"

echo
echo "--- stdout master ---"
cat "$TMPDIR/master.out"

echo
echo "--- risultati verificati ---"
sort "$ACTUAL_RESULTS"
