# Tony Lezard's N-Queens Bit Manipulation Algorithm

> "The best optimizations come from understanding the problem
  structure, not just applying brute force." - Performance Engineering
  Principle

---

## Overview

Tony Lezard's 1991 implementation of the N-Queens problem demonstrates
an interesting optimization: using only **N bits** for each of the
three bit vectors (`row`, `left`, `right`) instead of the traditional
approach that requires **N**, **2N-1**, and **2N-1** bits
respectively. This write-up explains how the algorithm works and why
this optimization is possible.

---

## The Algorithm

Here's the complete implementation (12 lines of code):

```c
void try(int row, int left, int right) {
    int poss, place;
    // If row bitvector is all 1s, a valid ordering of the queens exists.
    if (row == 0xFF) ++count;
    else {
        poss = ~(row | left | right) & 0xFF;
        while (poss != 0) {
            place = poss & -poss;
            try(row | place, (left | place) << 1, (right | place) >> 1);
            poss &= ~place;
        }
    }
}
```

---

## How It Works

### Bit Vector Representation

The algorithm uses three bit vectors to track constraints:

1. **`row`**: Tracks which columns are occupied in the rows processed so far
   - Bit `i` = 1 means column `i` is occupied
   - Size: **N bits** (one per column)

2. **`left`**: Tracks left-diagonal threats for the current row
   - Bit `i` = 1 means column `i` is threatened by a left diagonal
   - Size: **N bits** (one per column in current row)

3. **`right`**: Tracks right-diagonal threats for the current row
   - Bit `i` = 1 means column `i` is threatened by a right diagonal
   - Size: **N bits** (one per column in current row)

#### Visual Example: Initial State (N=8)

```
Chess Board (8×8):
    0   1   2   3   4   5   6   7
  ┌───┬───┬───┬───┬───┬───┬───┬───┐
0 │ · │ · │ · │ · │ · │ · │ · │ · │    row = 0b00000000 (no columns occupied)
  ├───┼───┼───┼───┼───┼───┼───┼───┤
1 │ · │ · │ · │ · │ · │ · │ · │ · │   left = 0b00000000 (no left-diagonal threats)
  ├───┼───┼───┼───┼───┼───┼───┼───┤
2 │ · │ · │ · │ · │ · │ · │ · │ · │  right = 0b00000000 (no right-diagonal threats)
  ├───┼───┼───┼───┼───┼───┼───┼───┤
3 │ · │ · │ · │ · │ · │ · │ · │ · │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
4 │ · │ · │ · │ · │ · │ · │ · │ · │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
5 │ · │ · │ · │ · │ · │ · │ · │ · │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
6 │ · │ · │ · │ · │ · │ · │ · │ · │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
7 │ · │ · │ · │ · │ · │ · │ · │ · │
  └───┴───┴───┴───┴───┴───┴───┴───┘

Legend: · = empty, Q = queen, X = threatened
```

### Step-by-Step Execution

For an 8×8 board (N=8), the algorithm works as follows:

#### 1. Initialization: `try(0, 0, 0)`

```
    0   1   2   3   4   5   6   7
  ┌───┬───┬───┬───┬───┬───┬───┬───┐
0 │ · │ · │ · │ · │ · │ · │ · │ · │    row = 0b00000000
  └───┴───┴───┴───┴───┴───┴───┴───┘   left = 0b00000000
                                     right = 0b00000000
```

No queens placed, no threats. All positions in row 0 are valid.

#### 2. Finding Valid Positions

```c
poss = ~(row | left | right) & 0xFF;
```

This computes positions that are **NOT** threatened by:
- Any row (vertical threat from queens above)
- Any left diagonal (threats from top-left)
- Any right diagonal (threats from top-right)

**Example**: After placing queens in rows 0-2, computing valid
  positions for row 3:

