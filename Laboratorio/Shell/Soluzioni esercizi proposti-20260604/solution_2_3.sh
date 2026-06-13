#!/bin/bash
# Input variables
BIRTH_YEAR=1995
MONTHLY_BUDGET=450
# Dynamically get the current year
CURRENT_YEAR=$(date +%Y)
# Approximate age
AGE=$((CURRENT_YEAR - BIRTH_YEAR))
# Daily budget. Use bc for decimal precision
DAILY_BUDGET=$(echo "scale=2; $MONTHLY_BUDGET / 30" | bc)
# Output the results
echo "Current Year:    $CURRENT_YEAR"
echo "Approximate Age: $AGE years old"
echo "Daily Budget:    \$$DAILY_BUDGET"
