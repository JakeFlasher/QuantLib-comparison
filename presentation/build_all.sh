#!/usr/bin/env bash
set -euo pipefail

# End-to-end pipeline: CTest verification -> data generation -> figures -> PDF
#
# Usage: bash build_all.sh [--skip-ctest] [--skip-figures]

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Auto-detect comparison test build directory
if [ -d "${REPO_ROOT}/comparison_tests/build" ]; then
    BUILD_DIR="${REPO_ROOT}/comparison_tests/build"
elif [ -d "${REPO_ROOT}/build" ]; then
    BUILD_DIR="${REPO_ROOT}/build"
else
    BUILD_DIR=""
fi

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
    if [ -z "${BUILD_DIR}" ] || [ ! -d "${BUILD_DIR}" ]; then
        echo "Error: No CTest build directory found."
        echo "Searched: ${REPO_ROOT}/comparison_tests/build, ${REPO_ROOT}/build"
        echo "Run: cmake -S comparison_tests -B comparison_tests/build && cmake --build comparison_tests/build"
        exit 1
    fi
    echo "Using build directory: ${BUILD_DIR}"
    ctest --test-dir "${BUILD_DIR}" --output-on-failure
    echo "CTest: all tests passed."
    echo ""
fi

# Stage 2: Regenerate CSV data from QuantLib_huatai pipeline
if [ "$SKIP_FIGURES" = false ]; then
    echo "--- Stage 2: Data Regeneration ---"
    HUATAI_RESULTS="${REPO_ROOT}/QuantLib_huatai/results"
    HUATAI_BUILD="${HUATAI_RESULTS}/build"
    HUATAI_DATA="${HUATAI_RESULTS}/data"
    HUATAI_LIB="${REPO_ROOT}/QuantLib_huatai/build/ql"

    if [ ! -x "${HUATAI_BUILD}/generate_data" ]; then
        echo "Building data generator..."
        cmake -S "${HUATAI_RESULTS}" -B "${HUATAI_BUILD}" -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -3
        cmake --build "${HUATAI_BUILD}" --parallel 2>&1 | tail -3
    fi

    echo "Running data generator..."
    mkdir -p "${HUATAI_DATA}"
    (cd "${HUATAI_BUILD}" && LD_LIBRARY_PATH="${HUATAI_LIB}:${LD_LIBRARY_PATH:-}" ./generate_data "${HUATAI_DATA}")

    # Verify expected CSV files exist
    EXPECTED_CSVS=(truncated_call_StandardCentral.csv barrier_moderate_vol_StandardCentral.csv
                   barrier_low_vol_StandardCentral.csv grid_convergence_StandardCentral.csv
                   effective_diffusion_StandardCentral.csv mmatrix_offdiag_StandardCentral.csv
                   benchmark_StandardCentral.csv xcothx.csv)
    for csv in "${EXPECTED_CSVS[@]}"; do
        if [ ! -s "${HUATAI_DATA}/${csv}" ]; then
            echo "Error: Missing or empty CSV after data generation: ${csv}" >&2
            exit 1
        fi
    done
    echo "Data regeneration complete: $(ls "${HUATAI_DATA}"/*.csv | wc -l) CSV files."
    echo ""

    # Stage 3: Generate all 11 figures from fresh data
    echo "--- Stage 3: Figure Generation ---"
    python "${SCRIPT_DIR}/scripts/plot_all.py"
    echo ""
fi

# Stage 4: Verify all figure PDFs exist
echo "--- Stage 4: Figure Verification ---"
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

# Stage 5: LaTeX compilation
echo "--- Stage 5: LaTeX Compilation ---"
cd "${SCRIPT_DIR}"
latexmk -pdf -halt-on-error -interaction=nonstopmode main.tex
echo ""
echo "=== Build complete: presentation/main.pdf ==="
