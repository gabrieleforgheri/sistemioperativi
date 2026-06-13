#!/bin/bash

# Paths to CPU frequency files
BASE_PATH="/sys/devices/system/cpu/cpu0/cpufreq"
CUR_FREQ_FILE="$BASE_PATH/scaling_cur_freq"
MAX_FREQ_FILE="$BASE_PATH/cpuinfo_max_freq"
MIN_FREQ_FILE="$BASE_PATH/cpuinfo_min_freq"
if [[ ! -f "$CUR_FREQ_FILE" || ! -f "$MAX_FREQ_FILE" || ! -f "$MIN_FREQ_FILE" ]]; then
    echo "Error: Required CPU frequency files not found."
    exit 1
fi
# Read limits (values are in kHz, not Hz, in the Linux sysfs)
MAX_FREQ=$(cat "$MAX_FREQ_FILE")
MIN_FREQ=$(cat "$MIN_FREQ_FILE")
khz_to_ghz() {
    local val
    val=$(echo "scale=3; $1 / 1000000" | bc)
    # LC_NUMERIC=C forces printf to accept and output '.' as the decimal point
    LC_NUMERIC=C printf "%.3f GHz" "$val"
}
# Function to read current frequency
get_cpu_freq() {
    cat "$CUR_FREQ_FILE"
}

# Initialize the old frequency tracker
OLD_FREQ=$(get_cpu_freq)

echo "Min Freq: $(khz_to_ghz "$MIN_FREQ") | Max Freq: $(khz_to_ghz "$MAX_FREQ")"

while true; do
    CUR_FREQ=$(get_cpu_freq)
    
    # Convert to GHz for printing
    GHZ_STR=$(khz_to_ghz "$CUR_FREQ")    
    # Instead of decimals, we check if: abs(CUR - LIMIT) <= (LIMIT / 100)
    MAX_THRESHOLD=$(( MAX_FREQ / 100 ))
    MIN_THRESHOLD=$(( MIN_FREQ / 100 ))
    
    STATUS_LIMIT=""
    if (( (MAX_FREQ - CUR_FREQ) <= MAX_THRESHOLD )); then
        STATUS_LIMIT="[NEAR MAX (1%)]"
    elif (( (CUR_FREQ - MIN_FREQ) <= MIN_THRESHOLD )); then
        STATUS_LIMIT="[NEAR MIN (1%)]"
    fi

    # Evaluate Trend (increasing, decreasing, or stable within 10%)
    # 10% of the old frequency:
    STABLE_THRESHOLD=$(( OLD_FREQ / 10 ))
    # Calculate the absolute difference
    DIFF=$(( CUR_FREQ - OLD_FREQ ))
    if (( CUR_FREQ <= OLD_FREQ + STABLE_THRESHOLD && CUR_FREQ >= OLD_FREQ - STABLE_THRESHOLD )) ; then
        TREND="Stable"
    elif (( CUR_FREQ > OLD_FREQ )); then
        TREND="Increasing"
    else
        TREND="Decreasing"
    fi

    # Print results and prepare for next titeration
    printf "Current: %s %-16s | Trend: %s\n" "$GHZ_STR" "$STATUS_LIMIT" "$TREND"
    OLD_FREQ=$CUR_FREQ
    sleep 1
done
