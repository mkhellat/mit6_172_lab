import matplotlib
matplotlib.use('Agg')  # Use non-interactive backend
import matplotlib.pyplot as plt
import numpy as np

# ============================================================================
# Plot 1: T_4 and T_10 (Compatible)
# ============================================================================
fig1, ax1 = plt.subplots(figsize=(12, 8))

T_inf = np.linspace(0, 50, 1000)

# T_4 constraints
T1_T4_work = np.full_like(T_inf, 320)
T1_T4_greedy = 320 - 3 * T_inf

# T_10 constraints
T1_T10_work = np.full_like(T_inf, 420)
T1_T10_greedy = 420 - 9 * T_inf

# Plot constraint lines
ax1.plot(T1_T4_work, T_inf, 'b--', label='T₄ Work Law: T₁ ≤ 320', linewidth=2)
ax1.plot(T1_T4_greedy, T_inf, 'b-', label='T₄ Greedy Bound: T₁ ≥ 320 - 3T∞', linewidth=2)
ax1.plot(T1_T10_work, T_inf, 'g--', label='T₁₀ Work Law: T₁ ≤ 420', linewidth=2)
ax1.plot(T1_T10_greedy, T_inf, 'g-', label='T₁₀ Greedy Bound: T₁ ≥ 420 - 9T∞', linewidth=2)

# Span Law constraints
ax1.axhline(y=80, color='b', linestyle=':', alpha=0.5, label='T₄ Span Law: T∞ ≤ 80')
ax1.axhline(y=42, color='g', linestyle=':', linewidth=2, label='T₁₀ Span Law: T∞ ≤ 42 ⭐', alpha=0.7)

# Feasible region when T∞ ≤ 42 (tightest from T_10)
T_inf_constrained = np.linspace(0, 42, 1000)
T1_T4_feasible_low = 320 - 3 * T_inf_constrained
T1_T4_feasible_high = np.full_like(T_inf_constrained, 320)
T1_T10_feasible_low = 420 - 9 * T_inf_constrained
T1_T10_feasible_high = np.full_like(T_inf_constrained, 420)

# Find intersection
# T_4: T₁ ≥ 320 - 3T∞, T₁ ≤ 320
# T_10: T₁ ≥ 420 - 9T∞, T₁ ≤ 420
# When T∞ ≤ 42: T₁ ≥ max(320-3T∞, 420-9T∞), T₁ ≤ 320
# At T∞ = 42: T₁ ≥ max(194, 42) = 194, T₁ ≤ 320
# So: 194 ≤ T₁ ≤ 320 is feasible

ax1.fill_betweenx(T_inf_constrained, 
                 np.maximum(T1_T4_feasible_low, T1_T10_feasible_low),
                 T1_T4_feasible_high,
                 where=(np.maximum(T1_T4_feasible_low, T1_T10_feasible_low) <= T1_T4_feasible_high),
                 alpha=0.3, color='purple', label='Feasible Region: 194 ≤ T₁ ≤ 320 (when T∞ ≤ 42)')

ax1.axvline(x=194, color='purple', linestyle='--', alpha=0.5)
ax1.axvline(x=320, color='purple', linestyle='--', alpha=0.5)
ax1.text(257, 21, '✓ COMPATIBLE\n194 ≤ T₁ ≤ 320\nwhen T∞ ≤ 42', 
        bbox=dict(boxstyle='round', facecolor='green', alpha=0.7, edgecolor='black'),
        fontsize=12, ha='center', color='white', weight='bold')

ax1.set_xlabel('T₁ (Work)', fontsize=14)
ax1.set_ylabel('T∞ (Span)', fontsize=14)
ax1.set_title('T₄ and T₁₀: Compatible Measurements', fontsize=16, weight='bold')
ax1.legend(loc='upper right', fontsize=10)
ax1.grid(True, alpha=0.3)
ax1.set_xlim(0, 500)
ax1.set_ylim(0, 50)

