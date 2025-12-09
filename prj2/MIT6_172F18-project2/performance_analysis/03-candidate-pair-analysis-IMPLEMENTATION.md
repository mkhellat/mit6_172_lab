# Performance Debugging Session #3: Implementation Summary

**Date:** December 2025  
**Status:** ✅ INSTRUMENTATION COMPLETE  
**Next Step:** Run benchmarks and collect data

---

## Instrumentation Added

### Step 1: Candidate Pair Generation Statistics ✅

**Location:** `quadtree.c`, after candidate pair generation (line ~1157)

**What it measures:**
- Actual candidate pair count vs theoretical ($n \log n$)
- Ratio of candidates to brute-force pairs
- Average cells per line
- Ratio gap (how much worse than expected)

**Output (when `DEBUG_QUADTREE_STATS` defined):**
```
===== QUADTREE CANDIDATE PAIR STATISTICS (Step 1) =====
Input: n=3901 lines
Brute-force pairs: 7,606,950
Candidate pairs generated: [actual count]
Expected candidates (n*log2(n)): ~42,911
Expected ratio: 0.56%
Actual ratio: [actual %]
Ratio gap: [actual/expected]
Average cells per line: [average]
======================================================
```

### Step 2: Time Breakdown Analysis ✅

**Locations:**
- Build phase: `quadtree.c`, `QuadTree_build()` function
- Query phase: `quadtree.c`, `QuadTree_findCandidatePairs()` function
- Sort phase: `collision_world.c`, before/after `qsort()`
- Test phase: `collision_world.c`, before/after candidate testing loop

**What it measures:**
- Build phase time (tree construction)
- Query phase time (spatial filtering)
- Sort phase time (candidate pair sorting)
- Test phase time (actual collision testing)

**Output (when `DEBUG_QUADTREE_TIMING` defined):**
```
BUILD PHASE TIMING: [time]s
QUERY PHASE TIMING: [time]s
===== QUADTREE TIME BREAKDOWN (Step 2) =====
Sort phase: [time]s
Test phase: [time]s
==========================================
```

### Step 3: Tree Structure Analysis ✅

**Location:** `quadtree.c`, after query phase (line ~1270)

**What it measures:**
- Total nodes and leaf nodes
- Maximum depth reached vs expected (log₄(n))
- Maximum lines in any node
- Average lines per leaf
- Empty cells percentage

**Output (when `DEBUG_QUADTREE_STATS` defined and stats enabled):**
```
===== QUADTREE STRUCTURE ANALYSIS (Step 3) =====
Tree Structure:
  Total nodes: [count]
  Leaf nodes: [count]
  Max depth reached: [depth]
  Expected depth (log4(n)): ~[expected]
  Depth ratio: [actual/expected]x
  Max lines in any node: [count]
  Average lines per leaf: [average]
  Empty cells: [count] ([percentage]%)
  Lines in tree: [count]
===============================================
```

---

## How to Use

### Compile with Instrumentation

```bash
cd prj2/MIT6_172F18-project2
make clean
make CFLAGS="-O3 -DNDEBUG -DDEBUG_QUADTREE_STATS -DDEBUG_QUADTREE_TIMING"
```

**Note:** The Makefile may need to be updated to properly pass CFLAGS. Alternatively, edit the Makefile directly or compile manually.

### Enable Debug Stats in Tree Config

The tree structure analysis requires debug stats to be enabled when creating the tree. Check `collision_world.c` to ensure:

```c
QuadTreeConfig config = {
  .maxDepth = 12,
  .maxLinesPerNode = 32,
  .minCellSize = 0.001,
  .enableDebugStats = true  // Must be true for Step 3
};
```

### Run Benchmark

```bash
# Run with quadtree and capture stderr output
./screensaver -q 100 input/koch.in 2> koch_stats.txt

# Or for a single frame to see detailed stats
./screensaver -q 1 input/koch.in 2> koch_stats.txt
```

### Analyze Output

The instrumentation will output three sections:
1. **Step 1:** Candidate pair statistics (shows if we're generating too many candidates)
2. **Step 2:** Time breakdown (shows where time is spent)
3. **Step 3:** Tree structure (shows tree characteristics)

---

## Expected Findings

### If Too Many Candidates (Primary Hypothesis #3)

**Symptoms:**
- Actual ratio >> expected ratio (e.g., 10-50% instead of 0.6-2%)
- Average cells per line >> 4 (lines spanning many cells)
- Ratio gap >> 10x

**Action:**
- Investigate bounding box expansion factors
- Check if tree is subdividing effectively
- Consider reducing `k_rel` and `k_gap` parameters

### If High Overhead (Secondary Hypotheses #1, #2, #3)

**Symptoms:**
- Build + Query time >> Test time
- Construction overhead dominates
- Memory allocation overhead significant

**Action:**
- Optimize tree construction
- Reduce query traversal overhead
- Consider caching tree structure across frames

### If Tree Structure Issues (Primary Hypothesis #3)

**Symptoms:**
- Depth ratio >> 1.0 (tree too deep)
- Average lines per leaf >> maxLinesPerNode (not subdividing)
- Many empty cells (inefficient subdivision)

**Action:**
- Adjust `maxLinesPerNode` parameter
- Check subdivision logic
- Optimize tree structure

---

## Next Steps

1. **Run benchmarks** on test inputs (koch.in, sin_wave.in)
2. **Collect statistics** from stderr output
3. **Analyze results** to identify primary bottleneck
4. **Document findings** in this session's main document
5. **Implement fixes** based on findings

---

**Status:** ✅ Ready for data collection  
**Compilation:** ✅ Successful  
**Instrumentation:** ✅ Complete

