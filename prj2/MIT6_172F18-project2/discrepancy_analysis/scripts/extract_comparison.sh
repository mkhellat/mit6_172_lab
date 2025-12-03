#!/bin/bash
echo "Frame | BF_l1_p1.x | BF_l1_vel.x | BF_l2_p1.x | BF_l2_vel.x | BF_res | QT_l1_p1.x | QT_l1_vel.x | QT_l2_p1.x | QT_l2_vel.x | QT_res | Diverged"
echo "------|------------|-------------|------------|-------------|--------|------------|-------------|------------|-------------|--------|----------"
for frame in $(seq 1 99); do
  bf=$(grep "^FRAME_${frame}:" pair_101_105_bf.txt | head -1)
  qt=$(grep "^FRAME_${frame}:" pair_101_105_qt.txt | head -1)
  if [ -n "$bf" ] && [ -n "$qt" ]; then
    # Extract values using awk for precision
    bf_l1_p1x=$(echo "$bf" | awk -F'l1_p1=' '{print $2}' | awk -F',' '{print $1}')
    bf_l1_velx=$(echo "$bf" | awk -F'l1_vel=' '{print $2}' | awk -F',' '{print $1}')
    bf_l2_p1x=$(echo "$bf" | awk -F'l2_p1=' '{print $2}' | awk -F',' '{print $1}')
    bf_l2_velx=$(echo "$bf" | awk -F'l2_vel=' '{print $2}' | awk -F',' '{print $1}')
    bf_res=$(echo "$bf" | awk -F'result=' '{print $2}')
    qt_l1_p1x=$(echo "$qt" | awk -F'l1_p1=' '{print $2}' | awk -F',' '{print $1}')
    qt_l1_velx=$(echo "$qt" | awk -F'l1_vel=' '{print $2}' | awk -F',' '{print $1}')
    qt_l2_p1x=$(echo "$qt" | awk -F'l2_p1=' '{print $2}' | awk -F',' '{print $1}')
    qt_l2_velx=$(echo "$qt" | awk -F'l2_vel=' '{print $2}' | awk -F',' '{print $1}')
    qt_res=$(echo "$qt" | awk -F'result=' '{print $2}')
    diverged="NO"
    if [ "$bf_l1_p1x" != "$qt_l1_p1x" ] || [ "$bf_l1_velx" != "$qt_l1_velx" ] || [ "$bf_l2_p1x" != "$qt_l2_p1x" ] || [ "$bf_l2_velx" != "$qt_l2_velx" ]; then
      diverged="YES"
    fi
    printf "%5d | %11s | %12s | %11s | %12s | %6s | %11s | %12s | %11s | %12s | %6s | %s\n" \
      "$frame" "$bf_l1_p1x" "$bf_l1_velx" "$bf_l2_p1x" "$bf_l2_velx" "$bf_res" \
      "$qt_l1_p1x" "$qt_l1_velx" "$qt_l2_p1x" "$qt_l2_velx" "$qt_res" "$diverged"
  fi
done
