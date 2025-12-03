# Analysis Scripts

This directory contains all scripts used for the discrepancy analysis.

## Scripts Used in Investigation

### Analysis Scripts

#### `create_comparison.py`
**Purpose**: Extracts and compares physical quantities from BF and QT debug output  
**Usage**: 
```bash
python3 create_comparison.py > pair_101_105_comparison.txt
```
**Input**: 
- `pair_101_105_bf.txt` - BF history of pair (101,105)
- `pair_101_105_qt.txt` - QT history of pair (101,105)

**Output**: Frame-by-frame comparison with divergence markers

#### `extract_history.sh`
**Purpose**: Extracts history for pair (101,105) and creates comparison files  
**Usage**:
```bash
./extract_history.sh
```
**Creates**:
- `pair_101_105_bf_history.txt`
- `pair_101_105_qt_history.txt`
- `pair_101_105_comparison.txt`

#### `extract_comparison.sh`
**Purpose**: Alternative extraction script for creating comparisons  
**Usage**:
```bash
./extract_comparison.sh
```
**Note**: Similar functionality to `extract_history.sh`, provides alternative extraction method

### Debugging Scripts

#### `debug_discrepancy.sh`
**Purpose**: Automates comparison of candidate pairs and collisions found by BF and QT  
**Usage**:
```bash
./debug_discrepancy.sh <input_file> <num_frames>
```
**Features**:
- Compiles project with `-DDEBUG_DISCREPANCY`
- Runs both BF and QT
- Extracts unique collision pairs
- Identifies pairs present in QT but not BF, and vice-versa
- Provides options to sample extra collisions

### Performance Testing Scripts

#### `benchmark.sh`
**Purpose**: Automated performance testing for BF and QT implementations  
**Usage**:
```bash
./benchmark.sh -f <frames> -i <input_file> -o <output_file>
```
**Features**:
- Tests multiple input files
- Captures execution time and collision counts
- Calculates speedup ratios

#### `performance_test.sh`
**Purpose**: Performance testing with complexity pattern analysis  
**Usage**:
```bash
./performance_test.sh -f <frames> -w <width> -r <runs> -i <input_file>
```
**Features**:
- Analyzes complexity patterns
- Multiple runs for statistical analysis
- Detailed timing information

#### `profile_quadtree.sh`
**Purpose**: Profiling quadtree implementation  
**Usage**:
```bash
./profile_quadtree.sh -f <frames> -i <input_file>
```
**Features**:
- Detailed performance analysis
- Time command output
- Function-level profiling

#### `test_cutoff.sh`
**Purpose**: Tests different `maxLinesPerNode` values  
**Usage**:
```bash
./test_cutoff.sh -f <frames> -i <input_file> -c <cutoff_values>
```
**Features**:
- Automates testing of different cutoff values
- Finds optimal cutoff for quadtree subdivision
- Backup/restore mechanism for quadtree.c

## Script Execution Order

For the discrepancy investigation:

1. **Enable debug logging**: Compile with `-DDEBUG_DISCREPANCY`
2. **Run simulations**:
   ```bash
   ./screensaver 100 input/box.in > /dev/null 2>&1
   ./screensaver -q 100 input/box.in > /dev/null 2>&1
   ```
3. **Extract pair history**:
   ```bash
   grep "FRAME_.*101,105" debug_bf_pairs.txt > pair_101_105_bf.txt
   grep "FRAME_.*101,105" debug_qt_pairs.txt > pair_101_105_qt.txt
   ```
4. **Create comparison**:
   ```bash
   python3 create_comparison.py > pair_101_105_comparison.txt
   ```
5. **Analyze collisions**:
   ```bash
   grep "105" debug_bf_collisions.txt > line_105_collisions_bf.txt
   grep "105" debug_qt_collisions.txt > line_105_collisions_qt.txt
   ```

## Notes

- All scripts should be run from the project root directory
- Debug scripts require compilation with `-DDEBUG_DISCREPANCY`
- Performance scripts may require specific input files
- Python script requires Python 3

