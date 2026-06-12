#!/bin/bash

# Check if a log file was provided
LOG_FILE="$1"

if [[ -z "$LOG_FILE" ]]; then
    echo "Usage: $0 <path_to_access.log>"
    exit 1
fi
if [[ ! -f "$LOG_FILE" ]]; then
    echo "Error: File '$LOG_FILE' not found."
    exit 1
fi

echo "Analyzing $LOG_FILE for 404 and 500 errors..."

# grep: Filter lines containing a space, followed by 404 or 500, followed by another space.
# sed -E: Uses Extended Regular Expressions to capture fields and reformat them.
grep -E ' (404|500) ' "$LOG_FILE" | \
sed -E 's/^([^ ]+) .* \[(.*)\] ".*" (404|500) .*/Error \3 from IP \1 at time \2/'

