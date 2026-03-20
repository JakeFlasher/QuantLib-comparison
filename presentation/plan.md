# Academic Presentation: QuantLib v1.23 vs v1.42-dev Infrastructure Comparison

## Goal Description

Produce a publication-quality LaTeX manuscript with 11 professional figures presenting the verified comparison findings between stock QuantLib v1.23 and QuantLib_huatai (1.42-dev) for nonstandard finite difference scheme research. The manuscript demonstrates that v1.23 lacks the infrastructure to support ExponentialFitting and MilevTaglianiCN spatial discretization schemes, with all claims traceable to CTest results and version-controlled data. A single end-to-end script regenerates data, figures, and the final PDF.

### Research Papers Referenced
1. Milev & Tagliani (2010) — "Nonstandard Finite Difference Schemes with Application to Finance: Option Pricing" (Serdica Math. J., Vol. 36, pp. 75-88)
2. Milev & Tagliani (2010) — "Low Volatility Options and Numerical Diffusion of Finite Difference Schemes" (Serdica Math. J., Vol. 36, pp. 223-236)
3. Duffy (2004) — "A Critique of the Crank-Nicolson Scheme" (Wilmott Magazine, Issue 4, pp. 68-76)

### Figure Manifest (11 total)
- **Figs 1-9**: Existing from `QuantLib_huatai/results/plot_figures.py` (CN oscillations, three-scheme truncated call, barrier moderate/low vol, grid convergence, effective diffusion, M-matrix off-diagonals, benchmark, Peclet fitting factor)
- **Fig 10**: Feature matrix heatmap (NEW — from `feature_matrix.json`)
- **Fig 11**: Backport dependency graph (NEW — from `dependency_graph.json`)

### Verified Numerical Data Points
| Result | Source | Value |
|--------|--------|-------|
| v1.23 oscillation | `oscillation_v123` CTest | 5 neg nodes, min V = -6.14951 |
| Low-vol barrier StandardCentral | `discrete_barrier_huatai` CTest | 58 neg nodes |
| ExponentialFitting positivity | `positivity_huatai` CTest | 0 neg nodes |
| MilevTaglianiCN positivity | `positivity_huatai` CTest | 0 neg nodes |
| M-matrix StandardCentral | `mmatrix_diagnostic_huatai` CTest | 198 neg off-diags, min -4.512 |
| M-matrix ExponentialFitting | `mmatrix_diagnostic_huatai` CTest | 0 neg off-diags |
| Backport scope | `backport_analysis.md` | 13 files, depth 5 |
| Full test suite | CTest | 13/13 pass |

## Acceptance Criteria

- AC-1: LaTeX build succeeds
  - Positive Tests (expected to PASS):
    - `latexmk -pdf -halt-on-error main.tex` completes with exit code 0
    - All cross-references and citations resolve (no `??` or missing-ref warnings)
  - Negative Tests (expected to FAIL):
    - Removing a referenced figure file causes build failure
    - Removing references.bib causes citation failure

- AC-2: All 11 figures generate programmatically
  - Positive Tests (expected to PASS):
    - `python plot_all.py` produces all 11 PDF files in `presentation/figures/`
    - Each figure file is non-empty and valid PDF
  - Negative Tests (expected to FAIL):
    - Missing CSV data file causes script failure with clear error
    - Missing JSON input for fig10/fig11 causes failure

