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
- **ECL** (Embeddable Common Lisp)

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

```bash
python3 python/constraint_checker.py 4:80 10:42 64:9 --plot plots/result.png
```

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

## Integration with Existing Scripts

The tool can be integrated with existing plot generation scripts in `hw5/scripts/`:

```python
from constraint_checker import ConstraintChecker

checker = ConstraintChecker()
result = checker.check_measurements([(4, 80), (10, 42), (64, 9)])

if result['status'] == 'inconsistent':
    print(f"Found contradiction: {result['contradiction']}")
    # Generate detailed plot
    checker.generate_plot([(4, 80), (10, 42), (64, 9)], 
                         Path('plots/contradiction.png'))
```

## Future Enhancements

- [ ] Web interface for interactive checking
- [ ] Batch processing of multiple measurement sets
- [ ] Export results to LaTeX/Markdown
- [ ] Integration with Jupyter notebooks
- [ ] Parallelism bounds calculation (from write-up 2)
- [ ] Automatic plot generation with optimal axis ranges

## License

Part of MIT 6.172 Performance Engineering Lab coursework.

