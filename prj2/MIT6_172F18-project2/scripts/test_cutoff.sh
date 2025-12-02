#!/bin/bash
#
# test_cutoff.sh - Test different maxLinesPerNode values to find optimal cutoff
#
# This script tests various maxLinesPerNode values to determine the optimal
# threshold for quadtree subdivision. It modifies quadtree.c, rebuilds, and
# tests performance for each value.
#
# Usage:
#   ./scripts/test_cutoff.sh [OPTIONS]
#
# Options:
#   -i, --input FILE      Input file to test (default: input/smalllines.in)
#   -f, --frames N        Number of frames per test (default: 10)
#   -c, --cutoffs LIST    Comma-separated list of cutoff values (default: 16,24,32,40,48,64)
#   -r, --restore N       Restore maxLinesPerNode to this value after testing (default: 32)
#   -h, --help           Show this help message
#
# Output:
#   - Performance results for each cutoff value
#   - Collision counts for correctness verification
#   - Summary of best performing cutoff
#
# WARNING: This script modifies quadtree.c. Make sure you have committed
#          your changes or have a backup before running.
#
# Example:
#   ./scripts/test_cutoff.sh -i input/beaver.in -f 50 -c 8,16,32,64

set -e

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Default parameters
INPUT="input/smalllines.in"
FRAMES=10
CUTOFFS="16,24,32,40,48,64"
RESTORE_VALUE=32

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -i|--input)
            INPUT="$2"
            shift 2
            ;;
        -f|--frames)
            FRAMES="$2"
            shift 2
            ;;
        -c|--cutoffs)
            CUTOFFS="$2"
            shift 2
            ;;
        -r|--restore)
            RESTORE_VALUE="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -i, --input FILE      Input file to test (default: input/smalllines.in)"
            echo "  -f, --frames N        Number of frames per test (default: 10)"
            echo "  -c, --cutoffs LIST    Comma-separated list of cutoff values (default: 16,24,32,40,48,64)"
            echo "  -r, --restore N       Restore maxLinesPerNode to this value after testing (default: 32)"
            echo "  -h, --help           Show this help message"
            echo ""
            echo "WARNING: This script modifies quadtree.c. Make sure you have committed"
            echo "         your changes or have a backup before running."
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

# Check if screensaver binary exists
if [ ! -f "./screensaver" ]; then
    echo "Error: screensaver binary not found. Please build the project first."
    exit 1
fi

# Check if input file exists
if [ ! -f "$INPUT" ]; then
    echo "Error: Input file '$INPUT' not found."
    exit 1
fi

# Check if quadtree.c exists
if [ ! -f "quadtree.c" ]; then
    echo "Error: quadtree.c not found."
    exit 1
fi

# Backup quadtree.c
BACKUP_FILE="quadtree.c.backup.$(date +%s)"
cp quadtree.c "$BACKUP_FILE"
echo "Backed up quadtree.c to $BACKUP_FILE"
echo ""

# Convert comma-separated list to array
IFS=',' read -ra CUTOFF_ARRAY <<< "$CUTOFFS"

echo "=== Testing Different maxLinesPerNode Values ==="
echo "Input: $INPUT"
echo "Frames: $FRAMES"
echo "Cutoff values: ${CUTOFF_ARRAY[*]}"
echo ""

# Store results
declare -a RESULTS
declare -a TIMES
declare -a COLLISIONS

# Test each cutoff value
for cutoff in "${CUTOFF_ARRAY[@]}"; do
    echo "Testing maxLinesPerNode = $cutoff"
    
    # Update the cutoff in quadtree.c
    # CRITICAL: Pattern must match only the assignment line with digits, not
    # the function parameter assignment. Use [0-9]\+ to require at least one
    # digit, and include semicolon in match to be more specific.
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS sed requires different syntax
        sed -i '' "s/config\.maxLinesPerNode = [0-9]\+;/config.maxLinesPerNode = $cutoff;/" quadtree.c
    else
        sed -i "s/config\.maxLinesPerNode = [0-9]\+;/config.maxLinesPerNode = $cutoff;/" quadtree.c
    fi
    
    # Rebuild
    echo -n "  Building... "
    if make clean > /dev/null 2>&1 && \
       make CXXFLAGS="-std=gnu99 -Wall -fopencilk -O3 -DNDEBUG" > /dev/null 2>&1; then
        echo "OK"
    else
        echo "FAILED"
        echo "  ⚠ Build failed, skipping this cutoff value"
        echo ""
        continue
    fi
    
    # Test
    echo -n "  Time: "
    start=$(date +%s.%N)
    if timeout 30 ./screensaver -q "$FRAMES" "$INPUT" > /dev/null 2>&1; then
        end=$(date +%s.%N)
        elapsed=$(awk "BEGIN {printf \"%.3f\", $end - $start}")
        echo "${elapsed}s"
        TIMES+=("$elapsed")
    else
        echo "TIMEOUT or ERROR"
        TIMES+=("999.999")
    fi
    
    # Check collisions for correctness
    collisions=$(timeout 30 ./screensaver -q "$FRAMES" "$INPUT" 2>&1 | \
                 grep "Line-Line" | head -1 | awk '{print $1}' || echo "0")
    echo "  Collisions: $collisions"
    COLLISIONS+=("$collisions")
    RESULTS+=("$cutoff:$elapsed:$collisions")
    
    echo ""
done

# Restore original value
echo "=== Restoring maxLinesPerNode = $RESTORE_VALUE ==="
# Use same pattern as above: match digits followed by semicolon
if [[ "$OSTYPE" == "darwin"* ]]; then
    sed -i '' "s/config\.maxLinesPerNode = [0-9]\+;/config.maxLinesPerNode = $RESTORE_VALUE;/" quadtree.c
else
    sed -i "s/config\.maxLinesPerNode = [0-9]\+;/config.maxLinesPerNode = $RESTORE_VALUE;/" quadtree.c
fi

# Rebuild with restored value
make clean > /dev/null 2>&1
make CXXFLAGS="-std=gnu99 -Wall -fopencilk -O3 -DNDEBUG" > /dev/null 2>&1

# Summary
echo ""
echo "=== Summary ==="
# Table format: fits within 80 characters
# Cutoff(10) Time(s)(12) Collisions(12) = 34 chars
printf "%-10s %-12s %-12s\n" "Cutoff" "Time(s)" "Collisions"
echo "$(printf '=%.0s' {1..34})"

best_time=999.999
best_cutoff=""
for result in "${RESULTS[@]}"; do
    IFS=':' read -r cutoff time collisions <<< "$result"
    printf "%-10s %12s %12s\n" "$cutoff" "$time" "$collisions"
    
    if (( $(awk "BEGIN {print ($time < $best_time)}") )); then
        best_time=$time
        best_cutoff=$cutoff
    fi
done

if [ -n "$best_cutoff" ]; then
    echo ""
    echo "Best performance: maxLinesPerNode = $best_cutoff (${best_time}s)"
fi

echo ""
echo "Note: Original quadtree.c backed up to $BACKUP_FILE"
echo "      Restored maxLinesPerNode to $RESTORE_VALUE"
