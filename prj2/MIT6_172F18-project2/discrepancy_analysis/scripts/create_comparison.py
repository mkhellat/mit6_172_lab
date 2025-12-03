#!/usr/bin/env python3
import re

def extract_values(line):
    """Extract frame number and physical quantities from a line"""
    match = re.match(r'FRAME_(\d+): l1_p1=\(([^,]+),([^)]+)\) l1_p2=\(([^,]+),([^)]+)\) l1_vel=\(([^,]+),([^)]+)\) l2_p1=\(([^,]+),([^)]+)\) l2_p2=\(([^,]+),([^)]+)\) l2_vel=\(([^,]+),([^)]+)\) result=(\d+)', line)
    if match:
        frame = int(match.group(1))
        return {
            'frame': frame,
            'l1_p1x': float(match.group(2)),
            'l1_p1y': float(match.group(3)),
            'l1_p2x': float(match.group(4)),
            'l1_p2y': float(match.group(5)),
            'l1_velx': float(match.group(6)),
            'l1_vely': float(match.group(7)),
            'l2_p1x': float(match.group(8)),
            'l2_p1y': float(match.group(9)),
            'l2_p2x': float(match.group(10)),
            'l2_p2y': float(match.group(11)),
            'l2_velx': float(match.group(12)),
            'l2_vely': float(match.group(13)),
            'result': int(match.group(14))
        }
    return None

# Read BF data
bf_data = {}
with open('pair_101_105_bf.txt', 'r') as f:
    for line in f:
        vals = extract_values(line.strip())
        if vals:
            bf_data[vals['frame']] = vals

# Read QT data
qt_data = {}
with open('pair_101_105_qt.txt', 'r') as f:
    for line in f:
        vals = extract_values(line.strip())
        if vals:
            qt_data[vals['frame']] = vals

# Create comparison
print("Frame | BF_l1_p1.x | BF_l1_vel.x | BF_l2_p1.x | BF_l2_vel.x | BF_res | QT_l1_p1.x | QT_l1_vel.x | QT_l2_p1.x | QT_l2_vel.x | QT_res | Diverged")
print("------|------------|-------------|------------|-------------|--------|------------|-------------|------------|-------------|--------|----------")

first_divergence = None
for frame in range(1, 100):
    if frame in bf_data and frame in qt_data:
        bf = bf_data[frame]
        qt = qt_data[frame]
        
        diverged = (abs(bf['l1_p1x'] - qt['l1_p1x']) > 1e-10 or
                   abs(bf['l1_velx'] - qt['l1_velx']) > 1e-10 or
                   abs(bf['l2_p1x'] - qt['l2_p1x']) > 1e-10 or
                   abs(bf['l2_velx'] - qt['l2_velx']) > 1e-10)
        
        if diverged and first_divergence is None:
            first_divergence = frame
        
        div_str = "YES" if diverged else "NO"
        print(f"{frame:5d} | {bf['l1_p1x']:11.10f} | {bf['l1_velx']:12.10f} | {bf['l2_p1x']:11.10f} | {bf['l2_velx']:12.10f} | {bf['result']:6d} | {qt['l1_p1x']:11.10f} | {qt['l1_velx']:12.10f} | {qt['l2_p1x']:11.10f} | {qt['l2_velx']:12.10f} | {qt['result']:6d} | {div_str}")

if first_divergence:
    print(f"\nFirst divergence at frame {first_divergence}")
else:
    print("\nNo divergence found in frames 1-99")
