#!/bin/bash

ACCESS_LOG="access.log"
AUTH_LOG="auth.log"
EXPECTED_DIR="expected_outputs"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color
TMP=$$

# Command line parsing
if [ $# -eq 1 ]; then 
    if [[ "$1" == /* ]]; then
        # Absolute path
        SCRIPT="$1"
    else
        # relative path
        if [[ "$1" == ./* ]]; then
            SCRIPT="$1"
        else
            SCRIPT="./$1"
        fi
    fi
else
    echo "Usage: $0 script_name.sh" >&2
    exit 1
fi

FILES_TO_VALIDATE=("web.ip" "ssh.ip" "bad_ip.csv" "bad_prefix.csv" "output.txt")

# Prerequisites
if [[ ! -x "$SCRIPT" ]]; then
    echo -e "${RED}Error:${NC} $SCRIPT is not executable or doesn't exist."
    exit 1
fi

if [[ ! -f "$ACCESS_LOG" || ! -f "$AUTH_LOG" ]]; then
    echo -e "${RED}Error:${NC} Source logs ($ACCESS_LOG and/or $AUTH_LOG) are missing."
    exit 1
fi

if [[ ! -d "$EXPECTED_DIR" ]]; then
    echo -e "${RED}Error:${NC} Reference directory '$EXPECTED_DIR/' not found."
    echo "Please create it and populate it with your expected baseline files."
    exit 1
fi

# Clean up
rm -f "${FILES_TO_VALIDATE[@]}"

# Execute script
echo "Running script"
$SCRIPT "$ACCESS_LOG" "$AUTH_LOG" > output.txt
RETVAL=$?

if [[ $RETVAL -ne 0 ]]; then
    echo -e "${RED}[FAIL]${NC} The script crashed or exited with an error code ($RETVAL)."
    exit 1
fi

echo -e "${GREEN}[OK]${NC} Script execution finished successfully."

# Validate intermediate file
ALL_PASSED=true

for file in "${FILES_TO_VALIDATE[@]}"; do
    echo -n "Validating '$file': "

    # Check if file exists
    if [[ ! -f "$file" ]]; then
        echo -e "${RED}[MISSING]${NC} File was not generated."
        ALL_PASSED=false
        continue
    fi
    if [[ ! -f "$EXPECTED_DIR/$file" ]]; then
        echo -e "${RED}[NO REFERENCE]${NC} Missing '$EXPECTED_DIR/$file' to compare against."
        ALL_PASSED=false
        continue
    fi

    # Compare files 
    if [[ "$file" == "bad_ip.csv" || "$file" == "bad_prefix.csv" ]]; then
        diff <(sort "$file") <(sort "$EXPECTED_DIR/$file") > $TMP
    else
        diff "$file" "$EXPECTED_DIR/$file" > $TMP
    fi
    DIFF_RETVAL=$?

    if [[ $DIFF_RETVAL -eq 0 ]]; then
        echo -e "${GREEN}[PASS]${NC} File matches the reference exactly."
    else
        echo -e "${RED}[MISMATCH]${NC} File contents differ from the reference."
        ALL_PASSED=false
        cat $TMP | head -n 10
    fi
done
rm -f $TMP
# Final Report
echo "--------------------------------------------------"
if [ "$ALL_PASSED" = true ]; then
    echo -e "${GREEN}SUCCESS: All intermediate files are consistent and structurally valid!${NC}"
    exit 0
else
    echo -e "${RED}FAILURE: One or more pipeline stages produced inconsistent data.${NC}"
    exit 1
fi
