#!/bin/bash


#!/bin/bash

# Configuration
SCRIPT="$1"
DAYS_LIMIT=7

SRC=src_dir
DST=dst_dir

# Text colors for grading output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

log_success() { echo -e "${GREEN}[PASS] $1${NC}"; }
log_failure() { echo -e "${RED}[FAIL] $1${NC}"; }

# Test setup
setup_basic_environment() {
    echo "Creating Basic Test Environment..."
    rm -rf ${SRC} ${DST}
    mkdir -p ${SRC}/dir1 ${SRC}/dir2 ${DST}
    # Standard unique files
    echo "Unique content 1" > ${SRC}/file1.txt
    echo "Unique content 2" > ${SRC}/dir1/file2.txt
    # Duplicate content files (F1 and F2 match, F3 and F4 match)
    echo "Duplicate 1" > ${SRC}/f1.txt
    echo "Duplicate 1" > ${SRC}/dir1/f2.txt
    echo "Duplicate 2"  > ${SRC}/dir2/f3.txt
    echo "Duplicate 2"  > ${SRC}/f4.txt
    # Symlinks (Should be skipped)
    ln -s ../file1.txt ${SRC}/dir1/broken_sym.txt
    # Old File
    touch -d "10 days ago" ${SRC}/old_file
}

setup_evil_environment() {
    echo "Creating Evil Test Environment (Spaces & Permissions)..."
    rm -rf ${SRC} ${DST}
    mkdir -p "${SRC}/spaced dir" "${SRC}/secret_dir" ${DST}
    # Files with spaces
    echo "Spaced content 1" > "${SRC}/spaced dir/spaced file1.txt"
    echo "Spaced content 1" > "${SRC}/spaced file2.txt" # Duplicate of file1
    # Unreadable file (Should be skipped)
    echo "Can't read this" > ${SRC}/unreadable.txt
    chmod 000 ${SRC}/unreadable.txt
    # Inaccessible directory (Should be skipped)
    echo "Hidden content" > ${SRC}/secret_dir/hidden.txt
    chmod 000 ${SRC}/secret_dir
    # many many files
    #(cd ${SRC} && seq 1 150000 | xargs -n 10000 touch)
}

cleanup() {
    # Fix permissions so rm doesn't complain during cleanup
    chmod 700 ${SRC}/secret_dir 2>/dev/null || true
    chmod 600 ${SRC}/unreadable.txt 2>/dev/null || true
    rm -rf ${SRC} ${DST} ${SRC} ${DST} files.txt duplicates.txt
}

# Validation

validate() {
    local src_dir=$1
    local dst_dir=$2
    local env_type=$3 # "basic" or "trick"

    echo "Running Verifications for [${env_type^^}] Environment"

    # Check if files.txt exists and was created
    if [ -f "files.txt" ]; then
        log_success "files.txt generated successfully."
    else
        log_failure "files.txt is missing!"
        return
    fi

    # Check for Symlinks and Unreadable files in files.txt
    if grep -q -E "broken_sym|unreadable|secret_dir" files.txt; then
        log_failure "files.txt contains files that should have been skipped (symlinks/unreadable)!"
    else
        log_success "Symlinks, unreadable files, and restricted directories successfully skipped."
    fi

    # Check duplicates.txt format verification
    if [ -f "duplicates.txt" ]; then
        log_success "duplicates.txt generated successfully."
        
        if [ "$env_type" == "basic" ]; then
            # Verify the exact structural map specified in your prompt
            # Expecting f2.txt to reference f1.txt, or vice-versa depending on finding order
            if grep -q "f2.txt:.*f1.txt" duplicates.txt || grep -q "f1.txt:.*f2.txt" duplicates.txt; then
                log_success "Duplicate tracking structure matches specifications for group 1."
            else
                log_failure "Group 1 duplicates not mapped correctly in duplicates.txt."
            fi
        fi
    else
        log_failure "duplicates.txt is missing!"
    fi

    # Check target directory structure integrity (No symlinks copied)
    if [ -d "$dst_dir" ]; then
        if [[ $(find "$dst_dir" -type l | wc -l) -eq 0 ]]; then
            log_success "Target backup contains zero symlinks."
        else
            log_failure "Target backup contains copied symlinks! Failure."
        fi
    else
        log_failure "Target backup directory does not exist!"
    fi

    # Hard link Deduplication Check (Inodes evaluation)
    if [ "$env_type" == "basic" ]; then
        # Find paths of duplicated entries in target backup
        local backed_f1=$(find "$dst_dir" -name "f1.txt")
        local backed_f2=$(find "$dst_dir" -name "f2.txt")

        if [ -n "$backed_f1" ] && [ -n "$backed_f2" ]; then
            local inode_f1=$(ls -i "$backed_f1" | awk '{print $1}')
            local inode_f2=$(ls -i "$backed_f2" | awk '{print $1}')

            if [ "$inode_f1" -eq "$inode_f2" ]; then
                log_success "Deduplication Active! Target files share the same inode ($inode_f1)."
            else
                log_failure "Deduplication Failed! Duplicate files have distinct inodes."
            fi
        else
            log_failure "Expected backup files are missing from target."
        fi
    fi

    # Space-in-name structural check
    if [ "$env_type" == "trick" ]; then
        local backed_space1=$(find "$dst_dir" -name "spaced file1.txt")
        local backed_space2=$(find "$dst_dir" -name "spaced file2.txt")

        if [ -n "$backed_space1" ] && [ -n "$backed_space2" ]; then
            log_success "Spaces in filenames managed cleanly without word-splitting bugs."
            
            local inode_s1=$(ls -i "$backed_space1" | awk '{print $1}')
            local inode_s2=$(ls -i "$backed_space2" | awk '{print $1}')
            if [ "$inode_s1" -eq "$inode_s2" ]; then
                log_success "Deduplication successfully applied to paths containing whitespace entries."
            else
                log_failure "Deduplication failed for files with spaces."
            fi
        else
            log_failure "Files with spaces were lost or split incorrectly during backup execution."
        fi
    fi
}

# Test

# Ensure the student script is executable
chmod +x "$SCRIPT" 2>/dev/null

## TEST 1: Basic Environment 
setup_basic_environment
echo "Invoking script on Basic environment..."
$SCRIPT ${SRC} ${DST} $DAYS_LIMIT
validate ${SRC} ${DST} "basic"
cleanup

## TEST 2: Evil Environment (Spaces, Symlinks, Permissions)
(
    setup_evil_environment
    # set low stack size to reduce ARG_MAX
    ulimit -s 256
    echo "Setting ARG_MAX to: " $(getconf ARG_MAX)
    echo "Invoking script on evil environment..."
    $SCRIPT ${SRC} ${DST} $DAYS_LIMIT
    validate ${SRC} ${DST} "trick"
)

# Clean up environments
#cleanup
echo "Testing Complete."

exit

# Takes too long for validation.
# evil_dir
mkdir -p evil_dir && cd evil_dir
# create many files
seq 1 150000 | xargs -n 10000 touch
# create a file with space in its name
touch "file with a space in its name"

# force a low ARG_MAX
(
    pwd
    ulimit -s 256
    getconf ARG_MAX
    ls * | sort -n | tail -n1 # Too many argumants
    for i in $(ls); 
    do 
        echo "$i";
        if [[ ! -f $i ]]; then
           echo "file \"$i\" not found" >&2
        fi
    done | sort -n | tail -n1 
)
