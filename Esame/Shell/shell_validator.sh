#!/bin/bash

if [ -z "$1" ]; then
    echo "Uso: $0 <nome_script.sh>"
    exit 1
fi

SCRIPT_UNDER_TEST="$1"
if [[ ! "$SCRIPT_UNDER_TEST" =~ ^/ ]] && [[ ! "$SCRIPT_UNDER_TEST" =~ ^\./ ]]; then
    SCRIPT_UNDER_TEST="./$SCRIPT_UNDER_TEST"
fi
TEST_DIR="/tmp/test_env"
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"

# Controlla se lo script da testare esiste
if [ ! -f "$SCRIPT_UNDER_TEST" ]; then
    echo "Errore: Lo script da testare ($SCRIPT_UNDER_TEST) non esiste."
    exit 1
fi
chmod +x "$SCRIPT_UNDER_TEST"

# --- Setup dei file sorgente per i test ---
# Sorgente corretto (Ritorna successo)
cat << 'EOF' > "$TEST_DIR/ok.c"
#include <stdio.h>
int main() { return 0; }
EOF

# Sorgente corretto che fallisce (Ritorna errore)
cat << 'EOF' > "$TEST_DIR/fail.c"
#include <stdio.h>
int main() { return 42; }
EOF

# Sorgente che va in timeout (Sleep 8 secondi)
cat << 'EOF' > "$TEST_DIR/timeout.c"
#include <unistd.h>
int main() { sleep(8); return 0; }
EOF

# Sorgente che non compila (Errore di sintassi)
cat << 'EOF' > "$TEST_DIR/syntax_error.c"
int main() { questo_non_compila; }
EOF

# File non leggibile
touch "$TEST_DIR/unreadable.c"
chmod -r "$TEST_DIR/unreadable.c"

RED='\033[0;31m'
GREEN='\033[0;32m'
RESET='\033[0m'

# Test valdiator
# Parametri: $1=Nome_Test, $2=Output_Ottenuto, $3=Stringa_Attesa_Nel_Finale
assert_output() {
    local test_name="$1"
    local output="$2"
    local expected="$3"
    
    # Prendiamo l'ultima riga dell'output per controllare il messaggio finale
    local last_line
    last_line=$(echo "$output" | tail -n 1)

    if [ "$last_line" == "$expected" ]; then
        echo -e "$test_name: ${GREEN}PASS${RESET}"
    else
        echo -e "$test_name: ${RED}FAIL${RESET}"
        echo "   -> Atteso come ultima riga: '$expected'"
        echo "   -> Ricevuto totale: '$output'"
    fi
}

# Step 1: Invocazione con numero di parametri sbagliato
out=$( "$SCRIPT_UNDER_TEST" "$TEST_DIR/ok.c" 2>&1 )
assert_output "Step 1: Parametri errati" "$out" "- Parameter error"

# Step 2a:  Sorgente non esistente
out=$( "$SCRIPT_UNDER_TEST" "$TEST_DIR/non_esisto.c" "$TEST_DIR/dist" 2>&1 )
assert_output "Step 2: Sorgente non esistente" "$out" "- Source error"

# Step 2b: Sorgente non leggibile
out=$( "$SCRIPT_UNDER_TEST" "$TEST_DIR/unreadable.c" "$TEST_DIR/dist" 2>&1 )
assert_output "Step 2: Sorgente non leggibile" "$out" "- Source error"

# Step 3: Sorgente C che non si compila
out=$( "$SCRIPT_UNDER_TEST" "$TEST_DIR/syntax_error.c" "$TEST_DIR/dist" 2>&1 )
assert_output "Step 3: Errore compilazione" "$out" "- Compiling syntax_error error"

# Step 4: Directory di destinazione non scrivibile
NON_WRITABLE="$TEST_DIR/noperm_dir"
mkdir -p "$NON_WRITABLE"
chmod 555 "$NON_WRITABLE" # Lettura ed esecuzione, no scrittura

out=$( "$SCRIPT_UNDER_TEST" "$TEST_DIR/ok.c" "$NON_WRITABLE" 2>&1 )
assert_output "Step 4: Destinazione non scrivibile" "$out" "- Install error"

# Ripristino per la pulizia
chmod 755 "$NON_WRITABLE"

# Step 5a: Eseguibile C che ritorna successo (0)
out=$( "$SCRIPT_UNDER_TEST" "$TEST_DIR/ok.c" "$TEST_DIR/dist_ok" 2>&1 )
assert_output "Step 5: Esecuzione con Successo" "$out" "+ Execution OK"

#  Step 5b: Eseguibile C che ritorna errore
out=$( "$SCRIPT_UNDER_TEST" "$TEST_DIR/fail.c" "$TEST_DIR/dist_fail" 2>&1 )
assert_output "Step 5: Esecuzione con Errore" "$out" "- Execution error"

# Step 6: Eseguibile C che va in timeout (sleep 8 secondi)
echo "Timout Test (5 sec.)..."
out=$( "$SCRIPT_UNDER_TEST" "$TEST_DIR/timeout.c" "$TEST_DIR/dist_timeout" 2>&1 )
assert_output "Step 6: Timeout (8s sleep)" "$out" "- Timeout error"

# Pulizia ambiente di test
rm -rf "$TEST_DIR"

