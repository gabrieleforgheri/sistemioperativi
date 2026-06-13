#!/bin/bash

# Auditor function
audit_file() {
    file="$1"
    
    if [ ! -e "$file" ]; then
        echo "[SKIP] $file does not exist."
        return
    fi

    # Get octal permissions (e.g., 644)
    perms=$(stat -c "%a" "$file")
    world_perms="${perms: -1}"

    # Check if world digit is >= 4 (read permission)
    if [ "$world_perms" -ge 4 ]; then
        echo "[WARNING] $file is world-readable! (Perms: $perms)"
        chmod o-r "$file"
        echo "          -> Fixed: Removed world-read permission."
    else
        echo "[SECURE] $file is not world-readable. (Perms: $perms)"
    fi
}
# Export the function so xargs/subshells can see it
export -f audit_file

# Define the array based on input
if [ $# -gt 0 ]; then
    sensitive_files=("$@")
else
    sensitive_files=("/etc/passwd" "/etc/shadow" "/etc/ssh/sshd_config")
fi
# check files
printf "%s\0" "${sensitive_files[@]}" | xargs -0 -I {} bash -c 'audit_file "{}"'
