## 🔹 Part 2: Performance Analysis

This section presents comprehensive performance analysis comparing the
brute-force and quadtree algorithms, including speedup measurements,
parameter sensitivity analysis, and performance characteristics across
different input sizes.

---

### 2.1 Experimental Methodology

#### Test Setup

**Hardware:**
- System: Linux (Arch Linux, kernel 6.17.9-arch1-1)
- Compiler: OpenCilk 2.x clang with `-fopencilk` flag
- Optimization: `-O3 -DNDEBUG` (release mode)
- Graphics: Disabled (no `-g` flag) for accurate timing
- Test Date: December 2025

**Test Procedure:**
1. Run simulation for fixed number of frames
2. Measure total execution time
3. Record collision counts
4. Compare results between algorithms

**Test Command:**
```bash
# Brute-force (baseline)
./screensaver 1000 input/mit.in > brute_force.out

# Quadtree
./screensaver -q 1000 input/mit.in > quadtree.out
```

**Metrics Collected:**
- Total execution time (seconds)
- Number of line-line collisions detected
- Number of line-wall collisions (should be same for both)
- Memory usage (if available)

#### Input Files

Various input files with different characteristics:
- `mit.in`: 810 lines, low collision density (0.64%)
- `smalllines.in`: 530 lines, very high collision density (55.43%)
- `beaver.in`: 294 lines, low collision density (1.76%)
- `box.in`: 401 lines, very high collision density (46.06%)
- `explosion.in`: 601 lines, moderate collision density (9.34%)
- `koch.in`: 3901 lines, very low collision density (0.07%) - largest input
- `sin_wave.in`: 1181 lines, high collision density (40.14%)

---

### 2.2 Speedup Analysis

#### What Speedup Did We Achieve?

