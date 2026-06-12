#!/bin/bash
# Threshold date (e.g., "7 days ago", "1 month ago", "2026-05-01")
AGE_THRESHOLD="7 days ago"
# Size threshold in bytes (10MB = 10 * 1024 * 1024)
SIZE_THRESHOLD=$((10 * 1024 * 1024))
# Warning: Notation ((SIZE_THRESHOLD=10 * 1024 * 1024))
# Works in bash but is not POSIX compliant
# Temporary reference file name
REF_FILE=".age_threshold_marker"
# touch -d creates a file with a specific modification timestamp
if ! touch -d "$AGE_THRESHOLD" "$REF_FILE" 2>/dev/null; then
    echo "Error: Invalid date format for AGE_THRESHOLD ('$AGE_THRESHOLD')."
    exit 1
fi
echo "Age Threshold: Older than $AGE_THRESHOLD"
echo "Size Threshold: Larger than 10MB ($SIZE_THRESHOLD bytes)"
for log_file in *.log; do
    # Skip directories that happen to end in .log
    # This works also in the case where no .log file exists ans the for loops over non-existing file '*.log'
    [ -f "$log_file" ] || continue
    # Check if the file is OLDER than the threshold
    if [ "$log_file" -ot "$REF_FILE" ]; then
        # Get file size in bytes using stat
        file_size=$(stat -c "%s" "$log_file")
        # Old AND larger than 10MB -> Delete it
        if [ "$file_size" -gt "$SIZE_THRESHOLD" ]; then
            echo rm "$log_file"
        # Old but smaller than 10MB -> Compress it
        else
            echo gzip "$log_file"
        fi
    fi
done
rm -f "$REF_FILE"

