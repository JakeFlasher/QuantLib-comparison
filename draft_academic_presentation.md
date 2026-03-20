# Draft: Academic Presentation of QuantLib v1.23 vs v1.42-dev Infrastructure Comparison

## Context

This project has completed a systematic, test-driven comparison between stock QuantLib v1.23 and the research branch QuantLib_huatai (1.42-dev), demonstrating that the v1.23 FD infrastructure cannot natively support nonstandard finite difference schemes for Black-Scholes PDE. All claims are backed by 13/13 passing CTest entries from a dual-version CMake build.

The research implements three spatial discretization schemes from the academic literature:
1. **StandardCentral** — baseline Crank-Nicolson (Crank & Nicolson, 1947)
2. **ExponentialFitting** — Duffy (2004), Wilmott Magazine
3. **MilevTaglianiCNEffectiveDiffusion** — Milev & Tagliani (2010), Serdica Math. J.

## Goal

Produce a publication-quality academic presentation document with:
1. **LaTeX manuscript** (or equivalent structured document) presenting the comparison findings
2. **Professional plots** (Python matplotlib/pgfplots) demonstrating the key numerical results
3. **Reproducible figure generation** from the existing verified test data and QuantLib_huatai results infrastructure

The presentation should be suitable for an academic audience evaluating whether QuantLib v1.23 or v1.42-dev is the appropriate base for nonstandard FD scheme research.

## Verified Numerical Results to Present

### Figure 1: Crank-Nicolson Spurious Oscillations (AC-1)
- **Data**: Truncated call, σ=0.001, r=0.05, K=50, U=70, T=5/12, xGrid=200
- **v1.23 result**: 5 negative interior nodes, min V = -6.14951, max V = 17.2146
- **Huatai StandardCentral (same algorithm)**: identical oscillation pattern
- **Plot**: V(S) vs S showing the characteristic oscillation spikes near the truncation point U=70
- **Source**: `test_cn_oscillation_v123` CTest output + QuantLib_huatai `results/generate_data.cpp` existing CSV data

### Figure 2: Three-Scheme Comparison on Truncated Call (AC-2)
- **Data**: Same parameters as Fig 1; all 3 schemes on the same axes
- **StandardCentral**: severe negative spikes near U=70
- **ExponentialFitting**: smooth, all V ≥ 0, 0 negative nodes
- **MilevTaglianiCN**: smooth, all V ≥ 0, min V = 2.8e-53
- **Fine-grid reference**: ExponentialFitting at xGrid=6400
- **Plot**: Overlay of V(S) for all 3 schemes + reference, zoomed near the discontinuity
- **Source**: `test_positivity_huatai` + existing `results/data/truncated_call_*.csv`

### Figure 3: Discrete Double Barrier — Moderate Volatility (AC-3)
- **Data**: K=100, L=95, U=110, σ=0.25, r=0.05, T=0.5, 5 monitoring dates
- **All 3 schemes**: identical prices (0.236266 at K=100 with xGrid=2000)
- **Grid convergence**: xGrid={200, 500, 1000} showing O(h²) convergence
- **Plot**: V(S) profiles for each scheme; convergence plot showing error vs grid size
- **Source**: `test_discrete_barrier_huatai` + `reproduce_paper_results` + existing `results/data/barrier_moderate_vol_*.csv`

### Figure 4: Discrete Double Barrier — Low Volatility (AC-3)
- **Data**: K=100, L=95, U=110, σ=0.001, r=0.05, T=1.0, 5 monitoring dates
- **StandardCentral**: 58 negative nodes (M-matrix violation)
- **ExponentialFitting**: 0 negative nodes
- **MilevTaglianiCN**: 0 negative nodes
- **Plot**: V(S) profiles showing StandardCentral oscillations vs smooth nonstandard solutions
- **Source**: `test_discrete_barrier_huatai` + existing `results/data/barrier_low_vol_*.csv`

### Figure 5: M-Matrix Off-Diagonal Analysis (AC-4)
- **Data**: σ=0.001, r=0.05, xGrid=200, uniform log-mesh [ln(50), ln(150)]
- **StandardCentral**: 198 negative off-diagonal entries, min lower = -4.512
- **ExponentialFitting**: 0 negative off-diagonals
- **MilevTaglianiCN + Fallback**: 0 after automatic correction
- **Plot**: Off-diagonal values a_{i,i-1} and a_{i,i+1} vs grid index for each scheme
- **Source**: `test_mmatrix_diagnostic_huatai` + existing `results/data/mmatrix_offdiag_*.csv`

### Figure 6: Effective Diffusion Coefficients
- **Data**: Same parameters as Fig 5
- **StandardCentral**: σ²/2 ≈ 5×10⁻⁷ (constant, insufficient)
- **ExponentialFitting**: (σ²/2)·ρ(Pe) — adaptive, much larger
- **MilevTaglianiCN**: σ²/2 + r²h²/(8σ²) — even larger at extreme low vol
- **Plot**: a_eff(S) vs S for each scheme on log scale
- **Source**: existing `results/data/effective_diffusion_*.csv`