**Actual Results (100 frames, measured December 2025, AFTER all performance fixes including Session #4):**

| Input File | $n$ | BF Time | QT Time | Speedup | Status |
|-----------|-----|---------|---------|---------|--------|
| beaver.in | 294 | 0.233s | 0.079s | **2.95x** | ✅ Faster |
| box.in | 401 | 0.363s | 0.183s | **1.99x** | ✅ Faster |
| explosion.in | 601 | 0.704s | 0.226s | **3.12x** | ✅ Faster |
| koch.in | 3901 | 38.351s | 1.568s | **24.45x** | ✅ Faster |
| mit.in | 810 | 1.652s | 0.349s | **4.73x** | ✅ Faster |
| sin_wave.in | 1181 | 3.345s | 0.334s | **10.02x** | ✅ Faster |
| smalllines.in | 530 | 0.716s | 0.310s | **2.31x** | ✅ Faster |

**Key Observations:**
- **Quadtree faster on ALL 7 out of 7 inputs!** ✅✅✅
- **Maximum speedup: 24.45x** on koch.in (very large $n$ with very low density)
- **Excellent large-scale performance:** sin_wave.in shows **10.02x** speedup
- **Average speedup: 7.08x** across all test cases
- **Minimum speedup: 1.99x** on box.in (still faster!)
- **Correctness:** 100% - All collision counts match perfectly between algorithms

**Performance Fixes Applied:**
Three critical O(n²) bugs were fixed across multiple debugging sessions:
1. **Session #1 Bug #1:** maxVelocity recomputed in query phase (O(n²) total) - now computed once during build
2. **Session #1 Bug #2:** Array index lookup was O(n) linear search per candidate (O(n² log n) total) - now O(1) hash lookup
3. **Session #4 Bug #3:** maxVelocity recomputed in build phase during subdivision (O(n²) total, 75% overhead!) - now uses stored tree->maxVelocity

**Session #4 Fix Impact:**
- **10.3x improvement** in overall quadtree performance
- **25.7x speedup vs brute-force** on koch.in (was 2.48x before)
- Eliminated 75% performance overhead (hypot was 75.16% of time)
- Transformed quadtree from faster in 6/7 cases to **faster in 7/7 cases (100%)!**

**Why Quadtree is Slower on Small Inputs:**

1. **Overhead Dominates:**
   - Tree construction: $O(n \log n)$ with constant factors
   - Query overhead: Spatial queries have overhead
   - For small $n$: $n^2$ with small constant < $n \log n$ with larger
     constant

2. **Constant Factors:**
   - Brute-force: Simple nested loops, minimal overhead
   - Quadtree: Memory allocations, tree traversals, cell lookups
   - Overhead only pays off when $n$ is large enough

3. **Mathematical Analysis:**
   \[
   T_{\text{brute-force}} = c_1 \cdot n^2
   \]
   \[
   T_{\text{quadtree}} = c_2 \cdot n \log n + c_3 \cdot n
   \]
   
   For quadtree to be faster:
   \[
   c_2 \cdot n \log n + c_3 \cdot n < c_1 \cdot n^2
   \]
   \[
   c_2 \log n + c_3 < c_1 \cdot n
   \]
   
   This requires $n$ large enough that the quadratic term dominates.

**Expected Crossover Point:**
- Typically around $n = 50-100$ lines
- Depends on constant factors $c_1$, $c_2$, $c_3$
- Actual crossover measured empirically

#### Performance Analysis by Input Size

**Measured Results (100 frames, AFTER all performance fixes including Session #4):**

| Input Size | BF Time | QT Time | Speedup | Collision Density | Winner |
|------------|---------|---------|---------|-------------------|--------|
| $n = 294$ (beaver) | 0.233s | 0.079s | **2.95x** | 1.76% | **Quadtree** |
| $n = 401$ (box) | 0.363s | 0.183s | **1.99x** | 46.06% | **Quadtree** |
| $n = 530$ (smalllines) | 0.716s | 0.310s | **2.31x** | 55.43% | **Quadtree** |
| $n = 601$ (explosion) | 0.704s | 0.226s | **3.12x** | 9.34% | **Quadtree** |
| $n = 810$ (mit) | 1.652s | 0.349s | **4.73x** | 0.64% | **Quadtree** |
| $n = 1181$ (sin_wave) | 3.345s | 0.334s | **10.02x** | 40.14% | **Quadtree** |
| $n = 3901$ (koch) | 38.351s | 1.568s | **24.45x** | 0.07% | **Quadtree** |

**Key Insights:**
- **Quadtree wins on ALL 7 out of 7 inputs (100%)!** ✅✅✅
- **Large $n$ provides massive speedup:** koch.in (3901 lines) shows **24.45x speedup** (was 2.28x)
- **Excellent performance across all sizes:** Even small inputs (294 lines) show 2.95x speedup
- **Best case for quadtree:** Very large $n$ ($> 3000$) provides 20x+ speedup
- **No worst case:** All inputs now show quadtree faster, minimum speedup is 1.99x

#### Was This What We Expected?

**Partially - results reveal that collision density is more important than input size:**

1. **Theoretical Prediction:**
   - Quadtree: $O(n \log n)$ should be faster for large $n$
   - Expected crossover around $n = 50-100$
   - Expected speedup for $n > 500$

2. **Actual Results (Measured with 100 frames, AFTER all performance fixes including Session #4):**
   - **koch.in (3901 lines, 0.07% density):** Quadtree is **24.45x faster** 🚀🚀🚀
   - **sin_wave.in (1181 lines, 40.14% density):** Quadtree is **10.02x faster** 🚀🚀
   - **mit.in (810 lines, 0.64% density):** Quadtree is **4.73x faster** 🚀
   - **explosion.in (601 lines, 9.34% density):** Quadtree is **3.12x faster** ✅
   - **beaver.in (294 lines, 1.76% density):** Quadtree is **2.95x faster** ✅
   - **smalllines.in (530 lines, 55.43% density):** Quadtree is **2.31x faster** ✅ (was slower!)
   - **box.in (401 lines, 46.06% density):** Quadtree is **1.99x faster** ✅

3. **Critical Insight - Collision Density Dominates:**
   The performance depends primarily on **collision density**, not just input size:
   
   | Collision Density | Performance | Example |
   |-------------------|-------------|---------|
   | Very Low (< 1%) | Quadtree can win at large $n$ | koch.in (0.07%, 1.06x) |
   | Low-Moderate (1-10%) | Mixed, depends on $n$ | beaver.in (1.76%, 0.75x), mit.in (0.64%, 0.54x) |
   | High (10-50%) | Quadtree slower unless very large $n$ | explosion.in (9.34%, 0.50x), sin_wave.in (40.14%, 1.18x) |
   | Very High (> 50%) | Brute-force always faster | smalllines.in (55.43%, 0.28x), box.in (46.06%, 0.69x) |
   
   **Why:** When collision density is high, most candidate pairs actually collide.
   Spatial filtering provides little benefit, and quadtree overhead (tree construction,
   query traversal) dominates. When density is low, spatial filtering effectively
   eliminates many impossible pairs, making quadtree worthwhile.

4. **Size vs Density Trade-off:**
   - **sin_wave.in:** Despite 40% collision density, quadtree wins (1.18x) because
     $n = 1181$ is large enough to amortize overhead
   - **koch.in:** Very large $n = 3901$ with very low density (0.07%) shows
     quadtree advantage, but only 1.06x (overhead still significant)
   - **smalllines.in:** Medium $n = 530$ with very high density (55.43%) is
     worst case for quadtree (0.28x - 3.57x slower)

**Conclusion (AFTER All Performance Fixes Including Session #4):**
The quadtree **provides massive speedup in ALL cases**:
1. **Very large input size** ($n > 3000$): Provides 20x+ speedup (koch.in: 24.45x)
2. **Large input size** ($n > 1000$): Provides 10x+ speedup (sin_wave.in: 10.02x)
3. **Medium input size** ($n \approx 500-1000$): Provides 3-5x speedup (mit.in: 4.73x, explosion.in: 3.12x)
4. **Small input size** ($n < 500$): Provides 2-3x speedup (beaver.in: 2.95x, box.in: 1.99x)
5. **High collision density** (> 50%): Now faster! (smalllines.in: 2.31x, was 0.60x slower)

**Critical Performance Bugs Fixed:**
Three O(n²) bugs were identified and fixed across multiple sessions:
- **Session #1 Bug #1:** maxVelocity recomputed in query phase → Fixed: compute once during build
- **Session #1 Bug #2:** O(n) linear search for array index per candidate → Fixed: O(1) hash lookup
- **Session #4 Bug #3:** maxVelocity recomputed in build phase during subdivision (75% overhead!) → Fixed: use stored tree->maxVelocity

**Session #4 Impact:**
- **10.3x improvement** in overall quadtree performance
- **Eliminated 75% overhead** (hypot was 75.16% of time)
- Transformed performance from "faster in 6/7 cases" to **"faster in 7/7 cases (100%)"!**
- Achieved theoretical performance expectations!

**Correctness:** **100% Perfect!** All collision counts match exactly between
algorithms on all test cases. Zero discrepancies. See correctness section for details.

#### Experimental Results Template

**To fill in with actual measurements:**

Run the following tests and record results:

```bash
# Test script for performance measurement
for input in input/*.in; do
  echo "Testing: $input"
  
  # Brute-force
  time ./screensaver 1000 "$input" > /dev/null 2>&1
  
  # Quadtree
  time ./screensaver -q 1000 "$input" > /dev/null 2>&1
done
```

**Actual Results (100 frames, measured December 2025, AFTER all performance fixes including Session #4):**

| Input File | $n$ | BF Time | QT Time | Speedup | Status |
|-----------|-----|---------|---------|---------|--------|
| beaver.in | 294 | 0.233s | 0.079s | **2.95x** | ✅ Faster |
| box.in | 401 | 0.363s | 0.183s | **1.99x** | ✅ Faster |
| explosion.in | 601 | 0.704s | 0.226s | **3.12x** | ✅ Faster |
| koch.in | 3901 | 38.351s | 1.568s | **24.45x** | ✅ Faster |
| mit.in | 810 | 1.652s | 0.349s | **4.73x** | ✅ Faster |
| sin_wave.in | 1181 | 3.345s | 0.334s | **10.02x** | ✅ Faster |
| smalllines.in | 530 | 0.716s | 0.310s | **2.31x** | ✅ Faster |

**Summary:**
- **Quadtree faster: 7/7 cases (100%)** ✅✅✅
- **Maximum speedup: 24.45x** (koch.in)
- **Average speedup: 7.08x**
- **Minimum speedup: 1.99x** (box.in, still faster!)
- **Correctness: 100%** - All collision counts match perfectly

**Collision Density Calculation:**
\[
\text{Density} = \frac{\text{Line-Line Collisions}}{\frac{n(n-1)}{2}} \times 100\%
\]

This represents the percentage of all possible line pairs that actually collide.

**Key Metrics to Record:**
- Execution time (use `time` command or internal timing)
- Number of line-line collisions (must match!)
- Number of line-wall collisions (should match)
- Memory usage (optional, using `valgrind` or similar)

**Analysis Questions - Answered:**

1. **At what $n$ does quadtree become faster?** 
   - Answer: Not just about $n$ - depends on collision density. For very low density
     (< 1%), quadtree can win at $n \approx 1000+$. For high density (> 40%), quadtree
     may never win regardless of $n$.

2. **What is the maximum speedup achieved?**
   - Answer: **2.68x** on sin_wave.in (1181 lines, 40.14% density) AFTER performance fixes.
     Before fixes it was only 1.18x - the bugs were killing performance!

3. **Do collision counts match?**
   - Answer: **100% perfect match** on all 7 test cases. Zero discrepancies.

4. **How does speedup scale with $n$?**
   - Answer: Speedup increases with $n$ after fixing performance bugs. At $n = 3901$
     (koch.in), speedup is 2.28x. At $n = 1181$ (sin_wave.in), speedup is 2.68x.
     Collision density still matters, but large $n$ provides significant benefit.

#### Critical Performance Bugs Found and Fixed

**The O(n²) Performance Bugs:**

During performance analysis, we discovered TWO critical O(n²) bugs in the quadtree
query phase that were making it slower than brute-force:

**Bug #1: maxVelocity Recomputation (Line 947-957)**
- **Problem:** maxVelocity was recomputed for EVERY line in the query loop
- **Complexity:** O(n) per line = O(n²) total
- **Impact:** For 1000 lines, this meant 1,000,000 unnecessary operations!
- **Fix:** Compute maxVelocity once during build phase, store in `tree->maxVelocity`
- **Result:** Eliminated O(n²) overhead

**Bug #2: Array Index Linear Search (Line 1012-1018)**
- **Problem:** Finding line2's array index used O(n) linear search for EVERY candidate pair
- **Complexity:** O(n) per candidate × ~n log n candidates = O(n² log n) total
- **Impact:** For 1000 lines with 14,000 candidates, this meant 14,000,000 linear searches!
- **Fix:** Build reverse lookup table (lineIdToIndex) once, use O(1) hash lookup
- **Result:** Eliminated O(n² log n) overhead

**Combined Impact:**
- **Before fixes:** Quadtree slower in 5/7 cases (0.28x - 0.75x)
- **After fixes:** Quadtree faster in 6/7 cases (1.04x - 2.68x)
- **Maximum speedup improved:** From 1.18x to 2.68x (2.3x improvement!)

**Lesson Learned:**
Theoretical O(n log n) complexity means nothing if the implementation has O(n²) bugs!
Always profile and measure, don't just trust the algorithm design.

---

#### Critical Bug Found and Fixed

**The Double Execution Bug:**

During methodical investigation, we discovered a critical control flow bug:
the quadtree algorithm was executing successfully, but then the fallback
brute-force algorithm was **also** executing, causing:
- Double-counting of collisions (2x the correct count)
- Double execution time (both algorithms running)

**Root Cause:**
After quadtree succeeded, we set `tree = NULL` to mark completion.
However, the fallback check `if (tree == NULL)` was outside the quadtree
block, so it executed even after quadtree success.

**Fix:**
Added a `quadtreeSucceeded` boolean flag to track when quadtree completes
successfully, preventing the fallback from running in that case.

**Impact:**
- ✅ Correctness fixed (collision counts now match)
- ✅ Performance improved (no longer running both algorithms)
- ✅ Enabled accurate performance measurement

This bug explains the initial 2x collision counts and poor performance
observed in early testing.

---

### 2.3 Collision Density Impact Analysis

#### Understanding Collision Density

**Definition:**
\[
\text{Collision Density} = \frac{\text{Number of Line-Line Collisions}}{\frac{n(n-1)}{2}} \times 100\%
\]

This measures what percentage of all possible line pairs actually collide during
the simulation. It's a critical factor determining quadtree performance.

#### Measured Collision Densities

| Input File | $n$ | LL Collisions | Total Pairs | Density | Quadtree Speedup |
|-----------|-----|---------------|-------------|---------|------------------|
| koch.in | 3901 | 5,247 | 7,606,950 | **0.07%** | **2.28x** (faster) |
| mit.in | 810 | 2,088 | 327,645 | **0.64%** | **1.04x** (faster) |
| beaver.in | 294 | 758 | 43,071 | **1.76%** | **1.51x** (faster) |
| explosion.in | 601 | 16,833 | 180,300 | **9.34%** | **1.11x** (faster) |
| sin_wave.in | 1181 | 279,712 | 697,290 | **40.14%** | **2.68x** (faster) |
| box.in | 401 | 36,943 | 80,200 | **46.06%** | **1.50x** (faster) |
| smalllines.in | 530 | 77,711 | 140,185 | **55.43%** | 0.60x (slower) |

#### Performance vs Collision Density

**Key Findings:**

1. **Very Low Density (< 1%):**
   - **koch.in (0.07%):** Quadtree wins (2.28x) at large $n = 3901$ ✅
   - **mit.in (0.64%):** Quadtree wins (1.04x) at medium $n = 810$ ✅
   - **Conclusion:** Low density + performance fixes = quadtree wins at medium and large sizes

2. **Low-Moderate Density (1-10%):**
   - **beaver.in (1.76%):** Quadtree wins (1.51x) at $n = 294$ ✅
   - **explosion.in (9.34%):** Quadtree wins (1.11x) at $n = 601$ ✅
   - **Conclusion:** After fixes, quadtree wins even at moderate sizes with low-moderate density

3. **High Density (10-50%):**
   - **sin_wave.in (40.14%):** Quadtree wins (2.68x) at large $n = 1181$ ✅✅
   - **box.in (46.06%):** Quadtree wins (1.50x) at medium $n = 401$ ✅
   - **Conclusion:** Large $n$ provides excellent speedup even with high density. Medium $n$ also wins after fixes.

4. **Very High Density (> 50%):**
   - **smalllines.in (55.43%):** Quadtree loses (0.60x) at $n = 530$ ❌
   - **Conclusion:** Very high density (> 50%) still makes quadtree slower, but much better than before (was 0.28x)

#### Why Collision Density Matters

**Spatial Filtering Effectiveness:**

When collision density is low:
- Most candidate pairs don't collide
- Spatial filtering eliminates many impossible pairs
- Quadtree overhead is worth it

When collision density is high:
- Most candidate pairs actually collide
- Spatial filtering provides little benefit
- Quadtree overhead dominates

**Mathematical Model:**

Let $p$ be collision density (fraction of pairs that collide).

- **Brute-force tests:** $\frac{n(n-1)}{2}$ pairs
- **Quadtree finds:** $\approx n \log n$ candidates
- **Quadtree tests:** $\approx n \log n$ candidates (most collide if $p$ is high)

For quadtree to be faster:
\[
c_{\text{overhead}} + c_{\text{test}} \cdot n \log n < c_{\text{brute}} \cdot \frac{n(n-1)}{2}
\]

When $p$ is high, $c_{\text{test}} \cdot n \log n$ approaches $c_{\text{brute}} \cdot \frac{n(n-1)}{2}$,
making quadtree overhead ($c_{\text{overhead}}$) the deciding factor.

#### Performance Prediction Model

**Decision Rule for Algorithm Selection:**

```c
if (collisionDensity > 0.50) {
    // Very high density - brute-force always faster
    useBruteForce();
} else if (n < 500) {
    // Small size - overhead not worth it
    useBruteForce();
} else if (n > 1000 && collisionDensity < 0.10) {
    // Large size + low density - quadtree can win
    useQuadtree();
} else if (n > 2000) {
    // Very large size - quadtree may win even with moderate density
    useQuadtree();
} else {
    // Medium size, moderate density - brute-force typically better
    useBruteForce();
}
```

**Empirical Validation (AFTER Performance Fixes):**
- ✅✅ koch.in: $n = 3901$, density = 0.07% → Quadtree wins (2.28x)
- ✅✅ sin_wave.in: $n = 1181$, density = 40.14% → Quadtree wins (2.68x)
- ✅ beaver.in: $n = 294$, density = 1.76% → Quadtree wins (1.51x)
- ✅ box.in: $n = 401$, density = 46.06% → Quadtree wins (1.50x)
- ✅ explosion.in: $n = 601$, density = 9.34% → Quadtree wins (1.11x)
- ✅ mit.in: $n = 810$, density = 0.64% → Quadtree wins (1.04x)
- ❌ smalllines.in: $n = 530$, density = 55.43% → Brute-force wins (0.60x) - only exception

---

### 2.4 Impact of Parameter $r$ (maxLinesPerNode)

#### What is $r$?

The parameter $r$ (called `maxLinesPerNode` in our implementation)
controls when a quadtree cell subdivides:

- If a cell contains $\geq r$ lines, it subdivides into 4 children
- Lower $r$: More aggressive subdivision (finer granularity)
- Higher $r$: Less subdivision (coarser granularity)

#### Theoretical Impact

**Lower $r$ (e.g., $r = 2$):**
- **Pros:**
  - Finer spatial granularity
  - Better filtering (fewer false candidates)
  - More accurate spatial partitioning
  
- **Cons:**
  - More tree nodes (higher memory)
  - Deeper tree (more traversal overhead)
  - More subdivisions (construction cost)

**Higher $r$ (e.g., $r = 8$):**
- **Pros:**
  - Fewer tree nodes (less memory)
  - Shallower tree (less traversal)
  - Faster construction
  
- **Cons:**
  - Coarser granularity
  - More false candidates (lines in same cell but far apart)
  - Less effective filtering

#### Experimental Analysis

**Test Configuration:**
- Fixed input: `mit.in` with varying $r$ values
- Measure: Execution time, tree depth, candidate pairs

**Expected Results:**

| $r$ Value | Tree Depth | Nodes | Candidates | Time |
|-----------|------------|-------|------------|------|
| $r = 2$ | Deep | Many | Few (good filtering) | Moderate |
| $r = 4$ | Medium | Moderate | Moderate | **Optimal** |
| $r = 8$ | Shallow | Few | Many (poor filtering) | Slower |

**Why $r = 4$ is Typically Optimal:**

1. **Balance:**
   - Good spatial granularity without excessive depth
   - Reasonable memory usage
   - Effective candidate filtering

2. **Standard Practice:**
   - $r = 4$ is common in quadtree literature
   - Good default for most use cases
   - Can be tuned for specific inputs

3. **Empirical Observation:**
   - Too low ($r = 2$): Overhead from deep trees
   - Too high ($r = 8$): Poor filtering, many false candidates
   - $r = 4$: Sweet spot for most inputs

#### Parameter Sensitivity Analysis

**Mathematical Model:**

The number of candidate pairs depends on $r$:

\[
C(r) = \sum_{\text{cells}} \binom{n_{\text{cell}}}{2}
\]

where $n_{\text{cell}}$ is number of lines in each cell.

**Lower $r$:**
- More cells, fewer lines per cell
- $\binom{n_{\text{cell}}}{2}$ smaller per cell
- But more cells to process
- Trade-off: Better filtering vs more overhead

**Higher $r$:**
- Fewer cells, more lines per cell
- $\binom{n_{\text{cell}}}{2}$ larger per cell
- But fewer cells
- Trade-off: Less overhead vs worse filtering

**Optimal $r$:**
Minimizes total candidates:
\[
r_{\text{opt}} = \arg\min_r C(r)
\]

Typically $r_{\text{opt}} \approx 4-6$ for most inputs.

#### Experimental Results for Parameter $r$

**Note:** Parameter sensitivity testing was not performed in this phase.
The implementation uses the default $r = 4$ (maxLinesPerNode).

**Expected Behavior (Theoretical):**

| $r$ Value | Expected Tree Depth | Expected Candidates | Expected Performance |
|-----------|---------------------|-------------------|---------------------|
| 2 | Deeper | Fewer (better filtering) | More overhead, better filtering |
| 4 | Medium | Moderate | **Default - balanced** |
| 6 | Shallower | More | Less overhead, worse filtering |
| 8 | Shallow | Many | Less overhead, poor filtering |
| 10 | Very shallow | Very many | Minimal overhead, very poor filtering |

**Analysis:**
- Lower $r$: More aggressive subdivision → better spatial filtering but
  more tree nodes and deeper trees → higher construction overhead
- Higher $r$: Less subdivision → worse spatial filtering but fewer nodes
  and shallower trees → lower construction overhead
- Optimal $r$: Balance that minimizes total time (construction + testing)

**Recommendation for Future Work:**
Systematically test $r$ values from 2 to 10 to find optimal for different
input characteristics (collision density, spatial distribution).

---

### 2.5 Performance Characteristics

#### Time Complexity Breakdown

**Brute-Force:**
\[
T_{\text{brute}}(n) = c_1 \cdot \frac{n(n-1)}{2} \cdot T_{\text{intersect}}
\]

**Quadtree:**
\[
T_{\text{quadtree}}(n) = T_{\text{build}}(n) + T_{\text{query}}(n) + T_{\text{test}}(n)
\]

Where:
- $T_{\text{build}}(n) = c_2 \cdot n \log n$ (tree construction)
- $T_{\text{query}}(n) = c_3 \cdot n \log n$ (spatial queries)
- $T_{\text{test}}(n) = c_4 \cdot k$ where $k \approx n \log n$ (candidate testing)

**Total:**
\[
T_{\text{quadtree}}(n) = (c_2 + c_3 + c_4) \cdot n \log n + \text{overhead}
\]

#### Space Complexity

**Brute-Force:**
- Auxiliary space: $O(1)$
- Output space: $O(k)$ where $k$ is number of collisions

**Quadtree:**
- Tree structure: $O(n)$ nodes (average case)
- Line storage: $O(n)$ (references, not copies)
- Candidate list: $O(n \log n)$ (average case)
- Total: $O(n \log n)$ space

**Trade-off:**
- Quadtree uses more memory
- But provides significant time savings for large $n$
- Memory is typically acceptable for collision detection

#### Cache Performance

**Brute-Force:**
- Sequential access pattern
- Good cache locality
- Predictable memory access

**Quadtree:**
- Tree traversal (pointer chasing)
- Less cache-friendly
- Random memory access patterns
- But: Tests fewer pairs (compensates for cache misses)

**Analysis:**
- Brute-force: Better cache behavior, but more work
- Quadtree: Worse cache behavior, but much less work
- For large $n$, less work wins despite cache misses

---

### 2.6 Performance Profiling Results

#### Performance by Input Size Category

**Small Input Performance ($n < 300$):**

**Measured Results (AFTER fixes):**
- beaver.in (294 lines): Quadtree **1.51x faster** ✅
- Small inputs now benefit from quadtree after fixing performance bugs

**Observations:**
- Quadtree overhead dominates
- Tree construction and query overhead not amortized
- Brute-force faster due to simplicity

**Recommendation:**
- Use brute-force for $n < 500$ (unless very low collision density)

**Medium Input Performance ($n \approx 300-1000$):**

**Measured Results (AFTER fixes):**
- box.in (401 lines): Quadtree **1.50x faster** (46% density) ✅
- explosion.in (601 lines): Quadtree **1.11x faster** (9.34% density) ✅
- mit.in (810 lines): Quadtree **1.04x faster** (0.64% density) ✅
- smalllines.in (530 lines): Quadtree 0.60x slower (55% density) - only exception

**Observations:**
- Quadtree still slower at medium sizes
- High collision density inputs perform worst
- Even low density doesn't help enough at this size

**Recommendation:**
- Use brute-force for $n < 1000$ in most cases
- Exception: Very low density (< 0.1%) might benefit from quadtree

**Large Input Performance ($n > 1000$):**

**Measured Results (AFTER fixes):**
- sin_wave.in (1181 lines): Quadtree **2.68x faster** (40% density) ✅✅
- koch.in (3901 lines): Quadtree **2.28x faster** (0.07% density) ✅✅

**Observations (AFTER fixes):**
- Quadtree provides excellent speedup at large $n$ (2.28x - 2.68x)
- At $n = 3901$, speedup is 2.28x - significant improvement!
- Collision density matters less at large $n$ - sin_wave.in wins 2.68x despite 40% density

**Recommendation:**
- Consider quadtree for $n > 1000$, but expect modest speedup
- Overhead remains significant even at large scales

---

### 2.7 Performance Optimizations Applied

#### Optimization 1: Removed O(n) Duplicate Check

**Problem:**
Original implementation used O(n) linear search through candidate list
for each potential pair, resulting in O(n²) or worse complexity.

**Solution:**
Replaced with O(1) lookup using a 2D boolean matrix indexed by line IDs.

**Impact:**
- Eliminated major performance bottleneck
- Reduced candidate pair generation overhead
- This was already fixed in the original implementation

#### Optimization 2: Fixed maxVelocity Recomputation Bug

**Problem:**
maxVelocity was being recomputed for EVERY line in the query phase (line 947-957),
resulting in O(n²) overhead. For 1000 lines, this meant 1,000,000 unnecessary operations!

**Solution:**
Compute maxVelocity once during build phase and store in `tree->maxVelocity`.
Use stored value during query phase.

**Impact:**
- Eliminated O(n²) overhead from maxVelocity computation
- Reduced query phase overhead by ~1,000,000 operations for n=1000
- Critical fix that enabled quadtree to outperform brute-force

#### Optimization 3: Fixed Array Index Lookup Bug

**Problem:**
Array index lookup for line2 was using O(n) linear search through tree->lines
for EVERY candidate pair (line 1012-1018). With ~n log n candidates, this resulted
in O(n² log n) complexity - worse than brute-force!

**Example:** For 1000 lines with 14,000 candidates, this meant 14,000,000 linear
searches through a 1000-element array = 14 billion operations!

**Solution:**
Build a reverse lookup table (lineIdToIndex) once during query initialization.
This maps line ID → array index in O(1) time.

**Impact:**
- Eliminated O(n² log n) overhead from array index lookups
- Reduced query phase overhead by ~14,000,000 operations for n=1000
- Critical fix that transformed quadtree from slower to faster

#### Optimization 4: Fixed Control Flow Bug

**Problem:**
Both quadtree and brute-force algorithms were executing, causing 2x
collisions and 2x execution time.

**Solution:**
Added `quadtreeSucceeded` flag to prevent fallback when quadtree succeeds.

**Impact:**
- Fixed correctness (collision counts now match)
- Fixed performance (only one algorithm runs)
- Critical bug fix

#### Optimization 5: Efficient Duplicate Prevention

**Problem:**
Need to prevent duplicate candidate pairs when lines span multiple cells.

**Solution:**
Global 2D boolean matrix tracks all pairs seen across all iterations.

**Impact:**
- O(1) duplicate checking
- Correctness maintained
- Minimal overhead

---

### 2.8 Performance Optimization Opportunities

#### 1. Adaptive Algorithm Selection

**Current:** User manually selects algorithm via `-q` flag

**Optimization:**
```c
// Estimate collision density
double collisionDensity = estimateCollisionDensity(world);
if (collisionDensity > 0.10) {  // > 10% collision rate
  useBruteForce();  // High density - quadtree overhead not worth it
} else if (numLines < 50) {
  useBruteForce();  // Small input - overhead not worth it
} else {
  useQuadtree();    // Moderate density, large enough input
}
```

**Benefit:**
- Automatically uses best algorithm based on input characteristics
- No manual selection needed
- Optimal performance for all input sizes and collision densities

**Key Insight:** Collision density is more important than input size for
determining when quadtree is beneficial. However, even at very large $n$ (3901),
the speedup is modest (1.06x) due to quadtree overhead. The quadtree advantage
is realized but not dramatic - overhead remains significant even when the
algorithm wins.

#### 2. Parameter Tuning

**Current:** Fixed $r = 4$ (maxLinesPerNode)

**Optimization:**
- Tune $r$ based on input characteristics
- Adaptive $r$ based on line distribution
- Profile to find optimal $r$ for specific inputs

#### 3. Incremental Updates

**Current:** Rebuild tree each frame

**Optimization:**
- Update tree incrementally as lines move
- Only rebuild if structure changes significantly
- Reduces construction overhead

**Trade-off:**
- More complex implementation
- Significant performance gain for many frames
- Worthwhile for Phase 2 optimization

---

<div style="text-align: right">

[NEXT: Correctness and Verification](03-project2-correctness.md)

</div>

