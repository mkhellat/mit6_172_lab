#!/usr/bin/env python3
"""
Constraint Checker - Python Interface to LISP Constraint Solver

This module provides a Python interface to the LISP constraint solver,
handling measurement validation, constraint derivation, and plot generation.
"""

import subprocess
import json
import sys
import os
from pathlib import Path
from typing import List, Tuple, Dict, Optional
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent.parent / 'scripts'))

class ConstraintChecker:
    """Interface to LISP constraint solver for parallel execution time measurements."""
    
    def __init__(self, lisp_solver_path: Optional[Path] = None):
        """
        Initialize the constraint checker.
        
        Args:
            lisp_solver_path: Path to the LISP constraint solver script.
                            If None, uses default location.
        """
        if lisp_solver_path is None:
            base_dir = Path(__file__).parent.parent
            lisp_solver_path = base_dir / 'lisp' / 'constraint-solver.lisp'
        self.lisp_solver_path = Path(lisp_solver_path)
        
        if not self.lisp_solver_path.exists():
            raise FileNotFoundError(f"LISP solver not found at {lisp_solver_path}")
    
    def check_measurements(self, measurements: List[Tuple[int, float]]) -> Dict:
        """
        Check consistency of parallel execution time measurements.
        
        Args:
            measurements: List of (processor_count, time) tuples.
                         Example: [(4, 80), (10, 42), (64, 9)]
        
        Returns:
            Dictionary with consistency check results.
        """
        # Format measurements for LISP
        lisp_input = self._format_for_lisp(measurements)
        
        # Call LISP solver
        result = self._call_lisp_solver(lisp_input)
        
        return result
    
    def _format_for_lisp(self, measurements: List[Tuple[int, float]]) -> str:
        """Format measurements as LISP list."""
        pairs = ' '.join(f'({p} {t})' for p, t in measurements)
        return f"'({pairs})"
    
    def _call_lisp_solver(self, lisp_input: str) -> Dict:
        """
        Call the LISP constraint solver and parse results.
        
        Args:
            lisp_input: LISP-formatted input string.
        
        Returns:
            Parsed result dictionary.
        """
        # Try SBCL first, then fall back to other Common Lisp implementations
        lisp_impls = ['sbcl', 'clisp', 'ecl']
        lisp_cmd = None
        
        for impl in lisp_impls:
            try:
                if impl == 'ecl':
                    # ECL uses --version differently
                    subprocess.run([impl, '--help'], 
                                  capture_output=True, 
                                  check=True)
                else:
                    subprocess.run([impl, '--version'], 
                                  capture_output=True, 
                                  check=True)
                lisp_cmd = impl
                break
            except (subprocess.CalledProcessError, FileNotFoundError):
                continue
        
        if lisp_cmd is None:
            raise RuntimeError(
                "No Common Lisp implementation found. "
                "Please install SBCL, CLISP, or ECL."
            )
        
        # Create a temporary LISP script that loads and calls the solver
        temp_script = f"""
(load "{self.lisp_solver_path.absolute()}")
(let ((result (constraint-solver:check-measurements {lisp_input})))
  (constraint-solver:format-result-readable result)
  (princ (constraint-solver:format-result-json result))
  (ext:quit))
"""
        
        try:
            if lisp_cmd == 'sbcl':
                process = subprocess.run(
                    [lisp_cmd, '--script', '-'],
                    input=temp_script,
                    text=True,
                    capture_output=True,
                    check=True
                )
            elif lisp_cmd == 'ecl':
                # ECL uses different syntax
                import tempfile
                with tempfile.NamedTemporaryFile(mode='w', suffix='.lisp', delete=False) as f:
                    f.write(temp_script)
                    temp_path = f.name
                
                try:
                    process = subprocess.run(
                        [lisp_cmd, '-load', temp_path],
                        capture_output=True,
                        text=True,
                        check=True
                    )
                finally:
                    os.unlink(temp_path)
            else:
                # For other implementations (CLISP), write to temp file
                import tempfile
                with tempfile.NamedTemporaryFile(mode='w', suffix='.lisp', delete=False) as f:
                    f.write(temp_script)
                    temp_path = f.name
                
                try:
                    process = subprocess.run(
                        [lisp_cmd, temp_path],
                        capture_output=True,
                        text=True,
                        check=True
                    )
                finally:
                    os.unlink(temp_path)
            
            # Parse output (JSON part is at the end, before ECL banner)
            output = process.stdout
            # Extract JSON from output - find the main JSON object
            # Look for the pattern: newline followed by { and "status"
            json_start = output.find('\n{\n  "status"')
            if json_start == -1:
                # Try alternative pattern
                json_start = output.find('{\n  "status"')
            if json_start == -1:
                # Last resort: find any { followed by "status"
                json_start = output.find('{"status"')
            
            if json_start != -1:
                # Find the matching closing brace for the main object
                brace_count = 0
                json_end = json_start
                for i in range(json_start, len(output)):
                    if output[i] == '{':
                        brace_count += 1
                    elif output[i] == '}':
                        brace_count -= 1
                        if brace_count == 0:
                            json_end = i + 1
                            break
                
                json_str = output[json_start:json_end]
                try:
                    return json.loads(json_str)
                except json.JSONDecodeError as e:
                    # If JSON parsing fails, try readable format
                    return self._parse_readable_output(output)
            else:
                # Fallback: parse from readable format
                return self._parse_readable_output(output)
        
        except subprocess.CalledProcessError as e:
            raise RuntimeError(f"LISP solver failed: {e.stderr}")
        except json.JSONDecodeError as e:
            raise RuntimeError(f"Failed to parse LISP output: {e}")
    
    def _parse_readable_output(self, output: str) -> Dict:
        """Parse human-readable output as fallback."""
        lines = output.split('\n')
        status = 'unknown'
        contradiction = None
        
        for line in lines:
            if 'Overall Status:' in line:
                status = line.split(':')[1].strip().lower()
            elif 'Contradiction:' in line:
                contradiction = line.split(':', 1)[1].strip()
        
        return {
            'status': status,
            'contradiction': contradiction
        }
    
    def generate_plot(self, measurements: List[Tuple[int, float]], 
                     output_path: Path, title: str = "Constraint Analysis") -> Path:
        """
        Generate constraint visualization plot for any set of measurements.
        
        This function dynamically generates plots showing:
        - Constraint lines (Work Law, Span Law, Greedy Scheduler Bound)
        - Feasible regions for each measurement
        - Contradictions (non-overlapping regions)
        
        Args:
            measurements: List of (processor_count, time) tuples.
                         Example: [(4, 80), (10, 42), (64, 9)]
            output_path: Path to save the plot.
            title: Plot title.
        
        Returns:
            Path to generated plot.
        """
        # First, check consistency to get constraint information
        result = self.check_measurements(measurements)
        
        # Create output directory if needed
        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        # Color palette for different measurements
        colors = plt.cm.tab10(np.linspace(0, 1, len(measurements)))
        
        # Create figure
        fig, ax = plt.subplots(figsize=(12, 8))
        
        # Derive constraints for each measurement
        constraints = []
        for (p, tp), color in zip(measurements, colors):
            work_law = p * tp  # T1 <= P * TP
            span_law = tp      # T∞ <= TP
            # Greedy bound: T1 >= P * TP - (P-1) * T∞
            constraints.append({
                'p': p,
                'tp': tp,
                'work_law': work_law,
                'span_law': span_law,
                'color': color
            })
        
        # Find tightest span constraint
        tightest_span = min(c['span_law'] for c in constraints)
        
        # Determine axis ranges
        max_work = max(c['work_law'] for c in constraints)
        max_span = max(c['span_law'] for c in constraints)
        
        # T_infinity range for plotting
        T_inf_range = np.linspace(0, min(max_span * 1.2, tightest_span * 3), 1000)
        T_inf_constrained = np.linspace(0, tightest_span, 1000)
        
        # Plot constraint lines for each measurement
        for i, c in enumerate(constraints):
            p, tp = c['p'], c['tp']
            color = c['color']
            work_law = c['work_law']
            
            # Work Law: vertical line at T1 = P * TP
            ax.axvline(x=work_law, color=color, linestyle='--', 
                      linewidth=2, alpha=0.7, 
                      label=f'T_{p} Work Law: T₁ ≤ {work_law:.0f}')
            
            # Span Law: horizontal line at T∞ = TP
            ax.axhline(y=tp, color=color, linestyle=':', 
                      linewidth=2 if tp == tightest_span else 1.5,
                      alpha=0.7,
                      label=f'T_{p} Span Law: T∞ ≤ {tp:.1f}' + 
                            (' (tightest)' if tp == tightest_span else ''))
            
            # Greedy Scheduler Bound: T1 = P * TP - (P-1) * T∞
            greedy_bound = work_law - (p - 1) * T_inf_range
            ax.plot(greedy_bound, T_inf_range, color=color, 
                   linewidth=2, alpha=0.8,
                   label=f'T_{p} Greedy Bound: T₁ ≥ {work_law:.0f} - {p-1}T∞')
            
            # Feasible region when T∞ <= tightest_span
            greedy_at_tightest = work_law - (p - 1) * T_inf_constrained
            work_law_array = np.full_like(T_inf_constrained, work_law)
            
            # Only fill if feasible
            feasible_low = greedy_at_tightest
            feasible_high = work_law_array
            
            ax.fill_betweenx(T_inf_constrained, feasible_low, feasible_high,
                           where=(feasible_low <= feasible_high),
                           alpha=0.2, color=color,
                           label=f'T_{p} Feasible Region (T∞ ≤ {tightest_span:.1f})')
        
        # Check for contradiction and highlight it
        if result.get('status') == 'inconsistent':
            contradiction = result.get('contradiction', '')
            # Try to extract min and max T1 from contradiction string
            import re
            match = re.search(r'([\d.]+)\s*<=\s*T1\s*<=\s*([\d.]+)', contradiction)
            if match:
                min_t1 = float(match.group(1))
                max_t1 = float(match.group(2))
                
                # Highlight the contradiction
                ax.axvline(x=min_t1, color='red', linestyle='--', 
                         linewidth=2, alpha=0.7)
                ax.axvline(x=max_t1, color='red', linestyle='--', 
                         linewidth=2, alpha=0.7)
                
                # Add contradiction annotation
                mid_t1 = (min_t1 + max_t1) / 2
                ax.text(mid_t1, tightest_span / 2,
                       f'NO OVERLAP!\nContradiction:\n{min_t1:.0f} <= T1 <= {max_t1:.0f}',
                       bbox=dict(boxstyle='round', facecolor='red', 
                               alpha=0.7, edgecolor='black'),
                       fontsize=12, ha='center', color='white', weight='bold')
        
        # Labels and title
        ax.set_xlabel('T₁ (Work)', fontsize=14)
        ax.set_ylabel('T∞ (Span)', fontsize=14)
        ax.set_title(f'{title}\nFeasible Regions in (T₁, T∞) Space', 
                    fontsize=16, weight='bold')
        ax.legend(loc='upper right', fontsize=9, ncol=1)
        ax.grid(True, alpha=0.3)
        
        # Set axis limits with some padding
        ax.set_xlim(0, max_work * 1.1)
        ax.set_ylim(0, min(max_span * 1.2, tightest_span * 3))
        
        plt.tight_layout()
        plt.savefig(output_path, dpi=300, bbox_inches='tight')
        plt.close()
        
        return output_path


