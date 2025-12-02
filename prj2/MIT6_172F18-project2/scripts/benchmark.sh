#!/bin/bash
#
# benchmark.sh - Comprehensive benchmark script for collision detection performance
#
# This script runs brute-force and quadtree algorithms on all input files
# and compares their performance and correctness.
#
# Usage:
#   ./scripts/benchmark.sh [OPTIONS]
#
# Options:
#   -f, --frames N        Number of frames to simulate (default: 100)
#   -i, --input FILE      Test specific input file (default: all .in files)
#   -o, --output FILE     Output file for results (default: benchmark_results.txt)
#   -h, --help           Show this help message
#
# Output:
#   - Prints results to stdout
#   - Saves detailed results to output file
#   - Reports speedup and correctness for each input
#
# Example:
#   ./scripts/benchmark.sh -f 50 -i input/smalllines.in

set -e

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Default parameters
FRAMES=100
INPUT_PATTERN="input/*.in"
OUTPUT_FILE="benchmark_results.txt"
SPECIFIC_INPUT=""

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -f|--frames)
            FRAMES="$2"
            shift 2
            ;;
        -i|--input)
            SPECIFIC_INPUT="$2"
            shift 2
            ;;
        -o|--output)
            OUTPUT_FILE="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -f, --frames N        Number of frames to simulate (default: 100)"
            echo "  -i, --input FILE      Test specific input file (default: all .in files)"
            echo "  -o, --output FILE     Output file for results (default: benchmark_results.txt)"
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

# Initialize output file
rm -f "$OUTPUT_FILE"
echo "Collision Detection Performance Benchmark" > "$OUTPUT_FILE"
echo "Generated: $(date)" >> "$OUTPUT_FILE"
echo "Frames per test: $FRAMES" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"
# Table format: fits within 80 characters
# Input(18) Lines(6) BF(s)(8) QT(s)(8) Spdup(6) BF-LL(8) QT-LL(8) OK(4) = 66 chars
printf "%-18s %-6s %-8s %-8s %-6s %-8s %-8s %-4s\n" \
    "Input" "Lines" "BF(s)" "QT(s)" "Spdup" "BF-LL" "QT-LL" "OK" >> "$OUTPUT_FILE"
echo "$(printf '=%.0s' {1..66})" >> "$OUTPUT_FILE"

echo "=== Collision Detection Performance Benchmark ==="
echo "Frames per test: $FRAMES"
echo ""

# Function to extract collision counts from output
extract_collisions() {
    local output="$1"
    local ll_coll=$(echo "$output" | grep -oP 'Line-Line Collisions: \K\d+' || echo "0")
    local lw_coll=$(echo "$output" | grep -oP 'Line-Wall Collisions: \K\d+' || echo "0")
    echo "$ll_coll $lw_coll"
}

# Function to run and time a test
run_test() {
    local algo="$1"  # "brute" or "quadtree"
    local input="$2"
    local frames="$3"
    
    local cmd="./screensaver"
    if [ "$algo" == "quadtree" ]; then
        cmd="./screensaver -q"
    fi
    
    # Run and capture output and time
    local start=$(date +%s.%N)
    local output=$($cmd "$frames" "$input" 2>&1)
    local end=$(date +%s.%N)
    local elapsed=$(awk "BEGIN {printf \"%.6f\", $end - $start}")
    
    # Extract collision counts
    local collisions=$(extract_collisions "$output")
    
    echo "$elapsed $collisions"
}

# Function to get line count from input file
get_line_count() {
    local file="$1"
    if [ -f "$file" ]; then
        wc -l < "$file" | tr -d ' '
    else
        echo "0"
    fi
}

# Determine which input files to test
if [ -n "$SPECIFIC_INPUT" ]; then
    INPUT_FILES=("$SPECIFIC_INPUT")
else
    INPUT_FILES=(input/*.in)
fi

# Test each input file
TOTAL_TESTS=0
PASSED_TESTS=0

for input_file in "${INPUT_FILES[@]}"; do
    if [ ! -f "$input_file" ]; then
        echo "Skipping $input_file (not found)"
        continue
    fi
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    input_name=$(basename "$input_file")
    line_count=$(get_line_count "$input_file")
    
    echo "Testing: $input_name"
    echo "  Lines: $line_count"
    echo "----------------------------------------"
    
    # Run brute-force
    echo -n "  Brute-force: "
    bf_result=$(run_test "brute" "$input_file" "$FRAMES")
    bf_time=$(echo $bf_result | cut -d' ' -f1)
    bf_ll=$(echo $bf_result | cut -d' ' -f2)
    bf_lw=$(echo $bf_result | cut -d' ' -f3)
    printf "Time: %.4fs, LL: %s, LW: %s\n" "$bf_time" "$bf_ll" "$bf_lw"
    
    # Run quadtree
    echo -n "  Quadtree:    "
    qt_result=$(run_test "quadtree" "$input_file" "$FRAMES")
    qt_time=$(echo $qt_result | cut -d' ' -f1)
    qt_ll=$(echo $qt_result | cut -d' ' -f2)
    qt_lw=$(echo $qt_result | cut -d' ' -f3)
    printf "Time: %.4fs, LL: %s, LW: %s\n" "$qt_time" "$qt_ll" "$qt_lw"
    
    # Calculate speedup
    speedup="N/A"
    status="UNKNOWN"
    if [ -n "$bf_time" ] && [ -n "$qt_time" ] && [ "$qt_time" != "0" ]; then
        speedup=$(awk "BEGIN {printf \"%.2f\", $bf_time / $qt_time}")
        
        # Check correctness
        if [ "$bf_ll" == "$qt_ll" ] && [ "$bf_lw" == "$qt_lw" ]; then
            status="PASS"
            PASSED_TESTS=$((PASSED_TESTS + 1))
            echo "  ✓ Correctness: PASSED (collision counts match)"
        else
            status="FAIL"
            echo "  ✗ Correctness: FAILED (collision counts differ!)"
            echo "    BF: LL=$bf_ll, LW=$bf_lw"
            echo "    QT: LL=$qt_ll, LW=$qt_lw"
        fi
        
        # Interpret speedup
        if (( $(awk "BEGIN {print ($speedup > 1)}") )); then
            echo "  Speedup: ${speedup}x (quadtree is faster)"
        elif (( $(awk "BEGIN {print ($speedup < 1)}") )); then
            slowdown=$(awk "BEGIN {printf \"%.2f\", 1 / $speedup}")
            echo "  Speedup: ${speedup}x (quadtree is ${slowdown}x slower)"
        else
            echo "  Speedup: ${speedup}x (equal performance)"
        fi
    else
        echo "  ⚠ Could not calculate speedup (invalid times)"
    fi
    
    # Save to output file (compact format for 80-char width)
    ok_mark="✓"
    if [ "$status" != "PASS" ]; then
        ok_mark="✗"
    fi
    printf "%-18s %-6s %8.4f %8.4f %6s %8s %8s %4s\n" \
        "$input_name" "$line_count" "$bf_time" "$qt_time" "$speedup" \
        "$bf_ll" "$qt_ll" "$ok_mark" >> "$OUTPUT_FILE"
    
    echo ""
done

# Summary
echo "=== Benchmark Summary ==="
echo "Total tests: $TOTAL_TESTS"
echo "Passed: $PASSED_TESTS"
echo "Failed: $((TOTAL_TESTS - PASSED_TESTS))"
echo ""
echo "Results saved to: $OUTPUT_FILE"
echo "=== Benchmark Complete ==="
