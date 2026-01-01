# Quick Start Guide

## Installation

1. **Install Common Lisp** (if not already installed):
   ```bash
   # Arch Linux
   sudo pacman -S ecl
   
   # Ubuntu/Debian
   sudo apt-get install ecl
   ```

2. **Install Python dependencies**:
   ```bash
   pip install matplotlib numpy
   ```

## Quick Test

Run the test script:
```bash
cd hw5/tools/constraint-solver
./test.sh
```

## Basic Usage

Check if measurements are consistent:
```bash
python3 python/constraint_checker.py 4:80 10:42 64:9
```

This will tell you if Ben Bitdiddle's measurements are consistent (they're not!).

## What It Does

The tool checks if parallel execution time measurements satisfy:
- **Work Law**: T₁ ≤ P × TP
- **Span Law**: T∞ ≤ TP  
- **Greedy Scheduler Bound**: TP ≤ (T₁ - T∞)/P + T∞

If all three can't be satisfied simultaneously, the measurements are inconsistent.

## Example Output

```
=== Measurement Consistency Check ===
Status: inconsistent
Contradiction: 339 <= T1 <= 320 is impossible

Pairwise Compatibility:
  ((4 80) (10 42)): COMPATIBLE
  ((4 80) (64 9)): COMPATIBLE
  ((10 42) (64 9)): COMPATIBLE
```

This shows that while each pair is compatible, all three together create a contradiction.

## Troubleshooting

**"No Common Lisp implementation found"**
- Install ECL: `sudo pacman -S ecl` or `sudo apt-get install ecl`

**"LISP solver failed"**
- Check that `lisp/constraint-solver.lisp` exists
- Try running ECL directly: `ecl -load lisp/constraint-solver.lisp`

**Import errors in Python**
- Make sure you're running from the `constraint-solver` directory
- Check that matplotlib and numpy are installed: `pip install matplotlib numpy`

