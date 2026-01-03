# Constraint Solver for Parallel Execution Time Measurements

A hybrid LISP + Python tool for checking consistency of parallel execution time
measurements using the Work Law, Span Law, and Greedy Scheduler Bound from
CLRS Chapter 27.

## Architecture

- **LISP Backend**: Symbolic constraint solving and mathematical analysis
- **Python Frontend**: Plot generation, CLI interface, and integration
- **Hybrid Approach**: Leverages LISP's strength in symbolic computation with
  Python's strength in visualization and tooling

## Directory Structure

```
constraint-solver/
├── lisp/
│   └── constraint-solver.lisp    # LISP constraint solver
├── python/
│   └── constraint_checker.py    # Python interface
├── plots/                        # Generated plots
└── README.md                     # This file
```

## Requirements

### LISP Implementation

You need one of the following Common Lisp implementations:
- **SBCL** (Steel Bank Common Lisp) - Recommended
- **CLISP** (GNU CLISP)
- **ECL** (Embeddable Common Lisp) - Supported with explicit exit

**Note**: ECL requires an explicit `(ext:quit)` command to exit after script
execution. The Python interface handles this automatically.

Install on Arch Linux:
```bash
sudo pacman -S sbcl  # or clisp, or ecl
```

Install on Ubuntu/Debian:
```bash
sudo apt-get install sbcl  # or clisp, or ecl
```

### Python Dependencies

```bash
pip install matplotlib numpy
```

## Usage

### Basic Usage (Python Interface)

```bash
cd hw5/tools/constraint-solver
python3 python/constraint_checker.py 4:80 10:42 64:9
```

### With Plot Generation

Generate constraint visualization plots for any set of measurements:

```bash
python3 python/constraint_checker.py 4:80 10:42 64:9 --plot plots/result.png
```

The tool automatically generates plots showing:
- Constraint lines (Work Law, Span Law, Greedy Scheduler Bound)
- Feasible regions for each measurement
- Contradictions (non-overlapping regions) when detected

