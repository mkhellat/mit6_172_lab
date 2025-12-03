#!/bin/bash
#
# debug_discrepancy.sh - Debug script to investigate collision count
# discrepancies between brute-force and quadtree
#
# This script runs both algorithms and compares:
# 1. Which pairs each algorithm tests
# 2. Which collisions each algorithm finds
# 3. Identifies extra/missing collisions

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

INPUT="${1:-input/box.in}"
FRAMES="${2:-100}"

echo "=== Investigating Collision Discrepancy ==="
echo "Input: $INPUT"
echo "Frames: $FRAMES"
echo ""

# Check if binary exists
if [ ! -f "./screensaver" ]; then
    echo "Error: screensaver binary not found. Please build first."
    exit 1
fi

# Create temporary directory
TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

echo "Step 1: Running brute-force..."
./screensaver "$FRAMES" "$INPUT" > "$TMPDIR/bf.out" 2>&1
BF_COLL=$(grep -oP 'Line-Line Collisions: \K\d+' "$TMPDIR/bf.out" || echo "0")
echo "  BF collisions: $BF_COLL"

echo ""
echo "Step 2: Running quadtree..."
./screensaver -q "$FRAMES" "$INPUT" > "$TMPDIR/qt.out" 2>&1
QT_COLL=$(grep -oP 'Line-Line Collisions: \K\d+' "$TMPDIR/qt.out" || echo "0")
echo "  QT collisions: $QT_COLL"

echo ""
echo "Step 3: Difference analysis..."
DIFF=$((QT_COLL - BF_COLL))
if [ "$DIFF" -gt 0 ]; then
    echo "  Quadtree has $DIFF MORE collisions than brute-force"
elif [ "$DIFF" -lt 0 ]; then
    echo "  Quadtree has $((DIFF * -1)) FEWER collisions than brute-force"
else
    echo "  ✓ Perfect match!"
    exit 0
fi

echo ""
echo "=== Next Steps ==="
echo "To investigate further, we need to:"
echo "1. Add debug code to log all pairs tested by each algorithm"
echo "2. Add debug code to log all collisions found by each algorithm"
echo "3. Compare the sets to find differences"
echo ""
echo "This requires modifying collision_world.c to add detailed logging."

