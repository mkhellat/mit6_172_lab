#!/bin/bash
#
# performance_test.sh - Comprehensive performance test to verify O(n^2) vs O(n log n) complexity
#
# This script tests brute-force vs quadtree with multiple runs and warmup,
# providing statistical analysis of performance differences.
#
# Usage:
#   ./scripts/performance_test.sh [OPTIONS]
#
# Options:
#   -f, --frames N        Number of frames per test (default: 50)
#   -w, --warmup N        Number of warmup runs (default: 2)
#   -r, --runs N          Number of test runs for averaging (default: 3)
#   -i, --input FILE      Test specific input file (default: smalllines, beaver, box)
#   -o, --output FILE     Output file for results (default: performance_results.txt)
#   -h, --help           Show this help message
#
# Output:
#   - Prints detailed progress to stdout
#   - Saves formatted results table to output file
#   - Provides complexity analysis
#
# Example:
#   ./scripts/performance_test.sh -f 100 -r 5 -i input/smalllines.in

set -e

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Default parameters
FRAMES=50
WARMUP_RUNS=2
TEST_RUNS=3
RESULTS_FILE="performance_results.txt"
SPECIFIC_INPUT=""

# Default input files to test
DEFAULT_INPUT_FILES=(
    "input/smalllines.in"
    "input/beaver.in"
    "input/box.in"
)

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -f|--frames)
            FRAMES="$2"
            shift 2
            ;;
        -w|--warmup)
            WARMUP_RUNS="$2"
            shift 2
            ;;
        -r|--runs)
            TEST_RUNS="$2"
            shift 2
            ;;
        -i|--input)
            SPECIFIC_INPUT="$2"
            shift 2
            ;;
        -o|--output)
            RESULTS_FILE="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -f, --frames N        Number of frames per test (default: 50)"
            echo "  -w, --warmup N        Number of warmup runs (default: 2)"
            echo "  -r, --runs N          Number of test runs for averaging (default: 3)"
            echo "  -i, --input FILE      Test specific input file"
            echo "  -o, --output FILE     Output file for results (default: performance_results.txt)"
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

echo "=========================================="
echo "Performance Test: Brute-Force vs Quadtree"
echo "=========================================="
echo ""
echo "Testing complexity: O(n^2) vs O(n log n)"
echo ""

# Initialize results file
rm -f "$RESULTS_FILE"
echo "Performance Test Results" > "$RESULTS_FILE"
echo "Generated: $(date)" >> "$RESULTS_FILE"
echo "" >> "$RESULTS_FILE"
echo "Test Configuration:" >> "$RESULTS_FILE"
echo "  Frames per test: $FRAMES" >> "$RESULTS_FILE"
echo "  Warmup runs: $WARMUP_RUNS" >> "$RESULTS_FILE"
echo "  Test runs: $TEST_RUNS" >> "$RESULTS_FILE"
echo "" >> "$RESULTS_FILE"
echo "Results:" >> "$RESULTS_FILE"
echo "$(printf '=%.0s' {1..70})" >> "$RESULTS_FILE"
# Table format: fits within 80 characters
# Input(18) Lines(9) BF(s)(9) QT(s)(9) Spdup(6) BF-Coll(10) = 61 chars
printf "%-18s %-9s %-9s %-9s %-6s %-10s\n" \
    "Input" "Lines" "BF(s)" "QT(s)" "Spdup" "BF-Coll" >> "$RESULTS_FILE"
printf "%.0s-" {1..70} >> "$RESULTS_FILE"
echo "" >> "$RESULTS_FILE"

# Function to get number of lines from input file
get_line_count() {
    local file="$1"
    if [ -f "$file" ]; then
        wc -l < "$file" | tr -d ' '
    else
        echo "0"
    fi
}

# Function to run and time a test with warmup and averaging
run_test() {
    local mode="$1"  # "bf" or "qt"
    local input_file="$2"
    local frames="$3"
    
    local cmd="./screensaver"
    if [ "$mode" = "qt" ]; then
        cmd="./screensaver -q"
    fi
    
    # Warmup runs
    for ((i=0; i<WARMUP_RUNS; i++)); do
        $cmd "$frames" "$input_file" > /dev/null 2>&1
    done
    
    # Actual timing runs
    local total_time=0
    for ((i=0; i<TEST_RUNS; i++)); do
        local start=$(date +%s.%N)
        $cmd "$frames" "$input_file" > /dev/null 2>&1
        local end=$(date +%s.%N)
        local elapsed=$(awk "BEGIN {print $end - $start}")
        total_time=$(awk "BEGIN {print $total_time + $elapsed}")
    done
    
    # Average time
    awk "BEGIN {printf \"%.6f\", $total_time / $TEST_RUNS}"
}

# Function to get collision count
get_collisions() {
    local mode="$1"
    local input_file="$2"
    local frames="$3"
    
    local cmd="./screensaver"
    if [ "$mode" = "qt" ]; then
        cmd="./screensaver -q"
    fi
    
    $cmd "$frames" "$input_file" 2>&1 | grep "Line-Line" | grep -oP '\d+' | head -1 || echo "0"
}

# Determine which input files to test
if [ -n "$SPECIFIC_INPUT" ]; then
    INPUT_FILES=("$SPECIFIC_INPUT")
else
    INPUT_FILES=("${DEFAULT_INPUT_FILES[@]}")
fi

# Test each input file
for input_file in "${INPUT_FILES[@]}"; do
    if [ ! -f "$input_file" ]; then
        echo "Skipping $input_file (not found)"
        continue
    fi
    
    echo "Testing: $input_file"
    num_lines=$(get_line_count "$input_file")
    
    echo "  Lines: $num_lines"
    echo "  Running brute-force..."
    bf_time=$(run_test "bf" "$input_file" "$FRAMES")
    bf_collisions=$(get_collisions "bf" "$input_file" "$FRAMES")
    
    echo "  Running quadtree..."
    qt_time=$(run_test "qt" "$input_file" "$FRAMES")
    qt_collisions=$(get_collisions "qt" "$input_file" "$FRAMES")
    
    # Calculate speedup
    speedup="N/A"
    if (( $(awk "BEGIN {print ($qt_time > 0)}") )); then
        speedup=$(awk "BEGIN {printf \"%.2f\", $bf_time / $qt_time}")
    fi
    
    # Print results
    printf "  BF Time: %.4f s, QT Time: %.4f s, Speedup: %s\n" \
        "$bf_time" "$qt_time" "$speedup"
    printf "  BF Collisions: %s, QT Collisions: %s" \
        "$bf_collisions" "$qt_collisions"
    
    # Check correctness
    if [ "$bf_collisions" == "$qt_collisions" ]; then
        echo " ✓"
    else
        echo " ✗ (MISMATCH!)"
    fi
    
    # Save to results file (compact format for 80-char width)
    printf "%-18s %-6s %9.4f %9.4f %7s %9s\n" \
        "$(basename $input_file)" "$num_lines" "$bf_time" "$qt_time" "$speedup" "$bf_collisions" >> "$RESULTS_FILE"
    
    echo ""
done

echo "========================================"
echo "Results saved to: $RESULTS_FILE"
echo ""
echo "Analysis:"
echo "  Expected pattern for O(n^2) vs O(n log n):"
echo "    - Speedup should INCREASE as n increases"
echo "    - For small n, speedup may be < 1 (overhead)"
echo "    - For large n, speedup should be >> 1"
echo ""
cat "$RESULTS_FILE"
