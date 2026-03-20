"""Shared plotting configuration for the academic presentation.

Imports style constants from the existing QuantLib_huatai plot_figures.py
to maintain visual consistency across all 11 figures.
"""

import sys
from pathlib import Path

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

# Add the existing results directory to import path
_RESULTS_DIR = Path(__file__).resolve().parent.parent.parent / 'QuantLib_huatai' / 'results'
sys.path.insert(0, str(_RESULTS_DIR))

# Import style constants from existing pipeline
from plot_figures import (
    COLORS, STYLES, LABELS, SCHEME_ORDER,
    read_csv, validate_meta, to_float_arrays, save_pdf,
    REQUIRED_META
)

# Extra colors for infrastructure figures (not in the upstream palette)
HEATMAP_COLORS = {
    'absent': '#D55E00',
    'partial': '#E69F00',
    'native': '#009E73',
}

NODE_COLORS = {
    'new': '#009E73',
    'adapted': '#D55E00',
}

# Re-export everything for convenient import by other scripts
__all__ = [
    'COLORS', 'STYLES', 'LABELS', 'SCHEME_ORDER',
    'read_csv', 'validate_meta', 'to_float_arrays', 'save_pdf',
    'REQUIRED_META',
    'HEATMAP_COLORS', 'NODE_COLORS',
    'plt', 'np', 'Path',
    'SCRIPT_DIR', 'HUATAI_DATA_DIR', 'FIG_DIR',
    'PRESENTATION_DATA_DIR',
]

SCRIPT_DIR = Path(__file__).resolve().parent
HUATAI_DATA_DIR = _RESULTS_DIR / 'data'
FIG_DIR = SCRIPT_DIR.parent / 'figures'
PRESENTATION_DATA_DIR = SCRIPT_DIR.parent / 'data'

# rcParams are already set by the upstream plot_figures.py import (module-level).
# No duplication here — common.py only adds infrastructure-figure colors and paths.
