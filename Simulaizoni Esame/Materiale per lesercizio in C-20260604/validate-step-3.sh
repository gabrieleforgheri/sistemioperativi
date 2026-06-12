#!/usr/bin/env bash
set -euo pipefail

src="${1:-log-shm-summary.c}"
tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT

cat > "$tmpdir/test.log" <<'LOG'
INFO normal line
WARNING login FAIL
ERROR service failure
DENIED user=root
INFO another normal line
ERROR FAIL DENIED ROOT WARNING
LOG

if grep -qE '\bread_all\b|\bwrite_all\b' "$src"; then
    echo "ERRORE: il sorgente non deve usare read_all() o write_all()" >&2
    exit 1
fi

gcc "$src" -o "$tmpdir/prog"

output="$("$tmpdir/prog" "$tmpdir/test.log" 6)"
normalized="$(printf "%s\n" "$output" | sed -E 's/pid=[0-9]+/pid=PID/g')"

expected="$(cat <<'EXP'
riga 0: pid=PID score=0
riga 1: pid=PID score=4
riga 2: pid=PID score=5
riga 3: pid=PID score=8
riga 4: pid=PID score=0
riga 5: pid=PID score=17

righe elaborate: 6
score totale: 34
score medio: 5.67
score massimo: 17
riga con score massimo: 5
EXP
)"

if [ "$normalized" != "$expected" ]; then
    echo "Output inatteso." >&2
    echo "=== output normalizzato ===" >&2
    printf "%s\n" "$normalized" >&2
    echo "=== atteso ===" >&2
    printf "%s\n" "$expected" >&2
    exit 1
fi

echo "OK: Step 3 validato"
