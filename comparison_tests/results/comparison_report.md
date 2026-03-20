# Comparison Report: QuantLib v1.23 vs QuantLib_huatai (1.42-dev)

## Executive Summary

Stock QuantLib v1.23 does not natively support the nonstandard finite difference scheme research implemented in QuantLib_huatai (1.42-dev). This is demonstrated through 12 passing tests: 7 compile-fail/absence tests against v1.23 headers, and 5 runtime tests on QuantLib_huatai. The v1.23 library cannot be built on either available compiler in this environment.

## Test Results (CTest-Verified)

All results from `ctest --output-on-failure` in `comparison_tests/build`.
CTest compiler: `/opt/llvm21-mlir/bin/clang++` (Clang 21.1.8).

```
12/12 tests passed, 0 failed
```

### Compile-Fail / Absence Tests (v1.23 headers)

| Test | What it proves |
|------|---------------|
| `v123_spatial_desc_missing` | `fdmblackscholesspatialdesc.hpp` absent — no scheme selection |
| `v123_discrete_barrier_missing` | `fdmdiscretebarrierstepcondition.hpp` absent — header not found |
| `v123_discrete_barrier_construct` | Cannot construct `FdmDiscreteBarrierStepCondition` — class absent |
| `v123_mmatrix_diagnostic_missing` | `fdmmatrixdiagnostic.hpp` absent — no M-matrix diagnostic |
| `v123_mesher_multipoint_missing` | Multi-point `cPoints` constructor absent |
| `v123_disposable_signature_mismatch` | With `QL_USE_DISPOSABLE`, `Array` return mismatches `Disposable<Array>` |
| `v123_library_build_fails` | v1.23 library build fails on the CTest compiler (Clang 21.1.8) |

### Runtime Tests (QuantLib_huatai)

**Oscillation (AC-1 partial)**: StandardCentral with sigma=0.001, r=0.05, K=50, U=70, T=5/12, xGrid=200:
- Negative interior nodes detected (V < -1e-10)
- *Note*: This verifies the oscillation on QuantLib_huatai's StandardCentral code path. The same behavior is *inferred* for v1.23 (identical algorithm), but not directly proven because the v1.23 library does not build on either available compiler. See "AC-1 Status" below.

**Positivity (AC-2)**: Both nonstandard schemes preserve positivity at sigma=0.001 and achieve <1% accuracy vs fine-grid reference at sigma=0.20.

**Discrete Barrier (AC-3)**: FdmDiscreteBarrierStepCondition successfully replicates Milev-Tagliani Example 4.1.

**M-Matrix Diagnostic (AC-4)**: `checkOffDiagonalNonNegative()` directly called on analytically constructed operators; `FdmMMatrixReport` fields verified.

**Paper Replication**: Grid convergence confirmed; low-vol stress test passed.

## AC-1 Status: v1.23 Runtime Remains Unverified

The plan's AC-1 requires a v1.23 runtime test. This is currently blocked:

1. **Source compatibility**: `test_cn_oscillation.cpp` is now v1.23 source-compatible (verified via `clang++ -fsyntax-only -I QuantLib_v1.23`).
2. **Library build failure**: v1.23 library cannot be built on either available compiler:
   - Clang 21.1.8: `timeseries.hpp` template instantiation failure
   - GCC 15.2.1: `timeseries.hpp:147 boost::mpl::if_` error
3. **Inference**: The StandardCentral algorithm is identical in both versions (v1.23 `fdmblackscholesop.cpp` has the same single code path). The oscillation verified on QuantLib_huatai is therefore expected to reproduce on v1.23, but this has not been directly confirmed.

**AC-1 remains open** pending a compatible compiler or v1.23 library build workaround.

## v1.23 Build Failure Evidence

### CTest-verified (Clang 21.1.8)
The `v123_library_build_fails` test configures and attempts to build v1.23 using the CTest compiler (Clang 21.1.8). It fails as expected.

### Manual reproduction (GCC 15.2.1)
```bash
cd QuantLib_v1.23/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++ -DCMAKE_CXX_FLAGS="-fpermissive -w" \
  -DCMAKE_CXX_STANDARD=17
cmake --build . -j$(nproc)
# Error: ql/timeseries.hpp:50: instantiating erroneous template
#        ql/timeseries.hpp:147: boost::mpl::if_
```

Both failures trace to `ql/timeseries.hpp` in the real v1.23 tag (`QuantLib-v1.23`, commit `b6517ba10`).
