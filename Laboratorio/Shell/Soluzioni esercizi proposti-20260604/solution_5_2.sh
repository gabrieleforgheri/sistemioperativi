#!/bin/bash
# Example format in config file: include /path/to/file.conf
INCLUDE_KEYWORD="include"

# Recursive processing function
process_file() {
    local file_path="$1"
    # Verify the file exists before processing
    if [[ ! -f "$file_path" ]]; then
        echo "ERROR: Include file '$file_path' not found" >&2
        return
    fi

    # Sed script
    # s/#.*//            -> Removes everything from '#' to the end of the line (Inline comments)
    # s/^[[:space:]]*$// -> Turns lines with only whitespace into completely empty lines
    # /^$/d              -> Deletes all completely empty lines. This should be the last command!
    sed -e 's/#.*//' -e 's/^[[:space:]]*$//' -e '/^$/d' "$file_path" | while read -r line; do
        # Check if the stripped line is an include statement
        # read -r absorbs multiple spaces naturally
        include_target=$(echo "$line" | sed -n "s/^$INCLUDE_KEYWORD[[:space:]]\+\(.\+\)/\1/p")
        if [[ -n $include_target ]]; then
            # process recursively. [[ is required to cope with empty varaibles
            process_file "$include_target"
        else
            # Print the clean configuration line
            echo "$line"
        fi
    done
}

# Check if a starting configuration file was provided
if [[ -z "$1" ]]; then
    echo "Usage: $0 <main_config_file>"
    exit 1
fi

process_file "$1"