### Figure 7: Péclet Number Fitting Factor ρ = x·coth(x)
- **Data**: Pe ∈ [-100, 100], showing three-regime implementation
- **Regimes**: |x| < 10⁻⁶ (Taylor), |x| > 50 (asymptotic), else direct
- **Key property**: ρ(0) = 1 (reduces to StandardCentral), ρ → |Pe| (upwind-like)
- **Plot**: ρ vs Pe with regime boundaries marked
- **Source**: existing `results/data/xcothx.csv`

### Figure 8: Grid Convergence Study
- **Data**: European call, S=K=100, r=0.05, q=0.02, σ=0.20, T=1.0
- **Grid sizes**: xGrid = {25, 50, 100, 200, 400, 800, 1600}
- **All 3 schemes**: O(h²) convergence on moderate vol
- **Plot**: |error| vs xGrid on log-log axes, confirming second-order convergence
- **Source**: existing `results/data/grid_convergence_*.csv`

### Figure 9: Feature Matrix Visualization
- **Data**: 9 capabilities × 2 versions
- **Classification**: native / absent / partial-workaround
- **Plot**: Heatmap or annotated table showing capability gaps
- **Source**: `comparison_tests/phase4_analysis/feature_matrix.md`

### Figure 10: Backport Dependency Graph
- **Data**: 13 files, depth-5 dependency chain
- **Structure**: Directed graph showing transplant (new) vs adaptation (modified) files
- **Key insight**: Capabilities cannot be backported independently
- **Plot**: DAG visualization with color-coded nodes (new vs adapted)
- **Source**: `comparison_tests/phase4_analysis/backport_analysis.md`

## Document Structure

### 1. Introduction
- Problem statement: spurious oscillations in CN for low-vol barrier options
- Research question: can QuantLib v1.23 support nonstandard FD schemes?
- Contribution: systematic comparison proving v1.42-dev infrastructure is necessary

### 2. Mathematical Background
- Black-Scholes PDE in log-space
- Three spatial discretization schemes with mathematical formulations
- M-matrix property and its role in positivity preservation
- Péclet number and the fitting factor ρ = x·coth(x)

### 3. Infrastructure Analysis
- Feature matrix (9 capabilities)
- Backport dependency graph (13 files, depth 5)
- API surface differences (Disposable<>, FdmBlackScholesSpatialDesc)
- Build toolchain compatibility (timeseries.hpp failure)

### 4. Numerical Experiments
- Truncated call oscillation (Figs 1-2)
- Discrete double barrier at moderate/low vol (Figs 3-4)
- M-matrix diagnostics (Fig 5)
- Effective diffusion comparison (Fig 6)
- Péclet fitting factor (Fig 7)
- Grid convergence (Fig 8)

### 5. Discussion
- Why the infrastructure gap matters for reproducibility
- Comparison of backport effort vs upgrade path
- Implications for other QuantLib-based research projects

### 6. Conclusion
- Stock v1.23 insufficient for nonstandard FD research
- Minimum backport recapitulates research branch module structure
- v1.42-dev provides the correct infrastructure natively

## Technical Requirements

### Plotting
- Use Python matplotlib with publication-quality settings (serif fonts, proper axis labels, legends)
- Alternatively, pgfplots for direct LaTeX integration
- All figures should be vector format (PDF/SVG) for print quality
- Color scheme: colorblind-safe palette (e.g., seaborn's colorblind palette or Tol's bright)
- Grid lines, proper tick formatting, and mathematical axis labels

### Data Sources
- Primary: existing CSV files in `QuantLib_huatai/results/data/` (already generated by `generate_data.cpp`)
- Secondary: new data generation from the comparison test executables
- All data must be reproducible from the committed code

### LaTeX/Document
- Academic article format (e.g., article class with amsmath, graphicx, booktabs)
- BibTeX references for the three papers (Milev-Tagliani 2010, Duffy 2004)
- Proper equation numbering, theorem-like environments for key properties
- Tables using booktabs style

## File Structure

```
QuantLib_comparison/
  presentation/
    main.tex                    # Main LaTeX document
    references.bib              # BibTeX references
    figures/
      fig1_oscillation.pdf
      fig2_three_scheme.pdf
      fig3_barrier_moderate.pdf
      fig4_barrier_lowvol.pdf
      fig5_mmatrix.pdf
      fig6_effective_diffusion.pdf
      fig7_xcothx.pdf
      fig8_convergence.pdf
      fig9_feature_matrix.pdf
      fig10_dependency_graph.pdf
    scripts/
      plot_all.py               # Master plotting script
      plot_oscillation.py       # Individual figure scripts
      plot_barrier.py
      plot_mmatrix.py
      plot_convergence.py
      plot_xcothx.py
      plot_feature_matrix.py
      plot_dependency_graph.py
      common.py                 # Shared plotting configuration
    data/
      -> symlinks to QuantLib_huatai/results/data/
```

## Acceptance Criteria

1. LaTeX document compiles to PDF without errors
2. All 10 figures generate from a single `python plot_all.py` invocation
3. Figures use consistent styling (fonts, colors, line widths, legend placement)
4. Every numerical claim in the text is traceable to a CTest result or CSV data file
5. BibTeX references resolve correctly for all three papers
6. Document follows standard academic formatting conventions
7. Plots are colorblind-safe and suitable for both screen and print
8. All data is reproducible from committed code (no manual data entry)
