# Backport Scope Analysis: QuantLib_huatai (1.42-dev) Research Kernel to v1.23

## Methodology

For each core research capability absent in v1.23, we trace:
1. The primary implementation file(s) in QuantLib_huatai
2. Direct #include dependencies
3. Constructor/call chain dependencies
4. Whether each dependency exists in v1.23 (transplant) or requires adaptation
5. The dependency chain depth

## Capability 1: Spatial Scheme Dispatch (FdmBlackScholesSpatialDesc + setTime())

### Primary Files
- `ql/methods/finitedifferences/operators/fdmblackscholesspatialdesc.hpp` — NEW file, defines Scheme enum, HPolicy, MMatrixPolicy, tuning parameters, factory methods
- `ql/methods/finitedifferences/operators/fdmblackscholesop.cpp` — MODIFIED, `setTime()` restructured from 1 code path to 3

### Dependencies
| File | v1.23 Status | Classification |
|------|-------------|---------------|
| `fdmblackscholesspatialdesc.hpp` | absent | direct transplant (header-only) |
| `fdmhyperboliccot.hpp` (xCothx) | absent | direct transplant (header-only) |
| `fdmblackscholesop.hpp` | present, different signature | **adaptation** — add `spatialDesc` parameter to constructor |
| `fdmblackscholesop.cpp` | present, single code path | **adaptation** — add 2 new code paths + scheme dispatch |
| `fdmblackscholessolver.hpp` | present, different signature | **adaptation** — add `spatialDesc` parameter |
| `fdmblackscholessolver.cpp` | present | **adaptation** — add CN-equivalence validation |

### Dependency Chain Depth: 4
`FdmBlackScholesSpatialDesc` → `FdmBlackScholesOp` constructor → `FdmBlackScholesSolver` constructor → pricing engine constructors

### Backport Verdict
**Requires adaptation of 4 existing files + 2 new header transplants.** The operator `setTime()` restructuring is the most invasive change — it modifies the core coefficient computation loop that all FD pricing depends on.

---

## Capability 2: FdmDiscreteBarrierStepCondition

### Primary Files
- `ql/methods/finitedifferences/stepconditions/fdmdiscretebarrierstepcondition.hpp` — NEW
- `ql/methods/finitedifferences/stepconditions/fdmdiscretebarrierstepcondition.cpp` — NEW

### Dependencies
| File | v1.23 Status | Classification |
|------|-------------|---------------|
| `fdmdiscretebarrierstepcondition.hpp` | absent | direct transplant |
| `fdmdiscretebarrierstepcondition.cpp` | absent | direct transplant |
| `ql/methods/finitedifferences/stepconditions/fdmstepconditioncomposite.hpp` | present | compatible (generic framework) |
| `ql/methods/finitedifferences/meshers/fdmmesher.hpp` | present | compatible |
| `ql/methods/finitedifferences/utilities/fdmquantohelper.hpp` | present | compatible |

### Dependency Chain Depth: 2
`FdmDiscreteBarrierStepCondition` → `FdmStepConditionComposite` → solver rollback

### Backport Verdict
**Cleanest transplant.** The step condition class itself can be copied directly. However, to actually USE it requires modifications to the barrier engine (Capability 4 below) or a standalone test harness. The `FdmStepConditionComposite` framework in v1.23 is generic enough to host it.

---

## Capability 3: M-Matrix Diagnostics (FdmMMatrixReport + checkOffDiagonalNonNegative)

### Primary Files
- `ql/methods/finitedifferences/operators/fdmmatrixdiagnostic.hpp` — NEW (header-only)
- `ql/methods/finitedifferences/operators/modtriplebandlinearop.hpp` — exists in `ql/experimental/` in v1.23

### Dependencies
| File | v1.23 Status | Classification |
|------|-------------|---------------|
| `fdmmatrixdiagnostic.hpp` | absent | direct transplant (header-only) |
| `modtriplebandlinearop.hpp` | present in `ql/experimental/finitedifferences/` | move to main path (or include from experimental) |
| `fdmblackscholesop.cpp` | present | **adaptation** — add M-matrix check calls inside `setTime()` |
| `fdmblackscholesspatialdesc.hpp` | absent | already required by Capability 1 |

### Dependency Chain Depth: 3
`FdmMMatrixReport` → `ModTripleBandLinearOp` → `FdmBlackScholesOp::setTime()` integration

### Backport Verdict
**Transplant + integration.** The diagnostic header can be copied, and `ModTripleBandLinearOp` already exists in v1.23 experimental/. But integrating diagnostics into the operator requires the spatial descriptor (Capability 1) to be present first. This is a **one-way dependency**: Capability 3 depends on Capability 1, but Capability 1 does not depend on Capability 3.

---

## Capability 4: Barrier Engine Discrete Monitoring

### Primary Files
- `ql/pricingengines/barrier/fdblackscholesbarrierengine.hpp` — MODIFIED
- `ql/pricingengines/barrier/fdblackscholesbarrierengine.cpp` — MODIFIED

### Dependencies
| File | v1.23 Status | Classification |
|------|-------------|---------------|
| `fdblackscholesbarrierengine.hpp` | present, different API | **adaptation** — add 2 new constructors, dual-path dispatch |
| `fdblackscholesbarrierengine.cpp` | present, continuous-only | **adaptation** — add `calculateDiscrete()` method |
| `fdmdiscretebarrierstepcondition.hpp` | absent | requires Capability 2 |
| `fdmblackscholessolver.hpp` | present | requires Capability 1 modifications |
| `ql/instruments/barrieroption.hpp` | present | base class change: `DividendBarrierOption::engine` → `BarrierOption::engine` |