```
    0   1   2   3   4   5   6   7
  ┌───┬───┬───┬───┬───┬───┬───┬───┐
0 │ Q │ · │ · │ · │ · │ · │ · │ · │  Queen at (0,0)
  ├───┼───┼───┼───┼───┼───┼───┼───┤
1 │ · │ · │ Q │ · │ · │ · │ · │ · │  Queen at (1,2)
  ├───┼───┼───┼───┼───┼───┼───┼───┤
2 │ · │ · │ · │ · │ Q │ · │ · │ · │  Queen at (2,4)
  ├───┼───┼───┼───┼───┼───┼───┼───┤
3 │ X │ · │ X │ X │ X │ X │ · │ · │  Computing poss for row 3
  └───┴───┴───┴───┴───┴───┴───┴───┘

Bit breakdown for row 3:
  row  = 0b00010101  (cols 0, 2, 4 occupied by queens above)
  left = 0b00111000  (cols 3, 4, 5 threatened by left diagonals)
                     (from (0,0)→(3,3), (1,2)→(3,4), (2,4)→(3,5))
  right= 0b00001001  (cols 0, 3 threatened by right diagonals)
                     (from (1,2)→(3,0), (2,4)→(3,3))
  ─────────────────────────────────────────────────────────────
  row|left|right = 0b00111111  (all threats combined)
  poss = ~(row|left|right) & 0xFF = 0b11000010
                                      (cols 1, 6, 7 are valid!)
```

**Visual breakdown of threats**:
- **Column 0**: Threatened by row (queen at (0,0)) and right diagonal from (1,2)→(3,0)
- **Column 1**: **FREE** → Valid position!
- **Column 2**: Threatened by row (queen at (1,2))
- **Column 3**: Threatened by left diagonal from (0,0)→(3,3) and right diagonal from (2,4)→(3,3)
- **Column 4**: Threatened by row (queen at (2,4)) and left diagonal from (1,2)→(3,4)
- **Column 5**: Threatened by left diagonal from (2,4)→(3,5)
- **Column 6**: **FREE** → Valid position!
- **Column 7**: **FREE** → Valid position!

#### 3. Placing a Queen

```c
place = poss & -poss;  // Extract least significant set bit
```

This bit hack extracts the rightmost 1-bit (LSB) efficiently.

**Example** (continuing from the previous example where `poss = 0b11000010`):
```
 poss = 0b11000010  (columns 1, 6, and 7 are valid)
-poss = 0b00111110  (two's complement: flip bits and add 1)
place = 0b11000010 & 0b00111110 = 0b00000010  (column 1 selected - the LSB)
```

**Visual Example**: From the earlier example, selecting column 1

```
    0   1   2   3   4   5   6   7
  ┌───┬───┬───┬───┬───┬───┬───┬───┐
0 │ Q │ · │ · │ · │ · │ · │ · │ · │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
1 │ · │ · │ Q │ · │ · │ · │ · │ · │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
2 │ · │ · │ · │ · │ Q │ · │ · │ · │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
3 │ X │ · │ X │ X │ X │ X │ · │ · │   poss = 0b11000010 (cols 1,6,7 valid)
  └───┴───┴───┴───┴───┴───┴───┴───┘  place = 0b00000010 (col 1 selected)
```

**Another Example**: Placing first queen at column 0 (initial state)

```
    0   1   2   3   4   5   6   7
  ┌───┬───┬───┬───┬───┬───┬───┬───┐
0 │ Q │ · │ · │ · │ · │ · │ · │ · │   poss = 0b11111111 (all cols valid)
  └───┴───┴───┴───┴───┴───┴───┴───┘  place = 0b00000001 (col 0 selected - LSB)
```

#### 4. Recursive Call with Threat Propagation

```c
try(row | place, (left | place) << 1, (right | place) >> 1);
```

**Visual Example**: After placing queen at (0,0), moving to row 1

```
Before recursive call (row 0):
    row = 0b00000000
   left = 0b00000000
  right = 0b00000000
  place = 0b00000001 (column 0)

After computing new values:
  row | place = 0b00000001  (mark column 0 as occupied)
  (left | place) << 1 = 0b00000010  (left diagonal threat moves to col 1)
  (right | place) >> 1 = 0b00000000 (right diagonal threat moves off board)

Chess Board State:
    0   1   2   3   4   5   6   7
  ┌───┬───┬───┬───┬───┬───┬───┬───┐
0 │ Q │ · │ · │ · │ · │ · │ · │ · │    row = 0b00000001
  ├───┼───┼───┼───┼───┼───┼───┼───┤
1 │ X │ X │ · │ · │ · │ · │ · │ · │   left = 0b00000010 (col 1 threatened)
  └───┴───┴───┴───┴───┴───┴───┴───┘  right = 0b00000000 (no right threats)
                                      poss = 0b11111100 (cols 2-7 are valid)
```

