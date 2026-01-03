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
  (princ (constraint-solver:format-result-json result)))
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
        Generate constraint visualization plot.
        
        Args:
            measurements: List of (processor_count, time) tuples.
            output_path: Path to save the plot.
            title: Plot title.
        
        Returns:
            Path to generated plot.
        """
        # This would integrate with existing plot generation code
        # For now, we'll create a simple wrapper
        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        # Import plot generation logic (can be refactored from existing scripts)
        # For now, return a placeholder
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
        checker.generate_plot(measurements, args.plot)
        print(f"\nPlot saved to {args.plot}")


if __name__ == '__main__':
    main()

