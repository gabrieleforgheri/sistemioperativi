#!/bin/bash
# Check if a target directory was provided, otherwise default to the current directory
TARGET_DIR="${1:-.}"
# Verify the target directory exists
if [ ! -d "$TARGET_DIR" ]; then
    echo "Error: Directory '$TARGET_DIR' does not exist."
    exit 1
fi
# LC_ALL=C ensures predictable output sorting and formatting
# ls -R lists all files recursively; -p adds a '/' trailing slash to directories
# grep -v '/$' filters out directory names so we only process files
# xargs -I {} passes each file path safely to the 'file' utility, -d'\n' uses newline as delimiter
# cut, sort, and uniq aggregate the results from highest to lowest occurrence
cd "$TARGET_DIR" || exit 1
LC_ALL=C ls -Rp 2>/dev/null | grep -v '/$' | grep -v '^$' | grep -v ':' | \
    xargs -d '\n' -I {} file --mime-type "{}" 2>/dev/null | \
    cut -d' ' -f2- | \
    sort | \
    uniq -c | \
    sort -rn | \
    while read -r count mime; do
        printf "%-7d %s\n" "$count" "$mime"
    done

