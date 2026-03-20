# Comparison Report: QuantLib v1.23 vs QuantLib_huatai (1.42-dev)

## Executive Summary

Stock QuantLib v1.23 does not natively support the nonstandard finite difference scheme research implemented in QuantLib_huatai (1.42-dev). This is demonstrated through 12 passing tests: 7 compile-fail/absence tests against v1.23 headers, and 5 runtime tests on QuantLib_huatai showing the research capabilities work. The v1.23 library itself cannot be built on the available toolchain (GCC 15.2.1 + Boost 1.89.0), which is also verified by a dedicated test.

## Test Results

All results from `ctest --output-on-failure` in `comparison_tests/build`:

```
12/12 tests passed, 0 failed
```

### Phase 1: Compile-Fail / Absence Tests (v1.23 headers)

| Test | Status | What it proves |
|------|--------|---------------|
| `v123_spatial_desc_missing` | PASS | `fdmblackscholesspatialdesc.hpp` absent — no scheme selection possible |
| `v123_discrete_barrier_missing` | PASS | `fdmdiscretebarrierstepcondition.hpp` absent — header not found |
| `v123_discrete_barrier_construct` | PASS | Cannot construct `FdmDiscreteBarrierStepCondition` — class absent |
| `v123_mmatrix_diagnostic_missing` | PASS | `fdmmatrixdiagnostic.hpp` absent — no M-matrix diagnostic infrastructure |
| `v123_mesher_multipoint_missing` | PASS | Multi-point `cPoints` constructor absent |
| `v123_disposable_signature_mismatch` | PASS | With `QL_USE_DISPOSABLE`, `Array` return type mismatches `Disposable<Array>` base |
| `v123_library_build_fails` | PASS | v1.23 library cannot be built on GCC 15.2.1 (`timeseries.hpp` `boost::mpl::if_` error) |

### Phase 2: Runtime Tests (QuantLib_huatai)

**Oscillation (AC-1)**: StandardCentral with sigma=0.001, r=0.05, K=50, U=70, T=5/12, xGrid=200:
- Negative interior nodes: detected (V < -1e-10)
- Confirms: StandardCentral oscillates at low vol

**Positivity (AC-2)**:
- Low vol (sigma=0.001, xGrid=200): ExponentialFitting 0 negative nodes, MilevTaglianiCN 0 negative nodes
- Moderate vol accuracy (sigma=0.20, xGrid=800): Both nonstandard schemes within 1% of fine-grid reference at strike

**Discrete Barrier (AC-3)**: K=100, L=95, U=110, 5 monitoring dates:
- Moderate vol (sigma=0.25): All 3 schemes produce identical finite positive prices
- Low vol (sigma=0.001): StandardCentral has 58 negative nodes; nonstandard schemes have 0

### Phase 3: M-Matrix Diagnostic (AC-4)

- StandardCentral spatial operator: 198 negative off-diagonals (M-matrix violated)
- ExponentialFitting + FailFast policy: `checkOffDiagonalNonNegative()` runs, no exception (M-matrix satisfied)
- MilevTaglianiCN + FallbackToExponentialFitting: 0 negative off-diagonals after fallback

### Phase 5: Paper Replication

Grid convergence (sigma=0.25, T=0.5):
| xGrid | Standard | ExpFitting | MilevTagliani |
|-------|----------|------------|---------------|
| 200 | 0.264874 | 0.264874 | 0.264872 |
| 500 | 0.245054 | 0.245054 | 0.245054 |
| 1000 | 0.238712 | 0.238712 | 0.238712 |

Low-vol stress (sigma=0.001, T=1.0): StandardCentral 58 negative nodes; nonstandard 0.

## What Remains Unverified

**AC-1 on v1.23 runtime**: The plan specifies a v1.23 runtime test proving oscillations. This cannot be executed because v1.23 fails to build on the available toolchain. The oscillation behavior is verified on QuantLib_huatai (same StandardCentral code path). The v1.23 library build failure is independently verified by the `v123_library_build_fails` test.

## v1.23 Build Failure (Verified)

```
Compiler: g++ (GCC) 15.2.1 20260209
Boost: 1.89.0
Source: QuantLib v1.23 (tag QuantLib-v1.23, commit b6517ba10)
Error: ql/timeseries.hpp:50: instantiating erroneous template
       ql/timeseries.hpp:147: boost::mpl::if_ template failure
```

This is verified by the `v123_library_build_fails` CTest entry.