- AC-3: Consistent figure styling
  - Positive Tests (expected to PASS):
    - All figures use serif fonts, 10pt base, colorblind-safe palette (#0072B2, #D55E00, #009E73)
    - Style constants imported from existing `plot_figures.py` (no duplication)
  - Negative Tests (expected to FAIL):
    - A figure using a different font family or color palette

- AC-4: Provenance traceability
  - Positive Tests (expected to PASS):
    - `provenance.json` maps every figure and key claim to its source (CSV path, CTest name, or JSON input)
    - Manuscript parameter table matches CSV metadata headers
  - Negative Tests (expected to FAIL):
    - A numerical claim in the manuscript without a corresponding provenance entry

- AC-5: Bibliography resolves
  - Positive Tests (expected to PASS):
    - BibTeX contains entries for Milev-Tagliani 2010 (both papers), Duffy 2004, QuantLib documentation
    - All `\cite{}` commands resolve to bibliography entries
  - Negative Tests (expected to FAIL):
    - A `\cite{missing_key}` in the manuscript

- AC-6: End-to-end pipeline
  - Positive Tests (expected to PASS):
    - `bash build_all.sh` runs: ctest verification → data generation → 11 figures → PDF compilation
    - Script uses `set -euo pipefail` and fails on any missing artifact
  - Negative Tests (expected to FAIL):
    - build_all.sh succeeding when comparison_tests have not been built

- AC-7: Infrastructure figures from machine-readable sources
  - Positive Tests (expected to PASS):
    - `feature_matrix.json` and `dependency_graph.json` are valid JSON
    - Fig10 and Fig11 read exclusively from these JSON files (no markdown parsing)
  - Negative Tests (expected to FAIL):
    - Generating fig10/fig11 without the JSON files

## Path Boundaries

### Upper Bound (Maximum Acceptable Scope)
The implementation includes a complete LaTeX manuscript with all 11 figures, provenance tracking, auto-generated parameter tables, a full build pipeline, and a summary results table. The manuscript covers Introduction, Mathematical Background, Infrastructure Analysis, Numerical Experiments, Discussion, and Conclusion sections with proper cross-references throughout.

### Lower Bound (Minimum Acceptable Scope)
The implementation includes the LaTeX manuscript with all 11 figures and the build pipeline. Provenance tracking can be a structured comment in the manuscript source rather than a separate JSON file. Parameter tables can be manually maintained. The manuscript covers all six sections with at minimum one paragraph each.

### Allowed Choices
- Can use: LaTeX article class (or SIAM/Springer templates if preferred), matplotlib for all plots, networkx for dependency graph, latexmk for build, BibTeX or BibLaTeX
- Can use: Existing `plot_figures.py` style constants directly (import or copy)
- Can use: Existing CSV data in `QuantLib_huatai/results/data/` without regeneration
- Cannot use: Manual figure creation (all must be programmatic)
- Cannot use: Hardcoded numerical values in manuscript without provenance source

## Feasibility Hints and Suggestions

### Conceptual Approach

The existing `QuantLib_huatai/results/` infrastructure already handles 9 of 11 figures. The plan extends this with:

1. **JSON extraction** from markdown analysis files → `feature_matrix.json`, `dependency_graph.json`
2. **Two new plot scripts** reading the JSON files
3. **`plot_all.py`** orchestrates: copy/symlink existing 9 figures + generate 2 new ones
4. **LaTeX manuscript** structured as an academic article
5. **`build_all.sh`** ties everything together

For the feature matrix heatmap:
```python
# Read feature_matrix.json with 9 capabilities × 2 versions
# Map: native=2, partial-workaround=1, absent=0
# Use seaborn heatmap or matplotlib imshow with custom colormap
```

For the dependency graph:
```python
# Read dependency_graph.json with nodes (files) and edges (dependencies)
# Color-code: green=new transplant, orange=adapted existing
# Use networkx + matplotlib for DAG layout
```

### Relevant References
- `QuantLib_huatai/results/plot_figures.py` — existing 9-figure plotting pipeline with style settings
- `QuantLib_huatai/results/data/` — 28 CSV data files
- `QuantLib_huatai/results/reproduce.sh` — existing build/data/figure pipeline
- `QuantLib_huatai/ql/paper_references/math_formulation_in_latex.tex` — mathematical formulations (reference, not copy)
- `comparison_tests/phase4_analysis/feature_matrix.md` — source for fig10 JSON
- `comparison_tests/phase4_analysis/backport_analysis.md` — source for fig11 JSON
- `comparison_tests/results/comparison_report.md` — AC-by-AC verified results

## Dependencies and Sequence

### Milestones

1. **Data Preparation**: Extract structured data for infrastructure figures
   - Create `feature_matrix.json` and `dependency_graph.json` from markdown analysis
   - Create `provenance.json` mapping claims to sources

2. **Figure Pipeline**: Extend existing 9-figure pipeline with 2 new infrastructure figures
   - Create `common.py` importing existing style constants
   - Create `plot_feature_matrix.py` and `plot_dependency_graph.py`
   - Create `plot_all.py` orchestrating all 11 figures
   - Depends on Milestone 1 for JSON inputs

3. **Manuscript Writing**: LaTeX document with all sections
   - Create skeleton with `references.bib`
   - Write all 6 sections with figure/table cross-references
   - Depends on Milestone 2 for figure file names

4. **Build Pipeline**: End-to-end automation
   - Create `build_all.sh` with ctest → data → figures → PDF
   - Verify full pipeline on clean build
   - Depends on Milestones 2 and 3

## Task Breakdown

| Task ID | Description | Target AC | Tag | Depends On |
|---------|-------------|-----------|-----|------------|
| task1 | Create provenance.json mapping claims to CTest/CSV sources | AC-4 | coding | - |
| task2 | Extract feature_matrix.json from feature_matrix.md | AC-7 | coding | - |
| task3 | Extract dependency_graph.json from backport_analysis.md | AC-7 | coding | - |
| task4 | Create common.py importing style constants from existing plot_figures.py | AC-3 | coding | - |
| task5 | Create plot_feature_matrix.py reading feature_matrix.json | AC-2, AC-7 | coding | task2, task4 |
| task6 | Create plot_dependency_graph.py reading dependency_graph.json | AC-2, AC-7 | coding | task3, task4 |
| task7 | Create plot_all.py orchestrating existing 9 + 2 new figures | AC-2 | coding | task5, task6 |
| task8 | Create references.bib (Milev-Tagliani ×2, Duffy, QuantLib docs) | AC-5 | coding | - |
| task9 | Create main.tex LaTeX skeleton with all section stubs and figure floats | AC-1 | coding | task8 |
| task10 | Write Introduction and Mathematical Background sections | AC-1 | coding | task9 |
| task11 | Write Infrastructure Analysis section with feature matrix table | AC-1, AC-4 | coding | task9, task1 |
| task12 | Write Numerical Experiments section with 11 figure references | AC-1, AC-4 | coding | task9, task7 |
| task13 | Write Discussion and Conclusion sections | AC-1 | coding | task10, task11, task12 |
| task14 | Create build_all.sh end-to-end pipeline (set -euo pipefail) | AC-6 | coding | task7, task9 |
| task15 | Verify full pipeline: build_all.sh → ctest → figures → PDF | AC-1 through AC-7 | coding | task13, task14 |

## Claude-Codex Deliberation

### Agreements
- LaTeX manuscript (not slides) is the correct deliverable
- Reuse existing 9-figure pipeline from plot_figures.py
- 11 total figures (9 existing + 2 new infrastructure visuals)
- Machine-readable JSON inputs for fig10/fig11 (not markdown parsing)
- latexmk -pdf for build (not single pdflatex run)
- End-to-end pipeline includes ctest verification stage
- Scope bounded to Black-Scholes FD for barrier/truncated-call options
- Standardize naming: "MilevTaglianiCNEffectiveDiffusion (hereafter MilevTaglianiCN)"

### Resolved Disagreements
- **Figure count**: Claude initially proposed 10 figures. Codex noted existing fig9 (xcothx) was already taken. Resolution: 11 figures total (9 existing + 2 new).
- **Build command**: Claude proposed `pdflatex main.tex`. Codex required `latexmk -pdf -halt-on-error` for full ref/cite resolution. Resolution: adopted.
- **Verification scope**: Claude's pipeline only regenerated huatai figures. Codex required ctest verification of comparison_tests. Resolution: build_all.sh includes ctest stage.
- **Infrastructure figure inputs**: Claude had no structured data source. Codex required machine-readable JSON. Resolution: task2/task3 extract JSON from markdown.

### Convergence Status
- Final Status: `converged`
- Rounds: 2
- Round 1: 7 required changes (figure count, build command, verification, JSON inputs, provenance, bibliography, task order)
- Round 2: 0 required changes (approved for implementation)

## Pending User Decisions

No pending decisions. User confirmed LaTeX manuscript as the deliverable.

## Implementation Notes

### Code Style Requirements
- Implementation code and comments must NOT contain plan-specific terminology such as "AC-", "Milestone", "Step", "Phase", or similar workflow markers
- These terms are for plan documentation only, not for the resulting codebase
- Use descriptive, domain-appropriate naming in code instead
- LaTeX manuscript should use standard academic conventions (no plan IDs in text)

### Naming Convention
- Define once in manuscript abstract/introduction: "MilevTaglianiCNEffectiveDiffusion (hereafter MilevTaglianiCN)"
- Use "MilevTaglianiCN" consistently in plot legends, table headers, and prose
- Match code identifiers: `StandardCentral`, `ExponentialFitting`, `MilevTaglianiCN`

### Style Constants (from existing plot_figures.py)
- Figure size: 6.5" × 4.0" @ 300 DPI
- Font: serif, 10pt base
- Colors: #0072B2 (blue/StandardCentral), #D55E00 (orange/ExponentialFitting), #009E73 (green/MilevTaglianiCN)
- Line styles: solid (StandardCentral), dashed (ExponentialFitting), dash-dot (MilevTaglianiCN)

--- Original Design Draft Start ---

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
- **v1.23 result**: 5 negative interior nodes, min V = -6.14951
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

--- Original Design Draft End ---
