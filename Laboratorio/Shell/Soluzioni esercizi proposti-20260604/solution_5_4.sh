#!/bin/bash

# Check if a target HTML file was provided
HTML_FILE="$1"

if [[ -z "$HTML_FILE" ]]; then
    echo "Usage: $0 <path_to_file.html>"
    exit 1
fi
if [[ ! -f "$HTML_FILE" ]]; then
    echo "Error: File '$HTML_FILE' not found."
    exit 1
fi
# Run the single sed pipeline using Extended Regular Expressions (-E)
sed -E '
    # Bold conversion
    s/<b>(.*?)<\/b>/**\1**/g
    s/<strong>(.*?)<\/strong>/**\1**/g
    # Italic conversion
    s/<i>(.*?)<\/i>/*\1*/g
    s/<em>(.*?)<\/em>/*\1*/g
    # Headers conversion (h1 to h6)
    s/<h1>(.*?)<\/h1>/# \1/g
    s/<h2>(.*?)<\/h2>/## \1/g
    s/<h3>(.*?)<\/h3>/### \1/g
    s/<h4>(.*?)<\/h4>/#### \1/g
    s/<h5>(.*?)<\/h5>/##### \1/g
    s/<h6>(.*?)<\/h6>/###### \1/g
' "$HTML_FILE"

