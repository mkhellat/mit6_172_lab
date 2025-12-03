# Discrepancy Analysis Report

## Overview
This directory contains a comprehensive analysis of the discrepancy between brute-force (BF) and quadtree (QT) collision detection algorithms, specifically focusing on pair (101,105) and line 105's collision history.

## Directory Structure

```
discrepancy_analysis/
├── README.md                          # This file
├── INDEX.md                           # File index and navigation
├── INVESTIGATION_REPORT.md            # Full investigation report
├── STATISTICS.txt                     # Summary statistics
├── pair_101_105_bf.txt               # BF raw data for pair (101,105)
├── pair_101_105_qt.txt               # QT raw data for pair (101,105)
├── line_105_analysis.txt             # Summary analysis
├── line_105_collisions_bf.txt        # All BF collisions involving line 105
├── line_105_collisions_qt.txt        # All QT collisions involving line 105
├── history_files/                     # History files
│   ├── pair_101_105_bf_history.txt  # BF history of pair (101,105)
│   └── pair_101_105_qt_history.txt  # QT history of pair (101,105)
├── comparison_files/                  # Comparison files
│   ├── pair_101_105_comparison.txt  # Side-by-side comparison
│   └── line_105_collision_comparison.txt # Detailed collision comparison
├── scripts/                           # Analysis scripts
│   ├── README.md                     # Scripts documentation
│   ├── SCRIPTS_SUMMARY.txt           # Scripts summary
│   ├── create_comparison.py          # Python comparison script
│   ├── extract_comparison.sh         # Shell extraction script
│   ├── extract_history.sh           # History extraction script
│   ├── debug_discrepancy.sh         # Automated debugging
│   ├── benchmark.sh                 # Performance benchmarking
│   ├── performance_test.sh          # Performance testing
│   ├── profile_quadtree.sh          # Profiling script
│   └── test_cutoff.sh              # Cutoff testing script
└── debug_logs/                       # Raw debug output files
    ├── debug_bf_pairs.txt           # All pairs tested by BF
    ├── debug_bf_collisions.txt      # All collisions found by BF
    ├── debug_qt_pairs.txt           # All pairs tested by QT
    └── debug_qt_collisions.txt      # All collisions found by QT
```

## Key Findings

### 1. Divergence Point
- **First divergence**: Frame 90
- **Pair analyzed**: (101,105)
- **Root cause**: Line 105 (l2) had different collision histories

### 2. Frame 89 Status
- BF tested (101,105) in frame 89
- QT did NOT test (101,105) in frame 89
- This is a critical missing test case

### 3. Line 105 Collision Discrepancies (Frames 88-90)

**Frame 88:**
- BF: 3 collisions
- QT: 2 collisions
- Missing: (105,161)

**Frame 89 (CRITICAL):**
- BF: 9 collisions
- QT: 1 collision
- Missing: All 9 BF collisions
- False positive: (105,137) - BF did not find this

**Frame 90:**
- BF: 4 collisions
- QT: 1 collision
- Missing: All 4 BF collisions
- False positive: (105,137) - BF did not find this

### 4. Total Impact
- QT missed 13 collisions that BF found
- QT found 2 false positives
- This caused line 105's state to diverge, leading to different physics

## Files Description

### Documentation
- `README.md`: This file - directory overview
- `INDEX.md`: File index and navigation guide
- `INVESTIGATION_REPORT.md`: Full investigation report
- `STATISTICS.txt`: Summary statistics
- `FILE_INVENTORY.txt`: Complete file inventory

### Raw Data Files
- `pair_101_105_bf.txt`: BF raw data for pair (101,105)
- `pair_101_105_qt.txt`: QT raw data for pair (101,105)
- `line_105_collisions_bf.txt`: All BF collisions involving line 105
- `line_105_collisions_qt.txt`: All QT collisions involving line 105
- `line_105_analysis.txt`: Summary analysis

### History Files (history_files/)
- `pair_101_105_bf_history.txt`: Complete history of pair (101,105) physical quantities for BF
- `pair_101_105_qt_history.txt`: Complete history of pair (101,105) physical quantities for QT

### Comparison Files (comparison_files/)
- `pair_101_105_comparison.txt`: Frame-by-frame comparison showing when divergence occurs
- `line_105_collision_comparison.txt`: Detailed comparison of line 105's collisions

### Scripts (scripts/)
- `create_comparison.py`: Python script for comparing physical quantities
- `extract_history.sh`: Shell script for extracting history
- `extract_comparison.sh`: Shell script for extracting comparisons
- `debug_discrepancy.sh`: Automated discrepancy debugging
- `benchmark.sh`, `performance_test.sh`, `profile_quadtree.sh`, `test_cutoff.sh`: Performance testing scripts
- See `scripts/README.md` for detailed documentation

### Debug Logs (debug_logs/)
Raw output from both algorithms for detailed analysis:
- `debug_bf_pairs.txt`: All pairs tested by BF (718MB)
- `debug_bf_collisions.txt`: All collisions found by BF
- `debug_qt_pairs.txt`: All pairs tested by QT (44MB)
- `debug_qt_collisions.txt`: All collisions found by QT

## Next Steps
1. Investigate why QT missed 9 collisions in frame 89
2. Investigate why QT found false positive (105,137)
3. Fix the quadtree algorithm to match BF's collision detection


## Scripts Directory

The `scripts/` directory contains:
- **Analysis scripts**: `create_comparison.py`, `extract_history.sh`
- **Debugging scripts**: `debug_discrepancy.sh`
- **Performance scripts**: `benchmark.sh`, `performance_test.sh`, `profile_quadtree.sh`, `test_cutoff.sh`

See [scripts/README.md](scripts/README.md) for detailed documentation.
