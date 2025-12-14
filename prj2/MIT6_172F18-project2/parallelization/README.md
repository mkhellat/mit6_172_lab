# Parallelization Documentation

This directory contains documentation and plans for parallelizing the screensaver collision detection code using OpenCilk.

## Documents

1. **[00-parallelization-plan.md](00-parallelization-plan.md)**
   - Comprehensive parallelization plan
   - All phases: profiling, reducers, parallelization, tuning
   - Detailed implementation steps
   - Expected results and targets

2. **[01-quick-start.md](01-quick-start.md)**
   - Quick start guide
   - Immediate next steps
   - Common issues and solutions
   - Testing checklist

## Current Status

✅ **Cilk is already configured in Makefile!**
- Compiler: `/opt/opencilk-2/bin/clang`
- Flags: `-fopencilk` already set
- Ready to start parallelization

## Quick Start

1. **Verify setup:**
   ```bash
   /opt/opencilk-2/bin/clang --version
   make clean && make
   ```

2. **Profile serial program:**
   ```bash
   perf record -g ./screensaver -q 100 input/koch.in
   perf report
   ```

3. **Implement reducers** (see plan Section 2.2)

4. **Add parallelization** (see plan Section 3)

5. **Test correctness:**
   ```bash
   ./screensaver -q 100 input/koch.in | grep "Line-Line Collisions"
   CILK_NWORKERS=8 ./screensaver -q 100 input/koch.in | grep "Line-Line Collisions"
   ```

## Key Parallelization Targets

1. **Candidate Testing** (High Priority)
   - Embarrassingly parallel
   - Easy to implement
   - High impact

2. **Query Phase** (Medium Priority)
   - Independent lines
   - Moderate complexity
   - Medium impact

3. **Build Phase** (Low Priority)
   - Complex recursive parallelization
   - High impact but difficult

## Implementation Checklist

- [ ] Profile serial program
- [ ] Implement `IntersectionEventList` reducer
- [ ] Implement collision counter reducer
- [ ] Parallelize candidate testing
- [ ] Test correctness
- [ ] Run Cilksan race detector
- [ ] Measure speedup
- [ ] Parallelize query phase (optional)
- [ ] Tune for 8 cores

## Resources

- **OpenCilk Documentation:** https://opencilk.org/
- **Cilkscale:** For parallelism analysis
- **Cilksan:** For race detection
- **Perf:** For profiling

## Notes

- All shared state must use reducers (no direct writes)
- Correctness is critical (collision counts must match)
- Start with easy parallelization, optimize later
- Test on multiple inputs to ensure generality