def main():
    """CLI entry point."""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Check consistency of parallel execution time measurements'
    )
    parser.add_argument('measurements', nargs='+', 
                       help='Measurements as "P:T" pairs (e.g., "4:80" "10:42" "64:9")')
    parser.add_argument('--plot', type=Path, 
                       help='Generate plot and save to this path')
    parser.add_argument('--json', action='store_true',
                       help='Output results as JSON')
    
    args = parser.parse_args()
    
    # Parse measurements
    measurements = []
    for m in args.measurements:
        p, t = m.split(':')
        measurements.append((int(p), float(t)))
    
    # Check measurements
    checker = ConstraintChecker()
    result = checker.check_measurements(measurements)
    
    # Output results
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print(f"\n=== Measurement Consistency Check ===")
        print(f"Status: {result.get('status', 'unknown')}")
        if result.get('contradiction'):
            print(f"Contradiction: {result['contradiction']}")
        if result.get('pairwise'):
            print("\nPairwise Compatibility:")
            for p in result['pairwise']:
                pair_str = f"{p['pair']}"
                compat = "COMPATIBLE" if p.get('compatible') else "INCOMPATIBLE"
                print(f"  {pair_str}: {compat}")
    
    # Generate plot if requested
    if args.plot:
        try:
            plot_path = checker.generate_plot(measurements, args.plot, 
                                            title="Constraint Analysis")
            print(f"\nPlot saved to {plot_path}")
        except Exception as e:
            print(f"\nError generating plot: {e}", file=sys.stderr)
            sys.exit(1)


if __name__ == '__main__':
    main()