plt.tight_layout()
plt.savefig('plots/t4_t10_compatible.png', dpi=300, bbox_inches='tight')
print("Plot 1 saved: plots/t4_t10_compatible.png")

# ============================================================================
# Plot 2: T_4 and T_64 (Check compatibility)
# ============================================================================
fig2, ax2 = plt.subplots(figsize=(12, 8))

T_inf = np.linspace(0, 15, 1000)

# T_4 constraints
T1_T4_work = np.full_like(T_inf, 320)
T1_T4_greedy = 320 - 3 * T_inf

# T_64 constraints
T1_T64_work = np.full_like(T_inf, 576)
T1_T64_greedy = 576 - 63 * T_inf

# Plot constraint lines
ax2.plot(T1_T4_work, T_inf, 'b--', label='T₄ Work Law: T₁ ≤ 320', linewidth=2)
ax2.plot(T1_T4_greedy, T_inf, 'b-', label='T₄ Greedy Bound: T₁ ≥ 320 - 3T∞', linewidth=2)
ax2.plot(T1_T64_work, T_inf, 'r--', label='T₆₄ Work Law: T₁ ≤ 576', linewidth=2)
ax2.plot(T1_T64_greedy, T_inf, 'r-', label='T₆₄ Greedy Bound: T₁ ≥ 576 - 63T∞', linewidth=2)

# Span Law constraints
ax2.axhline(y=80, color='b', linestyle=':', alpha=0.5, label='T₄ Span Law: T∞ ≤ 80')
ax2.axhline(y=9, color='r', linestyle=':', linewidth=3, label='T₆₄ Span Law: T∞ ≤ 9 ⭐', alpha=0.7)

# Feasible region when T∞ ≤ 9 (tightest from T_64)
T_inf_constrained = np.linspace(0, 9, 1000)
T1_T4_feasible_low = 320 - 3 * T_inf_constrained
T1_T4_feasible_high = np.full_like(T_inf_constrained, 320)
T1_T64_feasible_low = 576 - 63 * T_inf_constrained
T1_T64_feasible_high = np.full_like(T_inf_constrained, 576)

# Check: T₁ ≥ max(320-3T∞, 576-63T∞), T₁ ≤ 320
# At T∞ = 9: T₁ ≥ max(293, -9) = 293, T₁ ≤ 320
# So: 293 ≤ T₁ ≤ 320 is feasible

ax2.fill_betweenx(T_inf_constrained,
                 np.maximum(T1_T4_feasible_low, T1_T64_feasible_low),
                 T1_T4_feasible_high,
                 where=(np.maximum(T1_T4_feasible_low, T1_T64_feasible_low) <= T1_T4_feasible_high),
                 alpha=0.3, color='purple', label='Feasible Region: 293 ≤ T₁ ≤ 320 (when T∞ ≤ 9)')

ax2.axvline(x=293, color='purple', linestyle='--', alpha=0.5)
ax2.axvline(x=320, color='purple', linestyle='--', alpha=0.5)
ax2.text(306.5, 4.5, '✓ COMPATIBLE\n293 ≤ T₁ ≤ 320\nwhen T∞ ≤ 9', 
        bbox=dict(boxstyle='round', facecolor='green', alpha=0.7, edgecolor='black'),
        fontsize=12, ha='center', color='white', weight='bold')

ax2.set_xlabel('T₁ (Work)', fontsize=14)
ax2.set_ylabel('T∞ (Span)', fontsize=14)
ax2.set_title('T₄ and T₆₄: Compatible Measurements', fontsize=16, weight='bold')
ax2.legend(loc='upper right', fontsize=10)
ax2.grid(True, alpha=0.3)
ax2.set_xlim(0, 600)
ax2.set_ylim(0, 20)

plt.tight_layout()
plt.savefig('plots/t4_t64_compatible.png', dpi=300, bbox_inches='tight')
print("Plot 2 saved: plots/t4_t64_compatible.png")

