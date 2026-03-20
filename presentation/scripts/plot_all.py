#!/usr/bin/env python3
"""plot_all.py — Generate all 11 publication-quality PDF figures.

Orchestrates:
- Figures 1-9: Delegated to existing QuantLib_huatai/results/plot_figures.py
- Figure 10: Feature matrix heatmap (from feature_matrix.json)
- Figure 11: Backport dependency graph (from dependency_graph.json)

All figures are saved to presentation/figures/.
"""

import shutil
import subprocess
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
PRESENTATION_DIR = SCRIPT_DIR.parent
FIG_DIR = PRESENTATION_DIR / 'figures'
HUATAI_RESULTS_DIR = PRESENTATION_DIR.parent / 'QuantLib_huatai' / 'results'
HUATAI_FIG_DIR = HUATAI_RESULTS_DIR / 'figures'

# Mapping from existing figure filenames to presentation figure filenames
EXISTING_FIGURES = {
    'fig1_cn_oscillations.pdf': 'fig1_cn_oscillations.pdf',
    'fig2_three_scheme_truncated.pdf': 'fig2_three_scheme_truncated.pdf',
    'fig3_barrier_moderate.pdf': 'fig3_barrier_moderate.pdf',
    'fig4_barrier_lowvol.pdf': 'fig4_barrier_lowvol.pdf',
    'fig5_convergence.pdf': 'fig5_convergence.pdf',
    'fig6_effective_diffusion.pdf': 'fig6_effective_diffusion.pdf',
    'fig7_mmatrix.pdf': 'fig7_mmatrix.pdf',
    'fig8_benchmark.pdf': 'fig8_benchmark.pdf',
    'fig9_xcothx.pdf': 'fig9_xcothx.pdf',
}


def generate_existing_figures():
    """Run the existing pipeline to generate figures 1-9."""
    print("Generating figures 1-9 via existing pipeline...")

    # Run existing plot_figures.py
    result = subprocess.run(
        [sys.executable, str(HUATAI_RESULTS_DIR / 'plot_figures.py')],
        cwd=str(HUATAI_RESULTS_DIR),
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        print(f"Error running plot_figures.py:\n{result.stderr}", file=sys.stderr)
        sys.exit(1)
    print(result.stdout, end='')

    # Copy figures to presentation directory
    FIG_DIR.mkdir(parents=True, exist_ok=True)
    for src_name, dst_name in EXISTING_FIGURES.items():
        src = HUATAI_FIG_DIR / src_name
        dst = FIG_DIR / dst_name
        if not src.exists():
            print(f"Error: expected figure {src} not found", file=sys.stderr)
            sys.exit(1)
        shutil.copy2(src, dst)
    print(f"  Copied 9 figures to {FIG_DIR}/")


def generate_new_figures():
    """Generate figures 10-11 from JSON data."""
    print("Generating figures 10-11...")

    for script_name in ['plot_feature_matrix.py', 'plot_dependency_graph.py']:
        script = SCRIPT_DIR / script_name
        result = subprocess.run(
            [sys.executable, str(script)],
            capture_output=True, text=True,
        )
        if result.returncode != 0:
            print(f"Error running {script_name}:\n{result.stderr}",
                  file=sys.stderr)
            sys.exit(1)
        print(result.stdout, end='')


def verify_all_figures():
    """Verify all 11 figure PDFs exist and are non-empty."""
    expected = list(EXISTING_FIGURES.values()) + [
        'fig10_feature_matrix.pdf',
        'fig11_dependency_graph.pdf',
    ]
    missing = []
    for name in expected:
        path = FIG_DIR / name
        if not path.exists() or path.stat().st_size == 0:
            missing.append(name)

    if missing:
        print(f"Error: missing or empty figures: {missing}", file=sys.stderr)
        sys.exit(1)

    print(f"\nAll 11 figures verified in {FIG_DIR}/")


def main():
    generate_existing_figures()
    generate_new_figures()
    verify_all_figures()


if __name__ == '__main__':
    main()
