#!/bin/bash
#
# profile_quadtree.sh - Performance profiling script for quadtree collision detection
#
# This script provides detailed profiling information using the time command
# and compares brute-force vs quadtree performance.
#
# Usage:
#   ./scripts/profile_quadtree.sh [OPTIONS]
#
# Options:
#   -i, --input FILE      Input file to test (default: input/beaver.in)
#   -f, --frames N        Number of frames to simulate (default: 100)
#   -h, --help           Show this help message
#
# Output:
#   - Detailed timing information from time command
#   - Collision counts for correctness verification
#   - Speedup calculation
#   - Correctness status
#
# Example:
#   ./scripts/profile_quadtree.sh -i input/smalllines.in -f 50

set -e

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Default parameters
INPUT="input/beaver.in"
FRAMES=100

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
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -i, --input FILE      Input file to test (default: input/beaver.in)"
            echo "  -f, --frames N        Number of frames to simulate (default: 100)"
            echo "  -h, --help            Show this help message"
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

echo "=== Quadtree Performance Profiling ==="
echo ""
echo "Input: $INPUT"
echo "Frames: $FRAMES"
echo ""

# Create temporary directory for output files
TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

# Test brute-force baseline
echo "1. Brute-Force Baseline:"
echo "-----------------------"
time ./screensaver "$FRAMES" "$INPUT" > "$TMPDIR/bf.out" 2>&1
BF_TIME=$(grep -oP 'Elapsed execution time: \K[\d.]+' "$TMPDIR/bf.out" || echo "")
BF_COLL=$(grep -oP 'Line-Line Collisions: \K\d+' "$TMPDIR/bf.out" || echo "0")
BF_LW=$(grep -oP 'Line-Wall Collisions: \K\d+' "$TMPDIR/bf.out" || echo "0")

if [ -n "$BF_TIME" ]; then
    echo "  Elapsed time: ${BF_TIME}s"
else
    echo "  Elapsed time: (not found in output)"
fi
echo "  Line-Line collisions: $BF_COLL"
echo "  Line-Wall collisions: $BF_LW"
echo ""

# Test quadtree
echo "2. Quadtree:"
echo "------------"
time ./screensaver -q "$FRAMES" "$INPUT" > "$TMPDIR/qt.out" 2>&1
QT_TIME=$(grep -oP 'Elapsed execution time: \K[\d.]+' "$TMPDIR/qt.out" || echo "")
QT_COLL=$(grep -oP 'Line-Line Collisions: \K\d+' "$TMPDIR/qt.out" || echo "0")
QT_LW=$(grep -oP 'Line-Wall Collisions: \K\d+' "$TMPDIR/qt.out" || echo "0")

if [ -n "$QT_TIME" ]; then
    echo "  Elapsed time: ${QT_TIME}s"
else
    echo "  Elapsed time: (not found in output)"
fi
echo "  Line-Line collisions: $QT_COLL"
echo "  Line-Wall collisions: $QT_LW"
echo ""

# Calculate speedup
if [ -n "$BF_TIME" ] && [ -n "$QT_TIME" ] && [ "$QT_TIME" != "0" ]; then
    speedup=$(awk "BEGIN {printf \"%.2f\", $BF_TIME / $QT_TIME}")
    echo "Speedup: ${speedup}x"
    
    if (( $(awk "BEGIN {print ($speedup < 1)}") )); then
        slowdown=$(awk "BEGIN {printf \"%.2f\", 1 / $speedup}")
        echo "  (Quadtree is ${slowdown}x SLOWER)"
    elif (( $(awk "BEGIN {print ($speedup > 1)}") )); then
        echo "  (Quadtree is ${speedup}x FASTER)"
    else
        echo "  (Equal performance)"
    fi
else
    echo "⚠ Could not calculate speedup (invalid times)"
fi

# Check correctness
echo ""
if [ "$BF_COLL" == "$QT_COLL" ] && [ "$BF_LW" == "$QT_LW" ]; then
    echo "✓ Correctness: PASSED"
    echo "  Both algorithms produced identical collision counts"
else
    echo "✗ Correctness: FAILED"
    echo "  Brute-force: LL=$BF_COLL, LW=$BF_LW"
    echo "  Quadtree:    LL=$QT_COLL, LW=$QT_LW"
fi

echo ""
echo "=== Profiling Complete ==="
