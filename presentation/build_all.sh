#!/usr/bin/env bash
set -euo pipefail

# End-to-end pipeline: CTest verification -> data generation -> figures -> PDF
#
# Usage: bash build_all.sh [--skip-ctest] [--skip-figures]

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build"

SKIP_CTEST=false
SKIP_FIGURES=false

for arg in "$@"; do
    case "$arg" in
        --skip-ctest)  SKIP_CTEST=true ;;
        --skip-figures) SKIP_FIGURES=true ;;
        *) echo "Unknown argument: $arg" >&2; exit 1 ;;
    esac
done

echo "=== Academic Presentation Build Pipeline ==="
echo "Repository: ${REPO_ROOT}"
echo ""

# Stage 1: CTest verification (optional)
if [ "$SKIP_CTEST" = false ]; then
    echo "--- Stage 1: CTest Verification ---"
    if [ ! -d "${BUILD_DIR}" ]; then
        echo "Error: Build directory ${BUILD_DIR} not found."
        echo "Run: cmake -S comparison_tests -B build && cmake --build build"
        exit 1
    fi
    ctest --test-dir "${BUILD_DIR}" --output-on-failure
    echo "CTest: all tests passed."
    echo ""
fi

# Stage 2: Generate all 11 figures
if [ "$SKIP_FIGURES" = false ]; then
    echo "--- Stage 2: Figure Generation ---"
    python "${SCRIPT_DIR}/scripts/plot_all.py"
    echo ""
fi

# Stage 3: Verify all figure PDFs exist
echo "--- Stage 3: Figure Verification ---"
EXPECTED_FIGS=(
    fig1_cn_oscillations.pdf
    fig2_three_scheme_truncated.pdf
    fig3_barrier_moderate.pdf
    fig4_barrier_lowvol.pdf
    fig5_convergence.pdf
    fig6_effective_diffusion.pdf
    fig7_mmatrix.pdf
    fig8_benchmark.pdf
    fig9_xcothx.pdf
    fig10_feature_matrix.pdf
    fig11_dependency_graph.pdf
)

for fig in "${EXPECTED_FIGS[@]}"; do
    if [ ! -s "${SCRIPT_DIR}/figures/${fig}" ]; then
        echo "Error: Missing or empty figure: ${fig}" >&2
        exit 1
    fi
done
echo "All 11 figures verified."
echo ""

# Stage 4: LaTeX compilation
echo "--- Stage 4: LaTeX Compilation ---"
cd "${SCRIPT_DIR}"
latexmk -pdf -halt-on-error -interaction=nonstopmode main.tex
echo ""
echo "=== Build complete: presentation/main.pdf ==="
