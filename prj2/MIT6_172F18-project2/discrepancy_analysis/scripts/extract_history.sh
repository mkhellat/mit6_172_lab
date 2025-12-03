#!/bin/bash
# Extract history for pair (101,105) and create comparison

echo "=== BF History of (101,105) ===" > pair_101_105_bf_history.txt
cat pair_101_105_bf.txt >> pair_101_105_bf_history.txt

echo "" >> pair_101_105_bf_history.txt
echo "=== QT History of (101,105) ===" > pair_101_105_qt_history.txt
cat pair_101_105_qt.txt >> pair_101_105_qt_history.txt

# Create comparison table
echo "Frame | BF_l1_p1.x | BF_l1_vel.x | BF_l2_p1.x | BF_l2_vel.x | BF_res | QT_l1_p1.x | QT_l1_vel.x | QT_l2_p1.x | QT_l2_vel.x | QT_res | Diverged" > pair_101_105_comparison.txt
echo "------|------------|-------------|------------|-------------|--------|------------|-------------|------------|-------------|--------|----------" >> pair_101_105_comparison.txt

# Process each frame
for frame in $(seq 1 99); do
  bf_line=$(grep "^FRAME_${frame}:" pair_101_105_bf.txt | head -1)
  qt_line=$(grep "^FRAME_${frame}:" pair_101_105_qt.txt | head -1)
  
  if [ -n "$bf_line" ] && [ -n "$qt_line" ]; then
    # Extract values using awk
    bf_l1_p1x=$(echo "$bf_line" | awk -F'l1_p1=(' '{print $2}' | awk -F',' '{print $1}')
    bf_l1_velx=$(echo "$bf_line" | awk -F'l1_vel=(' '{print $2}' | awk -F',' '{print $1}')
    bf_l2_p1x=$(echo "$bf_line" | awk -F'l2_p1=(' '{print $2}' | awk -F',' '{print $1}')
    bf_l2_velx=$(echo "$bf_line" | awk -F'l2_vel=(' '{print $2}' | awk -F',' '{print $1}')
    bf_res=$(echo "$bf_line" | awk -F'result=' '{print $2}')
    
    qt_l1_p1x=$(echo "$qt_line" | awk -F'l1_p1=(' '{print $2}' | awk -F',' '{print $1}')
    qt_l1_velx=$(echo "$qt_line" | awk -F'l1_vel=(' '{print $2}' | awk -F',' '{print $1}')
    qt_l2_p1x=$(echo "$qt_line" | awk -F'l2_p1=(' '{print $2}' | awk -F',' '{print $1}')
    qt_l2_velx=$(echo "$qt_line" | awk -F'l2_vel=(' '{print $2}' | awk -F',' '{print $1}')
    qt_res=$(echo "$qt_line" | awk -F'result=' '{print $2}')
    
    diverged="NO"
    if [ "$bf_l1_p1x" != "$qt_l1_p1x" ] || [ "$bf_l1_velx" != "$qt_l1_velx" ] || [ "$bf_l2_p1x" != "$qt_l2_p1x" ] || [ "$bf_l2_velx" != "$qt_l2_velx" ]; then
      diverged="YES"
    fi
    
    printf "%5d | %11s | %12s | %11s | %12s | %6s | %11s | %12s | %11s | %12s | %6s | %s\n" \
      "$frame" "$bf_l1_p1x" "$bf_l1_velx" "$bf_l2_p1x" "$bf_l2_velx" "$bf_res" \
      "$qt_l1_p1x" "$qt_l1_velx" "$qt_l2_p1x" "$qt_l2_velx" "$qt_res" "$diverged" >> pair_101_105_comparison.txt
  fi
done

echo "Comparison created: pair_101_105_comparison.txt"
