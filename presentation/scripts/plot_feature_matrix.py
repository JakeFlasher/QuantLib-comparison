#!/usr/bin/env python3
"""Generate Figure 10: Feature matrix heatmap from feature_matrix.json."""

import json
import sys
from pathlib import Path

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import numpy as np

# Apply publication-quality settings
plt.rcParams.update({
    'font.family': 'serif',
    'font.size': 10,
    'axes.labelsize': 11,
    'axes.titlesize': 12,
    'legend.fontsize': 8,
    'xtick.labelsize': 9,
    'ytick.labelsize': 9,
    'figure.dpi': 300,
    'savefig.dpi': 300,
    'savefig.bbox': 'tight',
    'pdf.fonttype': 42,
    'ps.fonttype': 42,
})

SCRIPT_DIR = Path(__file__).resolve().parent
DATA_DIR = SCRIPT_DIR.parent / 'data'
FIG_DIR = SCRIPT_DIR.parent / 'figures'


def main():
    json_path = DATA_DIR / 'feature_matrix.json'
    if not json_path.exists():
        print(f"Error: {json_path} not found", file=sys.stderr)
        sys.exit(1)

    with open(json_path) as f:
        data = json.load(f)

    caps = data['capabilities']
    status_map = data['status_map']

    labels = [c['short_name'] for c in caps]
    versions = ['v1.23', 'QuantLib_huatai\n(1.42-dev)']

    matrix = np.array([
        [status_map[c['v123']] for c in caps],
        [status_map[c['huatai']] for c in caps],
    ]).T  # shape: (n_caps, 2)

    # Colorblind-safe: absent=red-ish, partial=yellow-ish, native=green-ish
    cmap = mcolors.ListedColormap(['#D55E00', '#E69F00', '#009E73'])
    bounds = [-0.5, 0.5, 1.5, 2.5]
    norm = mcolors.BoundaryNorm(bounds, cmap.N)

    fig, ax = plt.subplots(figsize=(5.5, 5.0))
    im = ax.imshow(matrix, cmap=cmap, norm=norm, aspect='auto')

    ax.set_xticks([0, 1])
    ax.set_xticklabels(versions, fontsize=10)
    ax.set_yticks(range(len(labels)))
    ax.set_yticklabels(labels, fontsize=9)

    # Annotate each cell
    status_labels = {0: 'Absent', 1: 'Partial', 2: 'Native'}
    for i in range(len(labels)):
        for j in range(2):
            val = matrix[i, j]
            text_color = 'white' if val == 0 else 'black'
            ax.text(j, i, status_labels[val], ha='center', va='center',
                    fontsize=8, color=text_color, fontweight='bold')

    ax.set_title('Feature Matrix: QuantLib v1.23 vs v1.42-dev', fontsize=12)
    ax.tick_params(top=True, bottom=False, labeltop=True, labelbottom=False)

    # Legend as color patches
    from matplotlib.patches import Patch
    legend_elements = [
        Patch(facecolor='#009E73', label='Native'),
        Patch(facecolor='#E69F00', label='Partial workaround'),
        Patch(facecolor='#D55E00', label='Absent'),
    ]
    ax.legend(handles=legend_elements, loc='lower center',
              bbox_to_anchor=(0.5, -0.12), ncol=3, fontsize=8)

    FIG_DIR.mkdir(parents=True, exist_ok=True)
    fig.savefig(FIG_DIR / 'fig10_feature_matrix.pdf',
                metadata={'CreationDate': None, 'ModDate': None})
    plt.close(fig)
    print('  Fig 10: Feature matrix heatmap')


if __name__ == '__main__':
    main()