**Key Insight**: The queen at (0,0) threatens:
- Column 0 (vertical) → tracked in `row`
- Left diagonal through (1,1) → tracked in `left` (shifted left)
- Right diagonal through (1,-1) → off board, discarded (shifted right)

#### 5. Backtracking

```c
poss &= ~place;  // Remove this position from possibilities
```

After trying a position, remove it and try the next valid position.

#### 6. Termination

```c
if (row == 0xFF) ++count;
```

When `row` has all N bits set (0xFF = 0b11111111 for N=8), all N
queens are placed successfully.

**Example of a Complete Solution**:

```
    0   1   2   3   4   5   6   7
  ┌───┬───┬───┬───┬───┬───┬───┬───┐
0 │ Q │ · │ · │ · │ · │ · │ · │ · │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
1 │ · │ · │ · │ · │ Q │ · │ · │ · │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
2 │ · │ · │ · │ · │ · │ · │ · │ Q │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
3 │ · │ · │ · │ · │ · │ Q │ · │ · │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
4 │ · │ · │ Q │ · │ · │ · │ · │ · │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
5 │ · │ · │ · │ · │ · │ · │ Q │ · │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
6 │ · │ Q │ · │ · │ · │ · │ · │ · │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
7 │ · │ · │ · │ Q │ · │ · │ · │ · │
  └───┴───┴───┴───┴───┴───┴───┴───┘

row = 0b11111111 = 0xFF → Solution found! (count++)
```

---

## Why Only N Bits for Each Vector?

This is the key insight that makes Tony Lezard's algorithm interesting.

### Traditional Approach

The traditional representation uses:
- **`row`**: N bits (columns occupied)
- **`left`**: 2N-1 bits (all possible left diagonals across the board)
- **`right`**: 2N-1 bits (all possible right diagonals across the board)

For an 8×8 board, this requires tracking 15 different left diagonals
and 15 different right diagonals.

### Tony Lezard's Insight

**Key Observation**: Since we process the board **row by row** (top to
  bottom), we only need to track diagonal threats that affect **the
  current row**, not all diagonals across the entire board.

#### Left Diagonals

When a queen is placed at column `i` in row `r`:
- It threatens column `i+1` in row `r+1` (diagonal moves down-right)
- It threatens column `i+2` in row `r+2`
- And so on...

**The Shift Operation**: `(left | place) << 1`
- When we shift left by 1, the threat at column `i` moves to column
  `i+1` in the next row
- Since we're processing one row at a time, we only need to track the
  N columns that are relevant for the current row
- Bits that shift out of the N-bit range are irrelevant (they
  represent diagonals that have already passed through the board)

**Visual Example** (N=8): Left Diagonal Propagation

```
Step 1: Place queen at row 0, column 2
    0   1   2   3   4   5   6   7
  ┌───┬───┬───┬───┬───┬───┬───┬───┐
0 │ · │ · │ Q │ · │ · │ · │ · │ · │  place = 0b00000100 (col 2)
  └───┴───┴───┴───┴───┴───┴───┴───┘  Compute: (left | place) << 1
                                      = (0 | 0b00000100) << 1
                                      = 0b00001000 (col 3 threatened in row 1)

Step 2: In row 1, the threat propagates
    0   1   2   3   4   5   6   7
  ┌───┬───┬───┬───┬───┬───┬───┬───┐
0 │ · │ · │ Q │ · │ · │ · │ · │ · │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
1 │ · │ · │ · │ X │ · │ · │ · │ · │  left = 0b00001000 (col 3 threatened)
  └───┴───┴───┴───┴───┴───┴───┴───┘  The diagonal from (0,2) → (1,3) is tracked!
```

**Why the shift works**:
- Left diagonals go down-right: (row, col) → (row+1, col+1)
- Shifting left by 1 bit moves the threat from column `i` to column `i+1`
- This matches the diagonal movement pattern

#### Right Diagonals

Similarly, when a queen is placed at column `i` in row `r`:
- It threatens column `i-1` in row `r+1` (diagonal moves down-left)
- It threatens column `i-2` in row `r+2`
- And so on...

