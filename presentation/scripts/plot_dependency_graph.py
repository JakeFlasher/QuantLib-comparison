#!/usr/bin/env python3
"""Generate Figure 11: Backport dependency graph from dependency_graph.json."""

import json
import sys
from pathlib import Path

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

try:
    import networkx as nx
except ImportError:
    print("Error: networkx required. Install with: pip install networkx",
          file=sys.stderr)
    sys.exit(1)

# Apply publication-quality settings
plt.rcParams.update({
    'font.family': 'serif',
    'font.size': 10,
    'axes.labelsize': 11,
    'axes.titlesize': 12,
    'legend.fontsize': 8,
    'figure.dpi': 300,
    'savefig.dpi': 300,
    'savefig.bbox': 'tight',
    'pdf.fonttype': 42,
    'ps.fonttype': 42,
})

SCRIPT_DIR = Path(__file__).resolve().parent
DATA_DIR = SCRIPT_DIR.parent / 'data'
FIG_DIR = SCRIPT_DIR.parent / 'figures'

# Colorblind-safe: new=green, adapted=orange
NODE_COLORS = {
    'new': '#009E73',
    'adapted': '#D55E00',
}


def main():
    json_path = DATA_DIR / 'dependency_graph.json'
    if not json_path.exists():
        print(f"Error: {json_path} not found", file=sys.stderr)
        sys.exit(1)

    with open(json_path) as f:
        data = json.load(f)

    G = nx.DiGraph()
    node_info = {}
    for node in data['nodes']:
        G.add_node(node['id'])
        node_info[node['id']] = node

    for edge in data['edges']:
        G.add_edge(edge['from'], edge['to'])

    # Use graphviz layout if available, else spring layout
    try:
        pos = nx.nx_agraph.graphviz_layout(G, prog='dot')
    except (ImportError, Exception):
        pos = nx.spring_layout(G, k=2.5, iterations=100, seed=42)

    fig, ax = plt.subplots(figsize=(8.0, 10.0))

    colors = [NODE_COLORS[node_info[n]['type']] for n in G.nodes()]
    labels = {n: node_info[n]['label'] for n in G.nodes()}

    nx.draw_networkx_nodes(G, pos, ax=ax, node_color=colors,
                           node_size=2200, alpha=0.9, edgecolors='black',
                           linewidths=0.8)
    nx.draw_networkx_labels(G, pos, ax=ax, labels=labels,
                            font_size=6, font_family='serif')
    nx.draw_networkx_edges(G, pos, ax=ax, edge_color='gray',
                           arrows=True, arrowsize=15, width=1.0,
                           connectionstyle='arc3,rad=0.1')

    ax.set_title('Backport Dependency Graph\n(5 new files, 8 adapted, depth 5)',
                 fontsize=12)

    from matplotlib.patches import Patch
    legend_elements = [
        Patch(facecolor='#009E73', edgecolor='black', label='New (transplant)'),
        Patch(facecolor='#D55E00', edgecolor='black', label='Adapted (existing)'),
    ]
    ax.legend(handles=legend_elements, loc='lower right', fontsize=9)
    ax.axis('off')

    FIG_DIR.mkdir(parents=True, exist_ok=True)
    fig.savefig(FIG_DIR / 'fig11_dependency_graph.pdf',
                metadata={'CreationDate': None, 'ModDate': None})
    plt.close(fig)
    print('  Fig 11: Dependency graph')


if __name__ == '__main__':
    main()
