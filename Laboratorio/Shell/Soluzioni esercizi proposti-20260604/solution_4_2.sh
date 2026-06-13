#!/bin/bash

# Draw the rectangle boundary
draw_rect() {
    local x1=$1 y1=$2 x2=$3 y2=$4
    
    # Draw top border
    tput cup $y1 $x1
    for ((x=x1; x<=x2; x++)); do printf "+"; done

    # Draw bottom border
    tput cup $y2 $x1
    for ((x=x1; x<=x2; x++)); do printf "+"; done

    # Draw left and right vertical borders
    for ((y=y1+1; y<y2; y++)); do
        tput cup $y $x1; printf "+"
        tput cup $y $x2; printf "+"
    done
    
    # Hide the cursor and move it out of the way
    tput civis
    tput cup 0 0
}

# Calculate coordinates for a centered, half-sized rectangle
rect() {
    clear
    
    # Terminal size
    local lines=$(tput lines)
    local cols=$(tput cols)
    # rect size
    local rect_height=$((lines / 2))
    local rect_width=$((cols / 2))
    # Calculate rect corners
    local x1=$(( (cols - rect_width) / 2 ))
    local y1=$(( (lines - rect_height) / 2 ))
    local x2=$(( x1 + rect_width ))
    local y2=$(( y1 + rect_height ))
    # Draw
    draw_rect $x1 $y1 $x2 $y2
}

# Signal Cleanup Handler
cleanup() {
    tput cnorm # Restore the cursor visibility on exit
    clear
    exit 0
}

# Trap Signals
# Intercept SIGWINCH (Window Changed) to recalculate and redraw on resize
trap rect SIGWINCH
# Intercept Ctrl+C (SIGINT) and exit (SIGTERM) to clean up terminal state safely
trap cleanup SIGINT SIGTERM

# Main Logic
# Call rect once to draw it initially
rect
# Wait forever
while true; do
    sleep 1
done