**The Shift Operation**: `(right | place) >> 1`
- When we shift right by 1, the threat at column `i` moves to column `i-1` in the next row
- Again, we only need N bits because we only care about threats in the current row
- Bits that shift out are irrelevant (they represent diagonals that have already passed)

**Visual Example** (N=8): Right Diagonal Propagation

```
Step 1: Place queen at row 0, column 5
    0   1   2   3   4   5   6   7
  ┌───┬───┬───┬───┬───┬───┬───┬───┐
0 │ · │ · │ · │ · │ · │ Q │ · │ · │  place = 0b00100000 (col 5)
  └───┴───┴───┴───┴───┴───┴───┴───┘  Compute: (right | place) >> 1
                                      = (0 | 0b00100000) >> 1
                                      = 0b00010000 (col 4 threatened in row 1)

Step 2: In row 1, the threat propagates
    0   1   2   3   4   5   6   7
  ┌───┬───┬───┬───┬───┬───┬───┬───┐
0 │ · │ · │ · │ · │ · │ Q │ · │ · │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
1 │ · │ · │ · │ · │ X │ · │ · │ · │  right = 0b00010000 (col 4 threatened)
  └───┴───┴───┴───┴───┴───┴───┴───┘  The diagonal from (0,5) → (1,4) is tracked!
```

**Why the shift works**:
- Right diagonals go down-left: (row, col) → (row+1, col-1)
- Shifting right by 1 bit moves the threat from column `i` to column `i-1`
- This matches the diagonal movement pattern

### Visual Illustration: Why N Bits Suffice

For an 8×8 board, here's why N bits suffice:

#### Traditional View (2N-1 diagonals)

```
All left diagonals (15 total):
    0   1   2   3   4   5   6   7
  ┌───┬───┬───┬───┬───┬───┬───┬───┐
0 │/0 │/1 │/2 │/3 │/4 │/5 │/6 │/7 │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
1 │/1 │/2 │/3 │/4 │/5 │/6 │/7 │/8 │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
2 │/2 │/3 │/4 │/5 │/6 │/7 │/8 │/9 │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
3 │/3 │/4 │/5 │/6 │/7 │/8 │/9 │/10│
  ├───┼───┼───┼───┼───┼───┼───┼───┤
4 │/4 │/5 │/6 │/7 │/8 │/9 │/10│/11│
  ├───┼───┼───┼───┼───┼───┼───┼───┤
5 │/5 │/6 │/7 │/8 │/9 │/10│/11│/12│
  ├───┼───┼───┼───┼───┼───┼───┼───┤
6 │/6 │/7 │/8 │/9 │/10│/11│/12│/13│
  ├───┼───┼───┼───┼───┼───┼───┼───┤
7 │/7 │/8 │/9 │/10│/11│/12│/13│/14│
  └───┴───┴───┴───┴───┴───┴───┴───┘
  (15 unique left diagonals: /0 through /14)

All right diagonals (15 total):
    0   1   2   3   4   5   6   7
  ┌───┬───┬───┬───┬───┬───┬───┬───┐
0 │\7 │\8 │\9 │\10│\11│\12│\13│\14│
  ├───┼───┼───┼───┼───┼───┼───┼───┤
1 │\6 │\7 │\8 │\9 │\10│\11│\12│\13│
  ├───┼───┼───┼───┼───┼───┼───┼───┤
2 │\5 │\6 │\7 │\8 │\9 │\10│\11│\12│
  ├───┼───┼───┼───┼───┼───┼───┼───┤
3 │\4 │\5 │\6 │\7 │\8 │\9 │\10│\11│
  ├───┼───┼───┼───┼───┼───┼───┼───┤
4 │\3 │\4 │\5 │\6 │\7 │\8 │\9 │\10│
  ├───┼───┼───┼───┼───┼───┼───┼───┤
5 │\2 │\3 │\4 │\5 │\6 │\7 │\8 │\9 │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
6 │\1 │\2 │\3 │\4 │\5 │\6 │\7 │\8 │
  ├───┼───┼───┼───┼───┼───┼───┼───┤
7 │\0 │\1 │\2 │\3 │\4 │\5 │\6 │\7 │
  └───┴───┴───┴───┴───┴───┴───┴───┘
  (15 unique right diagonals: \0 through \14)
```

#### Tony Lezard's View (N bits per row)

