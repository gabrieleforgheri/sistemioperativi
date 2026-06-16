#!/bin/bash

TEST_DIR="./test_hierarchy"
EXPECTED_OUTPUT="./expected_output.txt"
ACTUAL_OUTPUT="./actual_output.txt"
SOLUTION_SCRIPT="$1" # Change this to the script name you are testing

# Ensure the student script exists and is executable
if [ ! -x "$SOLUTION_SCRIPT" ]; then
    echo "Error: Target script '$SOLUTION_SCRIPT' not found or not executable."
    exit 1
fi

# Initial setup
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR/Dir A/SubDir B"
mkdir -p "$TEST_DIR/Dir C"
mkdir -p "$TEST_DIR/Empty Dir"

# Readable, contains 'x' (Should match)
echo "hello x world" > "$TEST_DIR/Dir A/file one.txt" 
# Readable, contains 'y' (Should match)
echo "this has y" > "$TEST_DIR/Dir A/file two.log"   
# Readable, contains NO target chars (Should NOT match)
echo "clean content" > "$TEST_DIR/Dir A/clean file.txt"

# Readable, contains 'x' and spaces in filename (Should match)
echo "contains x" > "$TEST_DIR/Dir A/SubDir B/another file with spaces.txt"
# NOT readable, contains 'x' (Should NOT match because of permissions)
echo "secret x" > "$TEST_DIR/Dir A/SubDir B/unreadable.txt"
chmod 000 "$TEST_DIR/Dir A/SubDir B/unreadable.txt"

# Readable, contains NO target chars (Should NOT match)
echo "nothing here" > "$TEST_DIR/Dir C/empty_match.txt"

# Expected output
# Search for characters 'x' and 'y'
# Expected Results:
# - 'test_hierarchy'has 0 valid files (-)
# - 'test_hierarchy/Dir A' has 2 valid files (+)
# - 'test_hierarchy/Dir A/SubDir B' has 1 valid file (+) [unreadable.txt is ignored]
# - 'test_hierarchy/Dir C' has 0 valid files (-)
# - 'test_hierarchy/Empty Dir' has 0 valid files (-)
# - Total count: 3

cat << 'EOF' | sort > "$EXPECTED_OUTPUT"
- ./test_hierarchy
+ ./test_hierarchy/Dir A
+ ./test_hierarchy/Dir A/SubDir B
- ./test_hierarchy/Dir C
- ./test_hierarchy/Empty Dir
Total files found: 3
EOF

# Validate

# Execute script with G="test_hierarchy", C1="x", C2="y"
# We sort outputs (excluding the final total) because directory traversal order can vary (e.g., find vs * expansion)
"$SOLUTION_SCRIPT" "$TEST_DIR" "x" "y" |sort > "$ACTUAL_OUTPUT"

if diff "$EXPECTED_OUTPUT" "$ACTUAL_OUTPUT"; then
    echo -e "\n\033[0;32m[PASS]\033[0m Script passed validation successfully!"
    exit 0
else
    echo -e "\n\033[0;31m[FAIL]\033[0m Script output did not match expected layout."
    exit 1
fi

chmod 644 "$TEST_DIR/Dir A/SubDir B/unreadable.txt"
#rm -rf "$TEST_DIR" "$EXPECTED_OUTPUT" "$ACTUAL_OUTPUT"

