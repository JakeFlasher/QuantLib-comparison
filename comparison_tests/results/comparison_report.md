# Comparison Report: QuantLib v1.23 vs QuantLib_huatai (1.42-dev)

## Executive Summary

Stock QuantLib v1.23 does not natively support the nonstandard finite difference scheme research implemented in QuantLib_huatai (1.42-dev). The minimum credible backport requires 5 new physical files and adaptation of 6 existing files, with a dependency chain depth of 5 levels. Additionally, v1.23 fails to compile with GCC 15.2.1 + Boost 1.89.0 due to `boost::mpl::if_` template errors in `timeseries.hpp`.

## Verified Test Results

All results below were produced from `ctest --output-on-failure` in the `comparison_tests/build` directory.

### Phase 1: Compile-Fail Tests (v1.23 headers)

| Test | Status | Evidence |
|------|--------|----------|
| `v123_spatial_desc_missing` | PASS (expected fail) | `fdmblackscholesspatialdesc.hpp` does not exist in v1.23 |
| `v123_discrete_barrier_missing` | PASS (expected fail) | `fdmdiscretebarrierstepcondition.hpp` does not exist in v1.23 |
| `v123_mmatrix_diagnostic_missing` | PASS (expected fail) | `fdmmatrixdiagnostic.hpp` does not exist in v1.23 |
| `v123_mesher_multipoint_missing` | PASS (expected fail) | `cPoints` vector constructor overload does not exist in v1.23 |

### Phase 2: Runtime Tests (QuantLib_huatai 1.42-dev)

**Oscillation Test (AC-1)**:
- StandardCentral with sigma=0.001, r=0.05, K=50, U=70, T=5/12, xGrid=400:
  - 7 negative interior nodes, min V = -7.2812
  - Confirms spurious oscillations with no recourse on v1.23

**Positivity Test (AC-2)**:
- Same parameters, raw rollback array inspection:
  - StandardCentral: 7 negative nodes (confirms problem)
  - ExponentialFitting: 0 negative nodes, min V = 0 (positivity preserved)
  - MilevTaglianiCN: 0 negative nodes, min V = 2.8e-53 (positivity preserved)
  - Fine-grid reference price at K: 1.0317

**Discrete Barrier Test (AC-3)**:
- K=100, L=95, U=110, 5 monitoring dates, FdmBackwardSolver + FdmDiscreteBarrierStepCondition:
  - Moderate vol (sigma=0.25, T=0.5): All 3 schemes produce identical price 0.236266
  - Low vol (sigma=0.001, T=1.0): StandardCentral has 58 negative nodes; ExponentialFitting and MilevTaglianiCN have 0

### Phase 3: M-Matrix Diagnostic Test (AC-4)

- StandardCentral spatial operator at sigma=0.001: **198 negative off-diagonal entries** (M-matrix violated)
- ExponentialFitting: **0 negative off-diagonals** (M-matrix satisfied)
- FallbackToExponentialFitting policy: after fallback, **0 negative off-diagonals** (automatic correction)

### Phase 5: Paper Replication

Grid convergence at sigma=0.25, T=0.5 (all schemes converge):
| xGrid | Standard | ExpFitting | MilevTagliani |
|-------|----------|------------|---------------|
| 200 | 0.264874 | 0.264874 | 0.264872 |
| 500 | 0.245054 | 0.245054 | 0.245054 |
| 1000 | 0.238712 | 0.238712 | 0.238712 |

Low-vol stress (sigma=0.001, T=1.0): StandardCentral 58 negative nodes; nonstandard schemes: 0.

## Three-Axis Evaluation

### Axis 1: Numerical Capability

**Can v1.23 compute ExponentialFitting or MilevTaglianiCN?** NO.
- `FdmBlackScholesSpatialDesc` type does not exist (verified: compile-fail test passed)
- `setTime()` has a single StandardCentral code path
- StandardCentral produces 7 negative nodes at low vol (verified)

### Axis 2: API Integration

**Can the research be wired into v1.23?** NOT without invasive changes.
- `FdmDiscreteBarrierStepCondition` absent (verified: compile-fail test passed)
- `FdmMMatrixReport` / `checkOffDiagonalNonNegative()` absent (verified: compile-fail test passed)
- Multi-point `cPoints` mesher constructor absent (verified: compile-fail test passed)

### Axis 3: Build/Toolchain

**Can v1.23 compile with modern compilers?** NO.
- GCC 15.2.1 + Boost 1.89.0: hard error in `timeseries.hpp:147` (`boost::mpl::if_` template instantiation failure)
- `-fpermissive -w` flags do not resolve the error
- QuantLib_huatai (1.42-dev) builds and runs cleanly with same compiler

## v1.23 Build Failure (Reproduced)

```
Compiler: g++ (GCC) 15.2.1 20260209
Boost: 1.89.0
Source: QuantLib v1.23 (tag QuantLib-v1.23, commit b6517ba10)
Error: ql/timeseries.hpp:50:11: error: instantiating erroneous template
       ql/timeseries.hpp:147:38: note: first error appeared here
       typedef typename boost::mpl::if_ <
```

Command:
```bash
cd QuantLib_v1.23/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++ -DCMAKE_CXX_FLAGS="-fpermissive -w" \
  -DCMAKE_CXX_STANDARD=17
cmake --build . -j$(nproc)
```
