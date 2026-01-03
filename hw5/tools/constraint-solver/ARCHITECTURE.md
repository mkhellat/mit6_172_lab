# Architecture Overview

## Design Philosophy

This tool uses a **hybrid LISP + Python architecture** to leverage the strengths
of both languages:

- **LISP**: Symbolic computation, constraint solving, mathematical elegance
- **Python**: Plotting, CLI interface, integration with existing tools

## Component Overview

```
┌─────────────────────────────────────────────────────────┐
│                    User Interface                         │
│  (Python CLI: constraint_checker.py)                      │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│              Python Interface Layer                     │
│  - Parses user input                                     │
│  - Formats for LISP                                      │
│  - Calls LISP solver                                     │
│  - Parses results                                        │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│            LISP Constraint Solver                        │
│  (constraint-solver.lisp)                               │
│                                                          │
│  - Derives constraints from measurements                │
│  - Checks for contradictions                            │
│  - Performs pairwise compatibility analysis             │
│  - Returns structured results                            │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│              Plot Generation                            │
│  (Python + Matplotlib)                                  │
│                                                          │
│  - Generates constraint visualization plots              │
│  - Shows feasible regions                                │
│  - Highlights contradictions                            │
└─────────────────────────────────────────────────────────┘
```

## Data Flow

1. **Input**: User provides measurements as `P:T` pairs
   - Example: `4:80 10:42 64:9`

2. **Python Processing**:
   - Parses input: `[(4, 80), (10, 42), (64, 9)]`
   - Formats for LISP: `'((4 80) (10 42) (64 9))`

3. **LISP Processing**:
   - Python generates temporary LISP script with:
     - `(load "constraint-solver.lisp")` to load the solver
     - `(check-measurements ...)` to run the analysis
     - `(format-result-readable ...)` and `(format-result-json ...)` for output
     - `(ext:quit)` for ECL to exit (prevents hanging in REPL)
   - LISP derives constraints for each measurement
   - Finds tightest span constraint
   - Evaluates greedy bounds
   - Checks for contradictions
   - Performs pairwise analysis
   - Returns structured results

4. **Result Formatting**:
   - LISP returns structured data (human-readable + JSON)
   - Python extracts JSON from output (handles ECL banner)
   - Python formats for display (human-readable or JSON)
   - Optionally generates plots

**Note on ECL**: ECL doesn't automatically exit after loading a script - it
stays in the REPL. The Python interface automatically adds `(ext:quit)` to the
generated script to ensure ECL exits properly. This prevents the test script
from hanging indefinitely.

## Constraint Derivation

For each measurement (P, TP):

1. **Work Law**: T₁ ≤ P × TP
   ```lisp
   (derive-work-law 4 80)  => 320
   ```

2. **Span Law**: T∞ ≤ TP
   ```lisp
   (derive-span-law 80)  => 80
   ```

3. **Greedy Scheduler Bound**: T₁ ≥ P × TP - (P-1) × T∞
   ```lisp
   (derive-greedy-bound 4 80)  => (lambda (t-inf) (- 320 (* 3 t-inf)))
   ```

## Contradiction Detection Algorithm

```lisp
1. Derive constraints from all measurements
2. Find tightest span constraint: min(T∞_max)
3. Evaluate greedy bounds at that T∞
4. Find feasible region: [max(greedy_bounds), min(work_laws)]
5. Check if feasible: max(greedy_bounds) <= min(work_laws)
6. If not feasible → CONTRADICTION
```

## Pairwise Compatibility

For each pair of measurements:
1. Derive constraints for the pair
2. Find tightest span constraint
3. Check if feasible region exists
4. Mark as compatible/incompatible

## Extension Points

The architecture supports easy extension:

1. **Additional Constraints**: Add new constraint types in LISP
2. **Plot Types**: Add new visualization functions in Python
3. **Output Formats**: Add formatters in Python
4. **Web Interface**: Add Flask/FastAPI layer on top of Python interface

## Future Enhancements

- [ ] Web interface (Flask/FastAPI)
- [ ] Jupyter notebook integration
- [ ] Batch processing
- [ ] Export to LaTeX/Markdown
- [ ] Parallelism bounds calculation
- [ ] Interactive plots (Plotly)

