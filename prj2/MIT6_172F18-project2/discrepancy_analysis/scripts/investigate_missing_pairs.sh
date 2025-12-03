#!/bin/bash
# Script to investigate why QT is missing pairs in frame 89
# Specifically checks if QT tested the 9 missing pairs and why they weren't found

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
DEBUG_LOGS_DIR="$PROJECT_ROOT/discrepancy_analysis/debug_logs"

if [ ! -d "$DEBUG_LOGS_DIR" ]; then
    echo "Error: debug_logs directory not found: $DEBUG_LOGS_DIR"
    exit 1
fi

echo "==================================================================="
echo "INVESTIGATING MISSING PAIRS IN FRAME 89"
echo "==================================================================="
echo ""

# The 9 missing pairs in frame 89
MISSING_PAIRS=("(41,105)" "(45,105)" "(49,105)" "(53,105)" "(57,105)" "(61,105)" "(65,105)" "(69,105)" "(93,105)")

echo "1. CHECKING IF QT TESTED THESE PAIRS:"
echo "--------------------------------------"
for pair in "${MISSING_PAIRS[@]}"; do
    count=$(grep -c "Frame 89: $pair" "$DEBUG_LOGS_DIR/debug_qt_pairs.txt" 2>/dev/null || echo "0")
    if [ "$count" -eq 0 ]; then
        echo "  ❌ $pair: NOT TESTED by QT"
    else
        echo "  ✓  $pair: Tested $count time(s) by QT"
        grep "Frame 89: $pair" "$DEBUG_LOGS_DIR/debug_qt_pairs.txt" | head -1
    fi
done
echo ""

echo "2. CHECKING IF BF TESTED THESE PAIRS:"
echo "--------------------------------------"
for pair in "${MISSING_PAIRS[@]}"; do
    count=$(grep -c "Frame 89: $pair" "$DEBUG_LOGS_DIR/debug_bf_pairs.txt" 2>/dev/null || echo "0")
    if [ "$count" -eq 0 ]; then
        echo "  ❌ $pair: NOT TESTED by BF"
    else
        result=$(grep "Frame 89: $pair" "$DEBUG_LOGS_DIR/debug_bf_pairs.txt" | head -1 | awk '{print $4}')
        echo "  ✓  $pair: Tested by BF, result=$result"
    fi
done
echo ""

echo "3. CHECKING LINE IDS IN TREE->LINES ARRAY ORDER:"
echo "------------------------------------------------"
echo "  (This would require code modification to log array indices)"
echo "  Hypothesis: If line 105 is at index < line 41, then when processing"
echo "  line1=line41, we'd skip line2=line105 due to array index check."
echo ""

echo "4. CHECKING FALSE POSITIVE (105,137):"
echo "---------------------------------------"
echo "  BF result:"
grep "Frame 89: (105,137)" "$DEBUG_LOGS_DIR/debug_bf_pairs.txt" 2>/dev/null | head -1
echo "  QT result:"
grep "Frame 89: (105,137)" "$DEBUG_LOGS_DIR/debug_qt_pairs.txt" 2>/dev/null | head -1
echo ""

echo "5. SUMMARY:"
echo "----------"
echo "  Missing pairs: 9"
echo "  False positives: 1 (105,137)"
echo "  Root cause: Quadtree spatial query not finding pairs in same cells"
echo "  OR: Array index filtering incorrectly excluding pairs"
echo ""

