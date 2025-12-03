# Discrepancy Analysis - File Index

## Quick Navigation

### Reports
- **[INVESTIGATION_REPORT.md](INVESTIGATION_REPORT.md)** - Full investigation report
- **[STATISTICS.txt](STATISTICS.txt)** - Summary statistics
- **[FILE_INVENTORY.txt](FILE_INVENTORY.txt)** - Complete file inventory
- **[README.md](README.md)** - Directory overview

### Raw Data Files
- **[pair_101_105_bf.txt](pair_101_105_bf.txt)** - BF raw data for pair (101,105)
- **[pair_101_105_qt.txt](pair_101_105_qt.txt)** - QT raw data for pair (101,105)
- **[line_105_collisions_bf.txt](line_105_collisions_bf.txt)** - All BF collisions with line 105
- **[line_105_collisions_qt.txt](line_105_collisions_qt.txt)** - All QT collisions with line 105

### History Files
- **[history_files/pair_101_105_bf_history.txt](history_files/pair_101_105_bf_history.txt)** - BF history of pair (101,105)
- **[history_files/pair_101_105_qt_history.txt](history_files/pair_101_105_qt_history.txt)** - QT history of pair (101,105)

### Comparison Files
- **[comparison_files/pair_101_105_comparison.txt](comparison_files/pair_101_105_comparison.txt)** - Frame-by-frame comparison
- **[comparison_files/line_105_collision_comparison.txt](comparison_files/line_105_collision_comparison.txt)** - Collision comparison

### Analysis Files
- **[line_105_analysis.txt](line_105_analysis.txt)** - Summary analysis

### Scripts
- **[scripts/](scripts/)** - Analysis and debugging scripts
  - `create_comparison.py` - Python script for comparing physical quantities
  - `extract_history.sh` - Shell script for extracting history
  - `extract_comparison.sh` - Shell script for extracting comparisons
  - `debug_discrepancy.sh` - Automated discrepancy debugging
  - `benchmark.sh` - Performance benchmarking
  - `performance_test.sh` - Performance testing with complexity analysis
  - `profile_quadtree.sh` - Profiling quadtree implementation
  - `test_cutoff.sh` - Testing different cutoff values
  - See [scripts/README.md](scripts/README.md) for detailed documentation

### Debug Logs
- **[debug_logs/](debug_logs/)** - Raw debug output files
  - `debug_bf_pairs.txt` - All pairs tested by BF (718MB)
  - `debug_bf_collisions.txt` - All collisions found by BF
  - `debug_qt_pairs.txt` - All pairs tested by QT (44MB)
  - `debug_qt_collisions.txt` - All collisions found by QT

## Key Metrics

- **First divergence**: Frame 90
- **Missing collisions**: 13 in frames 88-90
- **False positives**: 2 collisions
- **Match rate**: 65.7% for line 105 collisions

### Scripts
- **[scripts/](scripts/)** - Analysis and debugging scripts
  - `create_comparison.py` - Python script for comparing physical quantities
  - `extract_history.sh` - Shell script for extracting history
  - `debug_discrepancy.sh` - Automated discrepancy debugging
  - `benchmark.sh` - Performance benchmarking
  - `performance_test.sh` - Performance testing with complexity analysis
  - `profile_quadtree.sh` - Profiling quadtree implementation
  - `test_cutoff.sh` - Testing different cutoff values