# ============================================================================
# Plot 3: T_10 and T_64 (Check compatibility)
# ============================================================================
fig3, ax3 = plt.subplots(figsize=(12, 8))

T_inf = np.linspace(0, 15, 1000)

# T_10 constraints
T1_T10_work = np.full_like(T_inf, 420)
T1_T10_greedy = 420 - 9 * T_inf

# T_64 constraints
T1_T64_work = np.full_like(T_inf, 576)
T1_T64_greedy = 576 - 63 * T_inf

# Plot constraint lines
ax3.plot(T1_T10_work, T_inf, 'g--', label='T₁₀ Work Law: T₁ ≤ 420', linewidth=2)
ax3.plot(T1_T10_greedy, T_inf, 'g-', label='T₁₀ Greedy Bound: T₁ ≥ 420 - 9T∞', linewidth=2)
ax3.plot(T1_T64_work, T_inf, 'r--', label='T₆₄ Work Law: T₁ ≤ 576', linewidth=2)
ax3.plot(T1_T64_greedy, T_inf, 'r-', label='T₆₄ Greedy Bound: T₁ ≥ 576 - 63T∞', linewidth=2)

# Span Law constraints
ax3.axhline(y=42, color='g', linestyle=':', alpha=0.5, label='T₁₀ Span Law: T∞ ≤ 42')
ax3.axhline(y=9, color='r', linestyle=':', linewidth=3, label='T₆₄ Span Law: T∞ ≤ 9 ⭐', alpha=0.7)

# Feasible region when T∞ ≤ 9 (tightest from T_64)
T_inf_constrained = np.linspace(0, 9, 1000)
T1_T10_feasible_low = 420 - 9 * T_inf_constrained
T1_T10_feasible_high = np.full_like(T_inf_constrained, 420)
T1_T64_feasible_low = 576 - 63 * T_inf_constrained
T1_T64_feasible_high = np.full_like(T_inf_constrained, 576)

# Check: T₁ ≥ max(420-9T∞, 576-63T∞), T₁ ≤ 420
# At T∞ = 9: T₁ ≥ max(339, -9) = 339, T₁ ≤ 420
# So: 339 ≤ T₁ ≤ 420 is feasible

ax3.fill_betweenx(T_inf_constrained,
                 np.maximum(T1_T10_feasible_low, T1_T64_feasible_low),
                 T1_T10_feasible_high,
                 where=(np.maximum(T1_T10_feasible_low, T1_T64_feasible_low) <= T1_T10_feasible_high),
                 alpha=0.3, color='purple', label='Feasible Region: 339 ≤ T₁ ≤ 420 (when T∞ ≤ 9)')

ax3.axvline(x=339, color='purple', linestyle='--', alpha=0.5)
ax3.axvline(x=420, color='purple', linestyle='--', alpha=0.5)
ax3.text(379.5, 4.5, '✓ COMPATIBLE\n339 ≤ T₁ ≤ 420\nwhen T∞ ≤ 9', 
        bbox=dict(boxstyle='round', facecolor='green', alpha=0.7, edgecolor='black'),
        fontsize=12, ha='center', color='white', weight='bold')

ax3.set_xlabel('T₁ (Work)', fontsize=14)
ax3.set_ylabel('T∞ (Span)', fontsize=14)
ax3.set_title('T₁₀ and T₆₄: Compatible Measurements', fontsize=16, weight='bold')
ax3.legend(loc='upper right', fontsize=10)
ax3.grid(True, alpha=0.3)
ax3.set_xlim(0, 600)
ax3.set_ylim(0, 20)

plt.tight_layout()
plt.savefig('plots/t10_t64_compatible.png', dpi=300, bbox_inches='tight')
print("Plot 3 saved: plots/t10_t64_compatible.png")

print("\nAll pairwise compatibility plots generated successfully!")

