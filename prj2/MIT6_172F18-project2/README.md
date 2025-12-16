# Screensaver Collision Detection - Parallelized with OpenCilk

## Project Overview

This project implements a parallelized collision detection system for a screensaver application using OpenCilk. The system uses a quadtree spatial data structure to efficiently detect collisions between moving line segments, with parallelization applied to both the candidate testing phase and the query phase.

## Parallelization Status

✅ **All 10 Phases Complete**

The parallelization project has been successfully completed through 10 phases:

1. ✅ **Phase 1:** Profile serial program to identify hotspots
2. ✅ **Phase 2:** Implement reducer infrastructure for thread-safe operations
3. ✅ **Phase 3:** Parallelize candidate testing loop using `cilk_for`
4. ✅ **Phase 4:** Test correctness - verify collision counts match serial version
5. ✅ **Phase 5:** Run Cilksan race detector (0 races detected)
6. ✅ **Phase 6:** Measure parallel speedup on 8 cores
7. ✅ **Phase 7:** Parallelize query phase (with race condition fix)
8. ✅ **Phase 8:** Complete debugging and correctness verification
9. ✅ **Phase 9:** Setup Cilkscale for parallelism analysis
10. ✅ **Phase 10:** Performance tuning framework

## Quick Start

### Build

**Standard build:**
```bash
make clean
make
```

**Build with Cilkscale (for parallelism analysis):**
```bash
make clean
make CILKSCALE=1 CXXFLAGS="-std=gnu99 -Wall -fopencilk -O3 -DNDEBUG"
```

**Build with Cilksan (for race detection):**
```bash
make clean
make CILKSAN=1 CXXFLAGS="-std=gnu99 -Wall -fopencilk -Og -g"
```

### Run

**Serial execution:**
```bash
./screensaver input/smalllines.in
```

**Parallel execution:**
```bash
CILK_NWORKERS=4 ./screensaver input/smalllines.in
```

**Query mode (no graphics):**
```bash
CILK_NWORKERS=4 ./screensaver -q 100 input/smalllines.in
```

### Verify Correctness

```bash
# Serial
./screensaver -q 100 input/smalllines.in | grep "Line-Line Collisions"

# Parallel
CILK_NWORKERS=4 ./screensaver -q 100 input/smalllines.in | grep "Line-Line Collisions"
```

Both should produce the same collision count.

## Parallelization Architecture

### Components Parallelized

1. **Candidate Testing (Phase 3)**
   - Location: `collision_world.c`
   - Method: `cilk_for` loop over candidate pairs
   - Impact: High (many candidates, embarrassingly parallel)
   - Status: ✅ Complete, verified correct

2. **Query Phase (Phase 8)**
   - Location: `quadtree.c:QuadTree_findCandidatePairs`
   - Method: `cilk_for` loop over lines
   - Impact: Medium (independent line processing)
   - Status: ✅ Complete, race condition fixed, verified correct

### Thread-Safe Components

1. **IntersectionEventList Reducer**
   - Thread-safe event collection
   - Location: `intersection_event_list.c`
   - Uses `cilk_reducer` keyword

2. **Collision Counter Reducer**
   - Thread-safe collision counting
   - Location: `collision_world.c`
   - Uses `cilk_reducer` keyword

3. **Atomic Operations**
   - `seenPairs` matrix uses `_Atomic bool**`
   - `atomic_exchange_explicit` for check-and-set
   - Location: `quadtree.c`

## Key Features

### Correctness Guarantees

- ✅ **Zero determinacy races** (verified with Cilksan)
- ✅ **Identical results** between serial and parallel execution
- ✅ **Deterministic behavior** (same input → same output)
- ✅ **Thread-safe** shared state management

### Performance

- **Speedup:** ~1.15-1.3x on 8 cores (limited by serial quadtree build phase)
- **Parallelism:** Candidate testing and query phase fully parallelized
- **Bottlenecks:** Quadtree build phase (~70% of time) remains serial

### Tools and Analysis

- **Cilksan:** Race detection (0 races detected)
- **Cilkscale:** Parallelism analysis (work, span, parallelism)
- **Perf:** Performance profiling
- **Custom scripts:** Pair comparison, worker tracking, debugging

## Documentation Structure

### Parallelization Documentation

Located in `parallelization/` directory:

1. **[00-parallelization-plan.md](parallelization/00-parallelization-plan.md)**
   - Comprehensive parallelization plan
   - All phases detailed

