# Comparison Report: QuantLib v1.23 vs QuantLib_huatai (1.42-dev)

## Executive Summary

Stock QuantLib v1.23 does not natively support the nonstandard finite difference scheme research implemented in QuantLib_huatai (1.42-dev). The minimum credible backport requires 4 new files and adaptation of 6 existing files, with a dependency chain depth of 5 levels. This substantially recapitulates the same module structure as the research branch. Additionally, v1.23 fails to compile with modern C++ compilers (GCC 15, Clang 21) due to template compatibility issues that have been resolved in v1.42-dev.

## Three-Axis Evaluation

### Axis 1: Numerical Capability

**Question**: Can v1.23 compute the three spatial schemes (StandardCentral, ExponentialFitting, MilevTaglianiCN)?

**Answer**: NO.
- v1.23's `FdmBlackScholesOp::setTime()` has a single hardcoded code path (StandardCentral)
- No `FdmBlackScholesSpatialDesc` type exists for scheme selection
- No `xCothx()` helper for Peclet-based fitting factor computation
- StandardCentral produces spurious oscillations (V < -1e-10) at sigma=0.001 with no recourse

**Evidence**: Phase 1 compile-fail tests (spatial desc missing), Phase 2 runtime tests (oscillation detected)

### Axis 2: API Integration

**Question**: Can the research be wired into v1.23's FD framework?

**Answer**: NOT without invasive modifications.
- `FdmDiscreteBarrierStepCondition` class does not exist (discrete monitoring impossible)
- `FdmMMatrixReport` and `checkOffDiagonalNonNegative()` diagnostic infrastructure absent
- Barrier engine has only continuous monitoring (Dirichlet BCs at every timestep)
- Solver lacks CN-equivalence validation for Milev-Tagliani scheme safety

**Evidence**: Phase 1 compile-fail tests, Phase 3 infrastructure tests

### Axis 3: Build/Toolchain Compatibility

**Question**: Can v1.42-dev code compile against v1.23?

**Answer**: PARTIALLY, with significant caveats.
- `Disposable<T>` aliases to `T` by default — not a hard compile barrier
- However, v1.23 itself fails to compile with GCC 15 / Clang 21 (template errors in timeseries.hpp)
- Modern C++ features (range-based for, noexcept, unique_ptr) used in v1.42-dev are not available in v1.23's codebase patterns

**Evidence**: Disposable API analysis, build failure documentation

## Feature Matrix Summary

| Capability | v1.23 | QuantLib_huatai |
|-----------|-------|----------------|
| StandardCentral | native | native |
| ExponentialFitting | **absent** | native |
| MilevTaglianiCN | **absent** | native |
| Discrete barrier | **absent** | native |
| M-matrix diagnostics | **absent** | native |
| Multi-point mesh | partial-workaround | native |
| Band access | partial-workaround | native |
| Solver CN validation | **absent** | native |
| Disposable<> | API difference | removed |

**5 of 9 capabilities absent.** These form the core research kernel.

## Backport Analysis Summary

Minimum credible backport:
- 4 new files (transplant from QuantLib_huatai)
- 6 existing files modified
- Dependency chain depth: 5 levels
- Capabilities cannot be backported independently (linear dependency chain)
- The backport IS the upgrade, applied selectively

## Build Toolchain Finding

v1.23 fails to compile with:
- GCC 15.2.1 (template-body errors in timeseries.hpp)
- Clang 21 (same template instantiation failures)

This means v1.23 cannot even be used as a baseline without additional compiler compatibility patches — patches that v1.42-dev already includes.

## Conclusion

The evidence demonstrates that:
1. Stock v1.23 is **insufficient as-is** for the nonstandard FD scheme research
2. The minimum backport **recapitulates** the research branch's module structure
3. v1.23 has **additional build toolchain issues** with modern compilers
4. QuantLib_huatai (1.42-dev) provides the correct infrastructure natively

The recommended path is to use QuantLib_huatai (1.42-dev) as the research base.