**Key Insight**: At each row, we only need to track which of the N
  columns are threatened, not all 15 diagonals!

```
Processing Row 2:
    0   1   2   3   4   5   6   7
  ┌───┬───┬───┬───┬───┬───┬───┬───┐
0 │ Q │ · │ · │ · │ · │ · │ · │ · │  (queen at col 0)
  ├───┼───┼───┼───┼───┼───┼───┼───┤
1 │ · │ · │ Q │ · │ · │ · │ · │ · │  (queen at col 2)
  ├───┼───┼───┼───┼───┼───┼───┼───┤
2 │ X │ X │ X │ X │ · │ · │ · │ · │   left = 0b00001100 (cols 2,3 threatened)
  └───┴───┴───┴───┴───┴───┴───┴───┘  right = 0b00000010 (col 1 threatened)
                                       row = 0b00000101 (cols 0,2 occupied)
                                      ─────────────────
                                      Only 8 bits needed!

What we track:
  - Column 0: threatened by row (queen above)
  - Column 1: threatened by right diagonal from (1,2)
  - Column 2: threatened by row (queen above) and left diagonal from (0,0)
  - Column 3: threatened by left diagonal from (1,2)
  - Columns 4-7: free!

We don't need to track:
  - Diagonal /0 (only affects row 0)
  - Diagonal /14 (only affects row 7)
  - Diagonals that have already passed through
```

**Why this works**: The shift operations automatically:
1. **Propagate** threats from previous rows to the current row
2. **Discard** threats that have moved beyond the board (shifted out of N-bit range)
3. **Track** only the N columns relevant to the current row

At each row, we only need to know which of the N columns are
threatened. The shift operations automatically propagate diagonal
threats correctly to the next row, and bits that fall outside the
N-bit range represent diagonals that no longer affect future rows.

### Mathematical Justification

For a board of size N×N:
- **Total left diagonals**: 2N-1 (from top-left to bottom-right)
- **Total right diagonals**: 2N-1 (from top-right to bottom-left)

However, at any given row `r`:
- **Relevant left diagonals**: Only those that intersect row `r`,
    which is exactly N diagonals
- **Relevant right diagonals**: Only those that intersect row `r`,
    which is exactly N diagonals

The shift operations (`<< 1` and `>> 1`) ensure that:
1. Diagonal threats from previous rows are correctly propagated
2. Threats that have moved beyond the board boundaries are
automatically discarded (shifted out of the N-bit range)
3. Only the N columns of the current row need to be tracked

---

## Bit Manipulation Tricks

### `poss & -poss`: Extract Least Significant Set Bit

This is a classic bit manipulation trick:
- `-poss` computes the two's complement (flips all bits and adds 1)
- `poss & -poss` isolates the rightmost 1-bit

**Example**:
```c
 poss = 0b11000010  (columns 1, 6, and 7 are valid)
-poss = 0b00111110  (two's complement: flip bits and add 1)
place = 0b11000010 & 0b00111110 = 0b00000010  (column 1 selected - LSB)
```

**Visual Example**:

```
Chess Board:
    0   1   2   3   4   5   6   7
  ┌───┬───┬───┬───┬───┬───┬───┬───┐
3 │ X │ · │ X │ X │ X │ X │ · │ · │  poss = 0b11000010
  └───┴───┴───┴───┴───┴───┴───┴───┘         (cols 1, 6, 7 valid)
                                          place = 0b00000010
                                          (select col 1 - LSB)

Bit manipulation:
  poss    = 0b11000010  (binary: cols 1, 6, 7 valid)
  -poss   = 0b00111110  (two's complement: flip bits, add 1)
  place   = 0b00000010  (isolate rightmost 1-bit = col 1)
```

This efficiently finds the next column to try without iterating
through all bits.

### `poss &= ~place`: Remove Tried Position

```c
poss &= ~place;  // Clear the bit we just tried
```

This removes the current position from future consideration in the
backtracking loop.

---

## Performance Characteristics

- **Space Complexity**: O(N) bits instead of O(N) for traditional
    approach
- **Time Complexity**: Still exponential (backtracking), but with
    reduced memory overhead
- **Practical Benefit**: For N=8, uses 8 bits instead of 15 bits per
    diagonal vector (47% reduction)

