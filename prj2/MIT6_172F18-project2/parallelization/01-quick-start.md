# Quick Start Guide for Parallelization

## Current Status

✅ **Cilk is already configured in Makefile!**
- Compiler: `/opt/opencilk-2/bin/clang`
- Flags: `-fopencilk` already set
- Runtime: Automatically linked

## Immediate Next Steps

### Step 1: Verify Cilk Setup

```bash
cd prj2/MIT6_172F18-project2

# Check if Cilk compiler is available
/opt/opencilk-2/bin/clang --version

# Build with Cilk (should work as-is)
make clean
make

# Test basic Cilk functionality
echo 'int main() { cilk_spawn printf("Hello\n"); cilk_sync; return 0; }' > test.c
/opt/opencilk-2/bin/clang -fopencilk test.c -o test
./test
```

### Step 2: Profile Serial Program

```bash
# Profile with perf
perf record -g ./screensaver -q 100 input/koch.in
perf report

# Or use perf with specific events
perf record -e cycles,instructions,cache-misses ./screensaver -q 100 input/koch.in
perf report
```

**Look for:**
- Functions taking >10% of time
- Nested loops that can be parallelized
- Independent operations

### Step 3: Identify Parallelization Targets

**High Priority (Easy, High Impact):**
1. **Candidate Testing** - Test intersection for each candidate pair
   - Location: After candidate list is built
   - Parallelism: Embarrassingly parallel
   - Impact: High (many candidates)

2. **Brute-Force Outer Loop** - If using brute-force algorithm
   - Location: `collision_world.c:193` (outer loop)
   - Parallelism: Independent iterations
   - Impact: Medium (only if brute-force used)

**Medium Priority (Moderate Complexity):**
3. **Query Phase** - Find candidates for each line
   - Location: `quadtree.c:1021` (loop over lines)
   - Parallelism: Independent lines
   - Impact: Medium (5-6% of time)

**Low Priority (Complex):**
4. **Build Phase** - Quadtree construction
   - Location: `quadtree.c:625` (QuadTree_build)
   - Parallelism: Recursive tree construction
   - Impact: High (70% of time) but complex

### Step 4: Implement Reducers

**Critical:** Must convert shared state to reducers before parallelization!

**Files to modify:**
1. `intersection_event_list.h` - Add reducer type
2. `intersection_event_list.c` - Add merge/identity functions
3. `collision_world.c` - Use reducers instead of direct list access

**See:** `00-parallelization-plan.md` Section 2.2 for detailed implementation

### Step 5: Add Parallelization

**Start with easiest: Candidate Testing**

```c
// In collision_world.c, after candidate list is built
cilk_for (int i = 0; i < candidateList->count; i++) {
  CandidatePair* pair = &candidateList->pairs[i];
  if (intersect(pair->line1, pair->line2, timeStep)) {
    IntersectionEventList_appendNode(&REDUCER_VIEW(eventListReducer), ...);
  }
}
```

### Step 6: Test Correctness

```bash
# Compare collision counts
./screensaver -q 100 input/koch.in | grep "Line-Line Collisions"
CILK_NWORKERS=8 ./screensaver -q 100 input/koch.in | grep "Line-Line Collisions"

# Should match exactly!
```

### Step 7: Race Detection

```bash
# Build with Cilksan
make clean
make CXXFLAGS="-fopencilk -fsanitize=cilk -O3"

# Run with Cilksan
CILK_NWORKERS=8 ./screensaver -q 100 input/koch.in

# Check for race reports
```

### Step 8: Measure Performance

```bash
# Serial time
time ./screensaver -q 100 input/koch.in

# Parallel time (8 workers)
time CILK_NWORKERS=8 ./screensaver -q 100 input/koch.in

# Calculate speedup
```

## Recommended Implementation Order

1. ✅ **Verify Cilk setup** (5 min)
2. ✅ **Profile serial program** (15 min)
3. ✅ **Implement reducers** (1-2 hours)
4. ✅ **Parallelize candidate testing** (30 min)
5. ✅ **Test correctness** (15 min)
6. ✅ **Race detection** (30 min)
7. ✅ **Measure speedup** (15 min)
8. ✅ **Parallelize query phase** (1 hour)
9. ✅ **Tune and optimize** (1-2 hours)

**Total estimated time:** 5-7 hours for basic parallelization

## Common Issues

### Issue: "undefined reference to `cilk_spawn`"
**Solution:** Make sure `-fopencilk` flag is in CXXFLAGS

### Issue: "reducer not working"
**Solution:** Check reducer type declaration and merge/identity functions

### Issue: "Race detected by Cilksan"
**Solution:** All shared state must use reducers, no direct writes to shared variables

### Issue: "No speedup"
**Solution:** 
- Check if parallel section is actually being executed
- Verify CILK_NWORKERS is set
- Profile to see if parallel section is bottleneck

## Testing Checklist

- [ ] Serial and parallel produce same collision counts
- [ ] Cilksan reports no races
- [ ] Speedup > 1.5x on 8 cores
- [ ] Correctness maintained across all test inputs
- [ ] No crashes or memory errors

## Next Steps After Basic Parallelization

1. **Add Cilkscale** for parallelism analysis
2. **Tune spawn cutoffs** (if using recursive parallelization)
3. **Parallelize build phase** (if needed)
4. **Optimize load balancing**
5. **Benchmark on different inputs**

