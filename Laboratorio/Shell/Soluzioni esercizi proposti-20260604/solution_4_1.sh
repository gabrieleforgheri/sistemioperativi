#!/bin/bash

# Get nice value of PID
get_niceness() {
    local pid="$1"
    ps -p "$pid" -o nice= | tr -d '[:space:]'
}

# Sanity check
if [ -z "$1" ]; then
    echo "Usage: $0 <process_name>"
    echo "Example: $0 firefox"
    exit 1
fi
PROCESS_NAME="$1"
# Find PIDs using ps and grep
pids=$(ps -axco pid,command | grep -i "$PROCESS_NAME" | grep -o '^[[:space:]]*[0-8]*')
# easier option: pids=$(pgrep -i "$PROCESS_NAME")

# If no PIDs are found, exit
if [ -z "$pids" ]; then
    echo "No running processes found matching '$PROCESS_NAME'."
    exit 0
fi

# Loop through each found PID
for pid in $pids; do
    # Verify the PID is a valid number and still exists
    if ! ps -p "$pid" > /dev/null 2>&1; then
        continue
    fi
    current_nice=$(get_niceness "$pid")
    echo "Found PID: $pid | Current Nice Value: $current_nice"

    # Prompt user for priority adjustment
    echo "Select an action for PID $pid:"
    echo "1) High (-10)"
    echo "2) Low (10)"
    echo "3) No Change"
    read -p "Enter choice [1-3]: " choice

    case "$choice" in
        1|"[Hh]igh")
            new_nice="-10"
            ;;
        2|"[Ll]ow")
            new_nice="10"
            ;;
        3|"[Nn]o [Cc]hange"|*)
            continue
            ;;
    esac
    # Change the priority
    echo "Attempting to change niceness of PID $pid to $new_nice..."
    renice -n "$new_nice" -p "$pid" > /dev/null 2>&1
    # Check execution status
    if [ $? -eq 0 ]; then
        echo "[SUCCESS] Priority successfully changed for PID $pid."
    else
        echo "[FAILED] Could not change priority for PID $pid."
    fi
done
