# Performance Testing Scripts

This directory contains scripts for testing and benchmarking the collision detection algorithms (brute-force vs quadtree).

## Scripts Overview

### 1. `benchmark.sh` - Comprehensive Benchmark

Runs brute-force and quadtree algorithms on all input files and compares their performance and correctness.

**Usage:**
```bash
./scripts/benchmark.sh [OPTIONS]
```

**Options:**
- `-f, --frames N` - Number of frames to simulate (default: 100)
- `-i, --input FILE` - Test specific input file (default: all .in files)
- `-o, --output FILE` - Output file for results (default: benchmark_results.txt)
- `-h, --help` - Show help message

**Example:**
```bash
# Test all inputs with 50 frames
./scripts/benchmark.sh -f 50

# Test specific input
./scripts/benchmark.sh -i input/smalllines.in -f 100

# Save to custom output file
./scripts/benchmark.sh -o my_results.txt
```

**Output:**
- Prints results to stdout with speedup and correctness status
- Saves detailed results table to output file
- Reports Line-Line and Line-Wall collision counts
- Calculates speedup (quadtree time / brute-force time)

**Output Format:**
```
Testing: smalllines.in
  Lines: 530
----------------------------------------
  Brute-force: Time: 0.4263s, LL: 6137, LW: 0
  Quadtree:    Time: 0.1942s, LL: 6137, LW: 0
  ✓ Correctness: PASSED (collision counts match)
  Speedup: 2.20x (quadtree is faster)
```

---

### 2. `performance_test.sh` - Statistical Performance Test

Runs multiple test iterations with warmup to provide statistically reliable performance measurements.

**Usage:**
```bash
./scripts/performance_test.sh [OPTIONS]
```

**Options:**
- `-f, --frames N` - Number of frames per test (default: 50)
- `-w, --warmup N` - Number of warmup runs (default: 2)
- `-r, --runs N` - Number of test runs for averaging (default: 3)
- `-i, --input FILE` - Test specific input file (default: smalllines, beaver, box)
- `-o, --output FILE` - Output file for results (default: performance_results.txt)
- `-h, --help` - Show help message

**Example:**
```bash
# Default test (3 runs with 2 warmup)
./scripts/performance_test.sh

# More accurate test (5 runs, 3 warmup)
./scripts/performance_test.sh -r 5 -w 3

# Test specific input with more frames
./scripts/performance_test.sh -i input/beaver.in -f 100
```

**Output:**
- Prints detailed progress to stdout
- Saves formatted results table to output file
- Provides complexity analysis (O(n²) vs O(n log n) expectations)
- Reports average times across multiple runs

**Output Format:**
```
Testing: input/smalllines.in
  Lines: 530
  Running brute-force...
  Running quadtree...
  BF Time: 0.4263 s, QT Time: 0.1942 s, Speedup: 2.20
  BF Collisions: 6137, QT Collisions: 6137 ✓
```

**Results File Format:**
```
Performance Test Results
Generated: [timestamp]

Test Configuration:
  Frames per test: 50
  Warmup runs: 2
  Test runs: 3

Results:
========================================
Input                Lines      BF Time (s)     QT Time (s)     Speedup         BF Collisions  
----------------------------------------
smalllines.in        530        0.4263          0.1942          2.20            6137           
beaver.in            294        0.2181          0.0795          2.74            256            
box.in               401        0.2581          0.1093          2.36            0              
```

---

### 3. `profile_quadtree.sh` - Detailed Profiling

Provides detailed profiling information using the `time` command for a single input file.

**Usage:**
```bash
./scripts/profile_quadtree.sh [OPTIONS]
```

**Options:**
- `-i, --input FILE` - Input file to test (default: input/beaver.in)
- `-f, --frames N` - Number of frames to simulate (default: 100)
- `-h, --help` - Show help message

**Example:**
```bash
# Default test
./scripts/profile_quadtree.sh

# Test different input
./scripts/profile_quadtree.sh -i input/smalllines.in -f 50
```

**Output:**
- Detailed timing information from `time` command (real, user, sys)
- Collision counts for both algorithms
- Speedup calculation
- Correctness verification

**Output Format:**
```
=== Quadtree Performance Profiling ===

Input: input/beaver.in
Frames: 100

1. Brute-Force Baseline:
-----------------------
real    0m0.218s
user    0m0.215s
sys     0m0.003s
  Elapsed time: 0.218s
  Line-Line collisions: 256
  Line-Wall collisions: 0

2. Quadtree:
------------
real    0m0.080s
user    0m0.078s
sys     0m0.002s
  Elapsed time: 0.080s
  Line-Line collisions: 256
  Line-Wall collisions: 0

Speedup: 2.73x
  (Quadtree is 2.73x FASTER)

✓ Correctness: PASSED
  Both algorithms produced identical collision counts
```