Plots are saved to the specified path. See the [Plot Generation](#plot-generation)
section below for more details.

### JSON Output

```bash
python3 python/constraint_checker.py 4:80 10:42 64:9 --json
```

### Direct LISP Usage

```bash
sbcl --load lisp/constraint-solver.lisp
```

Then in the LISP REPL:
```lisp
(constraint-solver:check-measurements '((4 80) (10 42) (64 9)))
```

## Examples

### Example 1: Ben Bitdiddle's Measurements (Write-up 3)

```bash
python3 python/constraint_checker.py 4:80 10:42 64:9
```

**Expected Output:**
```
=== Measurement Consistency Check ===
Status: inconsistent
Contradiction: 339 <= T1 <= 320 is impossible

Pairwise Compatibility:
  ((4 80) (10 42)): COMPATIBLE
  ((4 80) (64 9)): COMPATIBLE
  ((10 42) (64 9)): COMPATIBLE
```

### Example 2: Professor Karan's Measurements (Write-up 4)

```bash
python3 python/constraint_checker.py 4:80 10:42 64:10
```

**Expected Output:**
```
=== Measurement Consistency Check ===
Status: inconsistent
Contradiction: 330 <= T1 <= 320 is impossible

Pairwise Compatibility:
  ((4 80) (10 42)): COMPATIBLE
  ((4 80) (64 10)): COMPATIBLE
  ((10 42) (64 10)): COMPATIBLE
```

### Example 3: Consistent Measurements

```bash
python3 python/constraint_checker.py 4:100 64:10
```

**Expected Output:**
```
=== Measurement Consistency Check ===
Status: consistent

Pairwise Compatibility:
  ((4 100) (64 10)): COMPATIBLE
```

## API Reference

### LISP Functions

#### `check-measurements measurements`
Main function to check measurement consistency.

**Parameters:**
- `measurements`: List of (processor-count time) pairs
  - Example: `'((4 80) (10 42) (64 9))`

**Returns:**
- List with `:overall` and `:pairwise` keys

#### `derive-constraints measurements`
Derive all constraints from measurements.

**Returns:**
- List of constraint dictionaries with:
  - `:processor-count`
  - `:time`
  - `:work-law` (T1 <= P * TP)
  - `:span-law` (T∞ <= TP)
  - `:greedy-bound` (function of T∞)

#### `check-pairwise-compatibility measurements`
Check compatibility of all pairs of measurements.

**Returns:**
- List of pairwise compatibility results

### Python API

#### `ConstraintChecker.check_measurements(measurements)`

**Parameters:**
- `measurements`: List of (processor_count, time) tuples
  - Example: `[(4, 80), (10, 42), (64, 9)]`

**Returns:**
- Dictionary with consistency check results

## How It Works

1. **Constraint Derivation**: For each measurement (P, TP):
   - Work Law: T₁ ≤ P × TP
   - Span Law: T∞ ≤ TP
   - Greedy Scheduler Bound: T₁ ≥ P × TP - (P-1) × T∞

2. **Contradiction Detection**:
   - Find tightest span constraint (minimum T∞)
   - Evaluate greedy bounds at that T∞
   - Check if feasible region exists (min-T₁ ≤ max-T₁)

3. **Pairwise Analysis**:
   - Check each pair of measurements independently
   - Determine if pairs are compatible

4. **Visualization**:
   - Generate plots showing feasible regions
   - Highlight contradictions

## Implementation Details

### LISP Implementation Compatibility

The tool supports multiple Common Lisp implementations with automatic detection:

- **SBCL**: Uses `--script` mode for non-interactive execution
- **CLISP**: Executes script file directly
- **ECL**: Uses `-load` mode with explicit `(ext:quit)` to prevent hanging

**ECL Behavior**: ECL doesn't automatically exit after loading a script - it
stays in the REPL waiting for input. The Python interface automatically adds
`(ext:quit)` to the generated script to ensure proper exit. This prevents the
test script from hanging indefinitely.

### Output Parsing

The Python interface extracts JSON from LISP output by:
1. Finding the JSON object (looking for `"status"` field pattern)
2. Matching braces to extract complete JSON object
3. Handling ECL banner output that appears after script execution
4. Falling back to readable format parsing if JSON extraction fails

## Plot Generation

The constraint solver tool includes **general-purpose plot generation** that works
for **any set of measurements**. Simply use the `--plot` option:

```bash
python3 python/constraint_checker.py 4:80 10:42 64:9 --plot plots/result.png
```

### Features

The automatically generated plots show:

- **Constraint Lines**:
  - **Work Law**: Vertical dashed lines (T₁ ≤ P × TP)
  - **Span Law**: Horizontal dotted lines (T∞ ≤ TP)
  - **Greedy Scheduler Bound**: Diagonal solid lines (T₁ ≥ P × TP - (P-1) × T∞)
  
- **Feasible Regions**: Shaded areas where all constraints are satisfied for each
  measurement (when T∞ ≤ tightest span constraint)

- **Contradictions**: Automatically detected and highlighted when measurements are
  inconsistent:
  - Red vertical lines marking the impossible T₁ range
  - Red annotation box explaining the contradiction

- **Automatic Formatting**:
  - Color-coded by measurement (each measurement gets a unique color)
  - Tightest span constraint marked with "(tightest)" label
  - Optimal axis ranges automatically determined
  - Publication-quality output (300 DPI)

### Examples

**Inconsistent measurements** (contradiction detected):
```bash
python3 python/constraint_checker.py 4:80 10:42 64:9 --plot plots/inconsistent.png
```
The plot will show non-overlapping feasible regions and highlight the contradiction.

**Consistent measurements**:
```bash
python3 python/constraint_checker.py 4:100 64:10 --plot plots/consistent.png
```
The plot will show overlapping feasible regions indicating consistency.

**Any number of measurements**:
```bash
python3 python/constraint_checker.py 2:50 4:30 8:20 16:15 --plot plots/multi.png
```
Works with 2, 3, 4, or more measurements.

### Plot Output

- **Format**: PNG (300 DPI, publication quality)
- **Size**: 12×8 inches (optimized for viewing and printing)
- **Location**: Specified by `--plot` argument (directory created if needed)

### Legacy Scripts

The dedicated scripts in `hw5/scripts/` are still available for reference:
- `generate_contradiction_plot.py` - Ben Bitdiddle's data
- `generate_pairwise_plots.py` - Pairwise compatibility for Ben's data
- `generate_contradiction_plot_karan.py` - Professor Karan's data
- `generate_pairwise_plots_karan.py` - Pairwise compatibility for Karan's data

These scripts are hardcoded for specific measurements. The `--plot` option in the
constraint checker works for **any measurements** and is the recommended approach.

## Integration with Existing Scripts

The constraint checker can be used to validate measurements before generating plots:

```python
from constraint_checker import ConstraintChecker

checker = ConstraintChecker()
result = checker.check_measurements([(4, 80), (10, 42), (64, 9)])

if result['status'] == 'inconsistent':
    print(f"Found contradiction: {result['contradiction']}")
    # Now generate detailed plot using existing scripts
    # python3 scripts/generate_contradiction_plot.py
```

## Future Enhancements

- [ ] **Pairwise Plot Generation**: Generate separate plots for each pair of
  measurements (like the existing pairwise scripts)
- [ ] Web interface for interactive checking
- [ ] Batch processing of multiple measurement sets
- [ ] Export results to LaTeX/Markdown
- [ ] Integration with Jupyter notebooks
- [ ] Parallelism bounds calculation (from write-up 2)
- [ ] Interactive plots (Plotly) for web interface

## License

Part of MIT 6.172 Performance Engineering Lab coursework.

