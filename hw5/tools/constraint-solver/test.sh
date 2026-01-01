#!/bin/bash
# Test script for constraint solver

set -e

echo "=== Testing Constraint Solver ==="
echo

# Test 1: Ben Bitdiddle's measurements (should be inconsistent)
echo "Test 1: Ben Bitdiddle's measurements (4:80, 10:42, 64:9)"
echo "Expected: INCONSISTENT (339 <= T1 <= 320)"
python3 python/constraint_checker.py 4:80 10:42 64:9
echo

# Test 2: Professor Karan's measurements (should be inconsistent)
echo "Test 2: Professor Karan's measurements (4:80, 10:42, 64:10)"
echo "Expected: INCONSISTENT (330 <= T1 <= 320)"
python3 python/constraint_checker.py 4:80 10:42 64:10
echo

# Test 3: Consistent measurements (from write-up 2)
echo "Test 3: Consistent measurements (4:100, 64:10)"
echo "Expected: CONSISTENT"
python3 python/constraint_checker.py 4:100 64:10
echo

echo "=== All tests completed ==="