---

### 4. `test_cutoff.sh` - Optimize maxLinesPerNode Threshold

Tests different `maxLinesPerNode` values to find the optimal threshold for quadtree subdivision.

**⚠️ WARNING:** This script modifies `quadtree.c`. Make sure you have committed your changes or have a backup before running.

**Usage:**
```bash
./scripts/test_cutoff.sh [OPTIONS]
```

**Options:**
- `-i, --input FILE` - Input file to test (default: input/smalllines.in)
- `-f, --frames N` - Number of frames per test (default: 10)
- `-c, --cutoffs LIST` - Comma-separated list of cutoff values (default: 16,24,32,40,48,64)
- `-r, --restore N` - Restore maxLinesPerNode to this value after testing (default: 32)
- `-h, --help` - Show help message

**Example:**
```bash
# Default test
./scripts/test_cutoff.sh

# Test with different cutoffs
./scripts/test_cutoff.sh -c 8,16,32,64

# Test different input
./scripts/test_cutoff.sh -i input/beaver.in -f 50
```

**Output:**
- Performance results for each cutoff value
- Collision counts for correctness verification
- Summary showing best performing cutoff
- Automatically restores original value after testing

**Output Format:**
```
=== Testing Different maxLinesPerNode Values ===
Input: input/smalllines.in
Frames: 10
Cutoff values: 16 24 32 40 48 64

Testing maxLinesPerNode = 16
  Building... OK
  Time: 0.205s
  Collisions: 6137

Testing maxLinesPerNode = 24
  Building... OK
  Time: 0.198s
  Collisions: 6137

...

=== Summary ===
Cutoff          Time (s)        Collisions     
=============================================
16              0.205           6137
24              0.198           6137
32              0.194           6137
...

Best performance: maxLinesPerNode = 32 (0.194s)
```

---

## Common Usage Patterns

### Quick Performance Check
```bash
# Fast benchmark on all inputs
./scripts/benchmark.sh -f 50
```

### Detailed Performance Analysis
```bash
# Statistical test with multiple runs
./scripts/performance_test.sh -f 100 -r 5 -w 3
```

### Profiling Specific Input
```bash
# Detailed profiling for problematic input
./scripts/profile_quadtree.sh -i input/smalllines.in -f 100
```

### Finding Optimal Configuration
```bash
# Test different cutoff values
./scripts/test_cutoff.sh -i input/smalllines.in -f 50 -c 16,24,32,40,48,64
```

---

## Prerequisites

1. **Build the project first:**
   ```bash
   make clean
   make
   ```

2. **Input files:** Ensure input files exist in `input/` directory

3. **Dependencies:**
   - `bash` (version 4.0+)
   - `awk` (for floating-point arithmetic)
   - `grep` (with Perl regex support: `-oP`)
   - `time` command (for profiling script)
   - `timeout` command (for test_cutoff.sh)

---

## Output Files

- `benchmark_results.txt` - Results from benchmark.sh
- `performance_results.txt` - Results from performance_test.sh
- `quadtree.c.backup.*` - Backup files created by test_cutoff.sh

---

## Interpreting Results

### Speedup Values
- **Speedup > 1**: Quadtree is faster (e.g., 2.5x means quadtree is 2.5x faster)
- **Speedup < 1**: Quadtree is slower (e.g., 0.5x means quadtree is 2x slower)
- **Speedup = 1**: Equal performance

### Correctness
- **PASSED**: Collision counts match exactly between algorithms
- **FAILED**: Collision counts differ (indicates correctness bug)

### Expected Patterns (O(n²) vs O(n log n))
- Speedup should **increase** as n (number of lines) increases
- For small n, speedup may be < 1 (overhead dominates)
- For large n, speedup should be >> 1 (quadtree benefits)

---

## Troubleshooting

### Script fails with "screensaver binary not found"
- Make sure you're running from the project root directory
- Build the project: `make clean && make`

### Script fails with "Input file not found"
- Check that input files exist in `input/` directory
- Use absolute or relative paths correctly

### test_cutoff.sh modifies quadtree.c unexpectedly
- The script creates a backup before modifying
- Check `quadtree.c.backup.*` files to restore
- The script should restore the original value, but verify manually

### Performance results seem inconsistent
- Use `performance_test.sh` with more runs (`-r 5` or higher)
- Increase warmup runs (`-w 3` or higher)
- Ensure system is not under heavy load

---

## Notes

- All scripts use `awk` for floating-point arithmetic (portable, no `bc` dependency)
- Scripts automatically detect project root and work from any directory
- All scripts include error handling and helpful error messages
- Results are saved to files for later analysis