### Dependency Chain Depth: 5
Engine constructors → `FdmBlackScholesSolver` → `FdmBlackScholesOp` → `FdmBlackScholesSpatialDesc` → `FdmDiscreteBarrierStepCondition`

### Backport Verdict
**Deepest dependency chain.** Every layer from engine to operator must be modified. The base class change (`DividendBarrierOption::engine` → `BarrierOption::engine`) may affect other engines.

---

## Capability 5: xCothx Numerically Stable Helper

### Primary Files
- `ql/methods/finitedifferences/operators/fdmhyperboliccot.hpp` — NEW (header-only, ~30 lines)

### Dependencies
| File | v1.23 Status | Classification |
|------|-------------|---------------|
| `fdmhyperboliccot.hpp` | absent | direct transplant |
| No external dependencies | — | standalone inline function |

### Dependency Chain Depth: 1
Standalone utility, but required by Capability 1 (ExponentialFitting code path in `setTime()`).

### Backport Verdict
**Trivial transplant.** Single header file with no dependencies. However, it has no value without Capability 1 (scheme dispatch) since it's only called from the ExponentialFitting code path.

---

## Capability 6: FdmBlackScholesMesher Multi-Point Concentration

### Primary Files
- `ql/methods/finitedifferences/meshers/fdmblackscholesmesher.hpp` — MODIFIED in v1.42-dev
- `ql/methods/finitedifferences/meshers/fdmblackscholesmesher.cpp` — MODIFIED in v1.42-dev

### What Changed
v1.23 has a single `cPoint` parameter (`std::pair<Real, Real>`). v1.42-dev adds a second constructor taking `std::vector<std::tuple<Real, Real, bool>> cPoints` for concentrating grid points at multiple locations (strike + barriers simultaneously).

### Dependencies
| File | v1.23 Status | Classification |
|------|-------------|---------------|
| `fdmblackscholesmesher.hpp` | present, different API | **adaptation** — add second constructor overload |
| `fdmblackscholesmesher.cpp` | present | **adaptation** — add multi-point grid concentration logic |

### Dependency Chain Depth: 1
The mesher is a leaf dependency — used by engines and solvers but depends only on `Fdm1dMesher` and `Concentrating1dMesher`, both of which exist in v1.23.

### Backport Verdict
**Adaptation of 2 existing files.** The new constructor requires a new grid-building loop that handles multiple concentration points. The underlying `Concentrating1dMesher` exists in v1.23, so the adaptation is bounded.

---

## Aggregate Analysis

### Total Files Touched

| Classification | Count | Files |
|---------------|-------|-------|
| New files (transplant) | 5 | `fdmblackscholesspatialdesc.hpp`, `fdmhyperboliccot.hpp`, `fdmmatrixdiagnostic.hpp`, `fdmdiscretebarrierstepcondition.hpp`, `fdmdiscretebarrierstepcondition.cpp` |
| Existing files (adaptation) | 8 | `fdmblackscholesop.{hpp,cpp}`, `fdmblackscholessolver.{hpp,cpp}`, `fdblackscholesbarrierengine.{hpp,cpp}`, `fdmblackscholesmesher.{hpp,cpp}` |
| **Total** | **13** | — |

### Dependency Structure

```
fdmblackscholesspatialdesc.hpp (NEW)
├── fdmhyperboliccot.hpp (NEW, standalone)
├── fdmmatrixdiagnostic.hpp (NEW, depends on ModTripleBandLinearOp)
├── fdmblackscholesop.hpp (ADAPT constructor)
│   └── fdmblackscholesop.cpp (ADAPT setTime() — 3 code paths, M-matrix integration)
│       └── fdmblackscholessolver.hpp (ADAPT constructor)
│           └── fdmblackscholessolver.cpp (ADAPT CN-equivalence validation)
│               └── fdblackscholesbarrierengine.hpp (ADAPT 4 constructors)
│                   └── fdblackscholesbarrierengine.cpp (ADAPT dual-path dispatch)
│                       └── fdmdiscretebarrierstepcondition.hpp (NEW)
│                           └── fdmdiscretebarrierstepcondition.cpp (NEW)
```

### Key Observation

The dependency chain is **linear and deep** (depth 5 from spatial descriptor to engine). Each capability depends on the one above it:
- xCothx needs scheme dispatch (Capability 1) to be called
- M-matrix diagnostics need scheme dispatch to know when to check
- Discrete barrier needs the solver modifications from Capability 1
- Engine discrete monitoring needs all of the above

**This means the capabilities cannot be backported independently.** A "minimal" backport that only adds discrete barrier monitoring still requires the spatial descriptor, operator modifications, and solver changes — which is most of the work.

## Conclusion

The minimum credible backport to enable paper replication on v1.23 requires:
- **5 new physical files** (transplant from QuantLib_huatai)
- **8 existing files modified** (constructor signatures, new code paths, dispatch logic, mesher adaptation)
- **13 total files touched**
- **Dependency chain depth of 5** (spatial desc → operator → solver → engine → step condition)
- **Mesher adaptation** adds 2 more files for multi-point grid concentration

This substantially recapitulates the same module set and dependency structure as the QuantLib_huatai research branch. The backport IS the upgrade, just applied selectively rather than wholesale.

### Additional Build Toolchain Note

During this analysis, we discovered that **v1.23 does not compile with modern GCC 15 or Clang 21** due to template compatibility issues in `timeseries.hpp`. The QuantLib_huatai (1.42-dev) codebase has already addressed these compiler compatibility issues. This adds a toolchain dimension to the backport effort that goes beyond the FD research kernel.
