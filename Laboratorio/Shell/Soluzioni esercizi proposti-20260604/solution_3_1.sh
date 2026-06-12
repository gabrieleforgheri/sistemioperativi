#!/bin/bash

# Sanity checks
if [ -z "$1" ]; then
    echo "Error: Target directory parameter is missing."
    echo "Usage: $0 /path/to/directory"
    exit 1
fi
TARGET_DIR="$1"
if [ ! -d "$TARGET_DIR" ]; then
    echo "Error: '$TARGET_DIR' is not a valid directory."
    exit 1
fi
cd "$TARGET_DIR" || exit 1
# Declare an associative array to store unique extensions and their counts
declare -A EXT_COUNTS

# Identify Extensions & Count Files
for file in *; do
    [[ -f "$file" ]] || continue
    ext=$(echo $file | cut -d. -f2-)
    if [[ -z $ext ]]; then 
        ext="no_extension"
    fi
    # Track unique extensions by counting them
    ((EXT_COUNTS[$ext]++))
done

# If no files were found, exit early
if [ ${#EXT_COUNTS[@]} -eq 0 ]; then
    echo "No files found to organize."
    exit 0
fi

# Create dir and Move Files via xargs
for ext in "${!EXT_COUNTS[@]}"; do
    mkdir -p "$ext"
    for file in *.$ext; do
        printf '%s\0' "$file"
    done | xargs -0 -I{} mv {} $ext
done 

# Final report
for ext in "${!EXT_COUNTS[@]}"; do
    echo "Extension [.$ext]: ${EXT_COUNTS[$ext]} file(s) moved."
done
