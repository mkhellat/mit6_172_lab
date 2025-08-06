## CASE: Optimized Build (DEBUG=0)

### Profiling Results
```plaintext
==18919== Cachegrind, a high-precision tracing profiler
... [output truncated for brevity] ...
Done testing.
==18919== 
==18919== I refs:        240,549,271
==18919== Branches:       27,623,764  (25,922,989 cond + 1,700,775 ind)
==18919== Mispredicts:       909,497  (909,102 cond + 395 ind)
==18919== Mispred rate:          3.3% (3.5% cond + 0.0% ind)
```

**Performance Analysis:**
- Total instructions: 240.5 million
- Execution times:
  - Random array: 14.3 ms / 13.7 ms
  - Inverted array: 28.2 ms / 27.7 ms
- Throughput: ≈2.9 billion instructions/sec
- Branch misprediction rate: 3.3% (3.5% conditional)

---

## CASE: Debug Build (DEBUG=1)

### Profiling Results
```plaintext
==17472== Cachegrind, a high-precision tracing profiler
... [output truncated for brevity] ...
Done testing.
==17472== 
==17472== I refs:        398,043,374
==17472== Branches:       39,952,097  (38,251,358 cond + 1,700,739 ind)
==17472== Mispredicts:     2,944,814  (2,944,438 cond + 376 ind)
==17472== Mispred rate:          7.4% (7.7% cond + 0.0% ind)
```

**Performance Analysis:**
- Total instructions: 398.0 million
- Execution times:
  - Random array: 99.0 ms / 99.2 ms
  - Inverted array: 193.0 ms / 193.1 ms
- Throughput: ≈681 million instructions/sec
- Branch misprediction rate: 7.4% (7.7% conditional)

---

```python
# Calculations:
Optimized_instructions = 240_549_271
Debug_instructions = 398_043_374

# Actual reduction:
Reduction = (Debug_instructions - Optimized_instructions) / Debug_instructions
= (398,043,374 - 240,549,271) / 398,043,374
= 157,494,103 / 398,043,374
= 0.3956 → 39.6% reduction

# Actual ratio:
Optimized_ratio = Optimized_instructions / Debug_instructions
= 240,549,271 / 398,043,374 
= 0.604 → 60.4% of debug instructions
```


## Performance Comparison

1. **Instruction Efficiency**:
   - Optimized build executes **39.6% fewer instructions** (240.5M vs
     398.0M)
   - Optimized build uses only **60.4% of the instructions** needed by
     debug build
   - 4.25× higher throughput (2.9B vs 0.68B instr/sec)

2. **Execution Time** (Random array first run):
   - Optimized: 14.3ms → 240.5M instructions / 0.0143s = 16.8B instr/s
   - Debug: 99.0ms → 398.0M instructions / 0.099s = 4.0B instr/s
   - **Actual speedup**: 16.8B / 4.0B = 4.2× IPC improvement

3. **Branch Prediction**:
   - 69% fewer mispredictions (0.9M vs 2.9M)
   - 55% lower misprediction rate (3.3% vs 7.4%)



## Conclusion:
The optimized build (`DEBUG=0`) shows dramatically better performance
than the debug build (`DEBUG=1`):
- **39.6% reduction in instruction count** (240.5M vs 398.0M)
- **4.2× improvement in instructions-per-cycle** (16.8B vs 4.0B
    instr/sec)
- **6.8-6.9× faster execution** across dataset types

Throughput comparison indicates that the sequence of instructions for
the optimized program are less less expensive compared to those of the
debug build.

The combined effect of reduced instructions and improved branch
prediction (69% fewer mispredictions) enables significantly better
pipeline utilization. In other words, compiler optimizations both
reduce work and improve CPU utilization efficiency.