import matplotlib
matplotlib.use('Agg')  # Use non-interactive backend
import matplotlib.pyplot as plt
import numpy as np

# Create figure
fig, ax = plt.subplots(figsize=(12, 8))

# Define T_infinity range
T_inf = np.linspace(0, 15, 1000)

# Constraint lines from T_4 = 80
T1_T4_work = np.full_like(T_inf, 320)  # T_1 ≤ 320
T1_T4_greedy = 320 - 3 * T_inf  # T_1 ≥ 320 - 3T_∞

# Constraint lines from T_10 = 42
T1_T10_work = np.full_like(T_inf, 420)  # T_1 ≤ 420
T1_T10_greedy = 420 - 9 * T_inf  # T_1 ≥ 420 - 9T_∞

# Constraint lines from T_64 = 10
T1_T64_work = np.full_like(T_inf, 640)  # T_1 ≤ 640
T1_T64_greedy = 640 - 63 * T_inf  # T_1 ≥ 640 - 63T_∞

# Plot constraint lines
ax.plot(T1_T4_work, T_inf, 'b--', label='T₄ Work Law: T₁ ≤ 320', linewidth=2)
ax.plot(T1_T4_greedy, T_inf, 'b-', label='T₄ Greedy Bound: T₁ ≥ 320 - 3T∞', linewidth=2)
ax.plot(T1_T10_work, T_inf, 'g--', label='T₁₀ Work Law: T₁ ≤ 420', linewidth=2)
ax.plot(T1_T10_greedy, T_inf, 'g-', label='T₁₀ Greedy Bound: T₁ ≥ 420 - 9T∞', linewidth=2)
ax.plot(T1_T64_work, T_inf, 'r--', label='T₆₄ Work Law: T₁ ≤ 640', linewidth=2)
ax.plot(T1_T64_greedy, T_inf, 'r-', label='T₆₄ Greedy Bound: T₁ ≥ 640 - 63T∞', linewidth=2)

# Span Law horizontal lines
ax.axhline(y=80, color='b', linestyle=':', alpha=0.5, label='T₄ Span Law: T∞ ≤ 80')
ax.axhline(y=42, color='g', linestyle=':', alpha=0.5, label='T₁₀ Span Law: T∞ ≤ 42')
ax.axhline(y=10, color='r', linestyle=':', linewidth=3, label='T₆₄ Span Law: T∞ ≤ 10 ⭐', alpha=0.7)

# Highlight feasible regions when T∞ ≤ 10
T_inf_constrained = np.linspace(0, 10, 1000)
T1_T4_feasible_low = 320 - 3 * T_inf_constrained
T1_T4_feasible_high = np.full_like(T_inf_constrained, 320)
T1_T10_feasible_low = 420 - 9 * T_inf_constrained
T1_T10_feasible_high = np.full_like(T_inf_constrained, 420)

# Fill feasible regions
ax.fill_betweenx(T_inf_constrained, T1_T4_feasible_low, T1_T4_feasible_high, 
                 where=(T1_T4_feasible_low <= T1_T4_feasible_high),
                 alpha=0.3, color='blue', label='T₄ Feasible Region (T∞ ≤ 10)')
ax.fill_betweenx(T_inf_constrained, T1_T10_feasible_low, T1_T10_feasible_high,
                 where=(T1_T10_feasible_low <= T1_T10_feasible_high),
                 alpha=0.3, color='green', label='T₁₀ Feasible Region (T∞ ≤ 10)')

# Mark the contradiction
ax.axvline(x=290, color='blue', linestyle='--', alpha=0.5)
ax.axvline(x=320, color='blue', linestyle='--', alpha=0.5)
ax.axvline(x=330, color='green', linestyle='--', alpha=0.5)
ax.fill_between([290, 320], 0, 10, alpha=0.2, color='blue', hatch='///')
ax.fill_between([330, 420], 0, 10, alpha=0.2, color='green', hatch='\\\\\\')
ax.text(305, 5, '❌ NO OVERLAP!\nContradiction:\n330 ≤ T₁ ≤ 320', 
        bbox=dict(boxstyle='round', facecolor='red', alpha=0.7, edgecolor='black'),
        fontsize=12, ha='center', color='white', weight='bold')

# Labels and title
ax.set_xlabel('T₁ (Work)', fontsize=14)
ax.set_ylabel('T∞ (Span)', fontsize=14)
ax.set_title('Feasible Regions in (T₁, T∞) Space\nShowing the Contradiction', fontsize=16, weight='bold')
ax.legend(loc='upper right', fontsize=10)
ax.grid(True, alpha=0.3)
ax.set_xlim(0, 700)
ax.set_ylim(0, 20)

plt.tight_layout()
plt.savefig('plots/contradiction_plot_karan.png', dpi=300, bbox_inches='tight')
print("Plot saved to plots/contradiction_plot_karan.png")