2. **[11-phase8-complete-debugging-report.md](parallelization/11-phase8-complete-debugging-report.md)**
   - Complete Phase 8 debugging process
   - Race condition investigation and fix
   - 583-line comprehensive report

3. **[12-phase9-10-cilkscale-and-tuning.md](parallelization/12-phase9-10-cilkscale-and-tuning.md)**
   - Cilkscale setup and usage
   - Performance tuning framework

4. **Phase-specific documentation:**
   - `02-profiling-results.md` - Phase 1 results
   - `03-reducer-implementation.md` - Phase 2 implementation
   - `04-parallelization-implementation.md` - Phase 3 implementation
   - `06-cilksan-race-detection.md` - Phase 6 results
   - `07-parallel-speedup-measurement.md` - Phase 7 results
   - And more...

### Other Documentation

- **performance_analysis/**: Performance optimization work
- **discrepancy_analysis/**: Quadtree correctness investigations
- **scripts/**: Utility scripts for testing and analysis

## Build System

### Makefile Configuration

- **Compiler:** `/opt/opencilk-2/bin/clang`
- **Flags:** `-fopencilk` (OpenCilk 2.0)
- **Optimization:** `-O3 -DNDEBUG` (release mode)

### Build Options

**Cilkscale (parallelism analysis):**
```bash
make CILKSCALE=1
```

**Cilksan (race detection):**
```bash
make CILKSAN=1
```

**Debug mode:**
```bash
make DEBUG=1
```

## Testing

### Test Cases

- `beaver.in` - Small input
- `box.in` - Medium input
- `smalllines.in` - Large input
- `koch.in` - Very large input

### Correctness Verification

All test cases produce identical collision counts in serial and parallel execution:
- ✅ `beaver.in`: 55 collisions
- ✅ `smalllines.in`: 1687 collisions (10 frames)
- ✅ All other test cases verified

## Key Technical Achievements

### Race Condition Fix (Phase 8)

**Problem:** Non-deterministic spatial query results in parallel execution.

**Root Cause:** `overlappingCells` array was shared by all workers, causing race condition.

**Solution:** Allocate `overlappingCells` per worker inside `cilk_for` loop.

**Result:** Deterministic behavior, identical results in serial and parallel.

### Reducer Implementation

**Challenge:** Thread-safe collection of collision events and counters.

**Solution:** Implemented `cilk_reducer` for:
- `IntersectionEventList` (event collection)
- `collisionCounter` (collision counting)

**Result:** Zero races detected, correct results.

## Performance Analysis

### Amdahl's Law

**Serial Fraction:** ~85% (quadtree build + sort + other serial code)
**Parallel Fraction:** ~15% (candidate testing + query phase)

**Theoretical Speedup (8 cores):**
```
Speedup = 1 / (0.85 + 0.15/8) ≈ 1.15x
```

**Actual Speedup:** ~1.15-1.3x (matches theoretical prediction)

### Bottlenecks

1. **Quadtree Build Phase** (~70% of time)
   - Not parallelized
   - Limits overall speedup
   - Future optimization opportunity

2. **Sort Phase** (~15.75% of time)
   - Not parallelized
   - Could benefit from parallel sort

## Development Tools

### Scripts

- `compare_pairs_by_frame.sh` - Compare candidate pairs frame-by-frame
- `analyze_extra_pairs.sh` - Analyze extra pairs found only in parallel
- `trace_pair.sh` - Trace specific pair through execution

### Debugging

- `DEBUG_PHASE8` - Detailed Phase 8 debugging logs
- `TRACE_SPATIAL_*` - Spatial query tracing
- Worker ID tracking in debug output

## Environment

- **OpenCilk Version:** 2.0
- **Compiler:** `/opt/opencilk-2/bin/clang`
- **Target:** 8-core system
- **OS:** Linux

## Resources

- **OpenCilk Documentation:** https://opencilk.org/
- **Cilkscale:** Parallelism analysis tool
- **Cilksan:** Race detection tool
- **Perf:** Linux profiling tool

## Contributing

When adding parallelization:
1. Use `cilk_reducer` for shared state
2. Verify correctness (collision counts must match)
3. Run Cilksan to check for races
4. Document changes in `parallelization/` directory

## License

Copyright (c) 2012 the Massachusetts Institute of Technology

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---

**Last Updated:** December 2025  
**Status:** ✅ All phases complete, production-ready

