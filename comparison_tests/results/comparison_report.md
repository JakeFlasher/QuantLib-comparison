# Comparison Report: QuantLib v1.23 vs QuantLib_huatai (1.42-dev)

## Executive Summary

Stock QuantLib v1.23 does not natively support the nonstandard finite difference scheme research implemented in QuantLib_huatai (1.42-dev). This is demonstrated through 13 passing CTest entries: 7 compile-fail/absence tests against v1.23 headers, 1 v1.23 runtime oscillation test, and 5 QuantLib_huatai runtime tests. The dual-version comparison runs from a single CMake invocation.

## Test Results (CTest-Verified)

Compiler: `/opt/llvm21-mlir/bin/clang++` (Clang 21.1.8).
Build: `cmake -S comparison_tests -B <build> && cmake --build <build> && ctest --test-dir <build>`.

```
13/13 tests passed, 0 failed
```

## AC-by-AC Verification

### AC-1: v1.23 StandardCentral Oscillation (CTest-verified)

**Test**: `oscillation_v123` — real v1.23 executable built from minimal source library.
**Parameters**: sigma=0.001, r=0.05, K=50, U=70, T=5/12, xGrid=200.
**Result**: 5 negative interior nodes, min V = -6.14951.
**Verdict**: **PASS** — v1.23 StandardCentral oscillates at low vol with no recourse.

Note: v1.23 full library does not build on the available toolchain. The comparison test suite works around this by building a minimal-source v1.23 static library from the FD infrastructure sources, excluding modules that fail on `timeseries.hpp`.

### AC-2: QuantLib_huatai Positivity + Accuracy (CTest-verified)

**Test**: `positivity_huatai`
**Positivity** (sigma=0.001, xGrid=200): ExponentialFitting 0 neg nodes; MilevTaglianiCN 0 neg nodes.
**Accuracy** (sigma=0.20, xGrid=800 vs 6400 reference): Both nonstandard schemes within 1% at strike.
**Verdict**: **PASS**

### AC-3: Discrete Barrier Replication (CTest-verified)

**Positive path**: `discrete_barrier_huatai` — K=100, L=95, U=110, 5 monitoring dates.
- Moderate vol (sigma=0.25, T=0.5): All 3 schemes produce identical positive prices.
- Low vol (sigma=0.001, T=1.0): StandardCentral 58 neg nodes; nonstandard 0.

**Negative path** (CTest-verified):
- `v123_discrete_barrier_missing`: header absent in v1.23.
- `v123_discrete_barrier_construct`: cannot construct the class in v1.23.

**Verdict**: **PASS**

### AC-4: M-Matrix Diagnostic Infrastructure (CTest-verified)

**Test**: `mmatrix_diagnostic_huatai`
- `checkOffDiagonalNonNegative()` called directly via `ModTripleBandLinearOp`
- StandardCentral: `report.ok=false`, `negativeLower=198`
- ExponentialFitting: `report.ok=true`
- FailFast integration: ExponentialFitting does not throw

**Negative path**: `v123_mmatrix_diagnostic_missing` — `fdmmatrixdiagnostic.hpp` absent in v1.23.
**Verdict**: **PASS**

### AC-5: Backport Scope Analysis (Document-verified)

**Document**: `comparison_tests/phase4_analysis/backport_analysis.md`
- 5 new physical files (transplant)
- 8 existing files modified (adaptation, including mesher)
- 13 total files touched
- Dependency chain depth 5
- Capabilities cannot be backported independently (one-way dependency chain)
- Minimum credible backport recapitulates the research branch module structure

**Verdict**: **PASS** (document evidence)

### AC-6: Disposable<> API Surface (CTest-verified + Document)

**CTest**: `v123_disposable_signature_mismatch` — with `QL_USE_DISPOSABLE`, v1.41-style `Array` returns cause signature mismatch against `Disposable<Array>` base class.
**Document**: `comparison_tests/phase4_analysis/disposable_api_analysis.md` — `Disposable<T>` aliases to `T` by default; barrier only with `QL_USE_DISPOSABLE`.
**Verdict**: **PASS**

### AC-7: Feature Matrix (Document-verified)

**Document**: `comparison_tests/phase4_analysis/feature_matrix.md`
- 9 capabilities classified as native / absent / partial-workaround
- 5 absent (core research: spatial desc, ExponentialFitting, MilevTaglianiCN, discrete barrier, M-matrix diagnostics)
- 3 partial-workaround (multi-point mesh, operator band access, Disposable)
- 1 native both (StandardCentral)
**Verdict**: **PASS**

### AC-8: Build Reproducibility (CTest-verified)

**Dual-version CMake flow**: A single `cmake -S comparison_tests -B <build>` configures:
- v1.23 minimal source library (FD infrastructure, excluding `timeseries.hpp` modules)
- QuantLib_huatai shared library (imported)
- 7 compile-fail tests + 1 full-library build failure test + 1 v1.23 runtime + 5 huatai runtime = 13 tests

**Result**: 13/13 pass from clean configure/build/test.
**Verdict**: **PASS**

## v1.23 Build Approach

The full v1.23 upstream library fails to build on both available compilers:
- Clang 21.1.8: `timeseries.hpp` template instantiation failure (verified by `v123_library_build_fails` CTest)
- GCC 15.2.1: `timeseries.hpp:147 boost::mpl::if_` error (manual reproduction)

The comparison suite works around this by building a minimal v1.23 static library from ~450 compilable source files (excluding cashflows, indexes, instruments, pricing engines, and other modules that transitively include `timeseries.hpp`). The FD infrastructure compiles cleanly and links the oscillation test executable.
