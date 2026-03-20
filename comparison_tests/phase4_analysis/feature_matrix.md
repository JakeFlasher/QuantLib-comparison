# Feature Matrix: QuantLib v1.23 vs QuantLib_huatai (1.42-dev)

## Classification Legend

- **native**: Available out-of-the-box, no modifications needed
- **absent**: Not present; must be written from scratch or transplanted
- **partial-workaround**: Some capability exists via alternative interface or experimental code

## Feature Comparison

| # | Capability | v1.23 Status | QuantLib_huatai Status | Evidence |
|---|-----------|-------------|----------------------|----------|
| 1 | StandardCentral FD scheme | native | native | `ql/methods/finitedifferences/operators/fdmblackscholesop.cpp` — single code path, always StandardCentral |
| 2 | ExponentialFitting FD scheme | **absent** | native | v1.23: `setTime()` has no scheme dispatch; no `FdmBlackScholesSpatialDesc` type. v1.42-dev: `fdmblackscholesspatialdesc.hpp` defines `Scheme::ExponentialFitting` |
| 3 | MilevTaglianiCNEffectiveDiffusion scheme | **absent** | native | Same as #2; requires `FdmBlackScholesSpatialDesc`, multi-path `setTime()`, and `xCothx()` helper |
| 4 | Discrete barrier step condition | **absent** | native | v1.23: `fdmdiscretebarrierstepcondition.hpp` does not exist. v1.42-dev: full implementation with monitoring times and rebate support |
| 5 | M-matrix diagnostics | **absent** | native | v1.23: `fdmmatrixdiagnostic.hpp` does not exist, no `FdmMMatrixReport` struct, no `checkOffDiagonalNonNegative()` function. v1.42-dev: complete diagnostic with fallback policy |
| 6 | Multi-point mesh concentration | **partial-workaround** | native | v1.23: `FdmBlackScholesMesher` has single `cPoint` parameter; `FdmBlackScholesMultiStrikeMesher` exists as workaround (different interface). Can also use `Uniform1dMesher` explicitly. v1.42-dev: `cPoints` vector constructor |
| 7 | Operator band access | **partial-workaround** | native | v1.23: `ModTripleBandLinearOp` exists in `ql/experimental/finitedifferences/` with `lower(i)`, `diag(i)`, `upper(i)` accessors. Not on main FD operator path. v1.42-dev: promoted to main `ql/methods/finitedifferences/operators/` |
| 8 | Solver CN-equivalence validation | **absent** | native | v1.23: no `isCrankNicolsonEquivalent1D()`. v1.42-dev: `fdmblackscholessolver.cpp` validates Milev-Tagliani requires CN-equivalent time stepping |
| 9 | Disposable<> API compatibility | present (aliased) | removed | v1.23: `Disposable<T>` aliases to `T` by default (`ql/utilities/disposable.hpp` line 34). With `QL_USE_DISPOSABLE` enabled, it's a distinct class with move semantics. v1.42-dev: `Disposable<>` fully removed, direct returns everywhere |

## Summary Statistics

| Classification | Count | Capabilities |
|---------------|-------|-------------|
| native (both) | 1 | StandardCentral |
| absent in v1.23 | 5 | ExponentialFitting, MilevTaglianiCN, Discrete barrier, M-matrix diagnostics, Solver CN validation |
| partial-workaround | 2 | Multi-point mesh, Operator band access |
| API difference | 1 | Disposable<> |

## Key Insight

5 of 9 capabilities are **absent** in stock v1.23. These 5 form the core research kernel: without scheme selection, discrete barrier monitoring, and M-matrix diagnostics, the papers (Milev-Tagliani 2010, Duffy 2004) cannot be replicated. The 2 partial-workaround capabilities (mesh concentration, band access) have alternative paths in v1.23 but with different interfaces.
