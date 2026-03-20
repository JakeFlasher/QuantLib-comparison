# QuantLib v1.23 vs QuantLib_huatai (1.42-dev): Systematic Comparison Demonstrating Infrastructure Necessity

## Goal Description

Demonstrate through systematic testing and analysis that stock QuantLib v1.23 does not natively support the nonstandard finite difference scheme research (Milev-Tagliani 2010, Duffy 2004) implemented in QuantLib_huatai (1.42-dev), and that the minimum credible backport substantially recapitulates the same module set and dependency structure as the research branch. The comparison evaluates three axes independently: **numerical capability** (can v1.23 compute the three spatial schemes?), **API integration** (can the research be wired into v1.23's FD framework?), and **build/toolchain compatibility** (can 1.42-dev code compile against v1.23?).

### Research Context

Three papers form the basis:
1. **Milev & Tagliani (2010)** - "Nonstandard Finite Difference Schemes with Application to Finance: Option Pricing" (Serdica Math. J., Vol. 36, pp. 75-88)
2. **Milev & Tagliani (2010)** - "Low Volatility Options and Numerical Diffusion of Finite Difference Schemes" (Serdica Math. J., Vol. 36, pp. 223-236)
3. **Duffy (2004)** - "A Critique of the Crank-Nicolson Scheme" (Wilmott Magazine, Issue 4, pp. 68-76)

Three spatial discretization schemes:
1. **StandardCentral** - Baseline centered-difference Crank-Nicolson (oscillates in low-vol regime)
2. **ExponentialFitting** (Duffy 2004) - Fitting factor `rho = x*coth(x)` adapts diffusion per Peclet number
3. **MilevTaglianiCNEffectiveDiffusion** (Milev-Tagliani 2010) - Artificial diffusion `r^2*h^2/(8*sigma^2)` preserving M-matrix positivity

### Gap Classification

**Core Research Blockers** (absent in stock v1.23, required for paper replication):
- `FdmBlackScholesSpatialDesc` + `setTime()` multi-path dispatch (scheme selection)
- `xCothx()` numerically stable helper (`fdmhyperboliccot.hpp`)
- `FdmDiscreteBarrierStepCondition` (discrete barrier monitoring class)
- `FdmMMatrixReport` + `checkOffDiagonalNonNegative()` (M-matrix diagnostics)

**Integration/Support Blockers** (absent or different, secondary evidence):
- `FdBlackScholesBarrierEngine` discrete-monitoring constructors
- `FdmBlackScholesMesher` multi-point `cPoints` constructor
- `FdmBlackScholesSolver` CN-equivalence validation gate
- `Disposable<>` API surface difference (note: `Disposable<T>` aliases to `T` by default in v1.23 unless `QL_USE_DISPOSABLE` is enabled; this is API-surface friction, not a hard compile barrier in default configuration)

**Available in v1.23 (corrections from analysis)**:
- `ModTripleBandLinearOp` exists in `ql/experimental/finitedifferences/` (band access available, but not on the main operator path)
- `FdmBlackScholesMultiStrikeMesher` exists (multi-strike workaround, different interface from `cPoints`)
- `FdmStepConditionComposite` is generic and can host new step conditions (the framework works; the specific barrier class is what's missing)

## Acceptance Criteria

Following TDD philosophy, each criterion includes positive and negative tests for deterministic verification.

### PRIMARY (gates the main conclusion)

- AC-1: v1.23 StandardCentral oscillation detection
  - Positive Tests (expected to PASS):
    - Price a truncated call with sigma=0.001, r=0.05, K=50, U=70, T=5/12, xGrid=200 on v1.23; at least 1 interior node has V < -1e-10
    - The test program compiles and runs successfully on v1.23
  - Negative Tests (expected to FAIL):
    - Attempting to switch to ExponentialFitting scheme on v1.23 (no `FdmBlackScholesSpatialDesc` type exists)
    - Achieving all-positive V values with StandardCentral under these parameters

- AC-2: QuantLib_huatai nonstandard scheme positivity
  - Positive Tests (expected to PASS):
    - Same parameters as AC-1; ExponentialFitting produces V >= 0 at ALL interior nodes
    - Same parameters as AC-1; MilevTaglianiCNEffectiveDiffusion produces V >= 0 at ALL interior nodes
    - Both schemes produce prices within 1% of a fine-grid reference solution at the strike
  - Negative Tests (expected to FAIL):
    - StandardCentral under same parameters still produces negative nodes (confirms the problem exists even on 1.42-dev)
    - A scheme producing V < -1e-10 at any interior node (nonstandard schemes must not do this)

- AC-3: Discrete double barrier replication (Milev-Tagliani Example 4.1)
  - Positive Tests (expected to PASS):
    - On QuantLib_huatai: price discrete double barrier knock-out call (K=100, L=95, U=110, 5 monitoring dates) at sigma=0.25, T=0.5 using `FdmDiscreteBarrierStepCondition` + low-level solver; all three schemes produce finite positive prices
    - On QuantLib_huatai: same at sigma=0.001, T=1.0; ExponentialFitting and MilevTaglianiCN produce all-positive grid values
  - Negative Tests (expected to FAIL):
    - On stock v1.23: `#include <ql/methods/finitedifferences/stepconditions/fdmdiscretebarrierstepcondition.hpp>` fails (file does not exist)
    - On stock v1.23: constructing a discrete barrier step condition without adding the class from scratch
    - On QuantLib_huatai at sigma=0.001: StandardCentral produces at least 1 node with V < 0 (M-matrix violation)

- AC-4: M-matrix diagnostic infrastructure
  - Positive Tests (expected to PASS):
    - On QuantLib_huatai: `checkOffDiagonalNonNegative()` on StandardCentral at sigma=0.001, xGrid=200 returns `ok=false` with `negativeLower > 0` or `negativeUpper > 0`
    - On QuantLib_huatai: same check on ExponentialFitting returns `ok=true`
  - Negative Tests (expected to FAIL):
    - On v1.23: `#include <ql/methods/finitedifferences/operators/fdmmatrixdiagnostic.hpp>` (file absent)
    - On v1.23: calling any M-matrix validation function (no such function exists)

- AC-5: Backport scope analysis
  - Positive Tests (expected to PASS):
    - Document identifies every file/module required for a minimal backport of the research kernel
    - Each missing capability has: required files, dependency chain, transplant-vs-adaptation classification
    - Conclusion demonstrates the minimum credible backport recapitulates the same module structure as the research branch
  - Negative Tests (expected to FAIL):
    - Finding a backport path that touches fewer than the core research files (spatial desc, operator, step condition, helper math, solver gating, mesher adaptation)
    - Claiming a single-file patch enables the research on v1.23

### SECONDARY (supporting evidence, does not gate conclusion)

- AC-6: API surface difference demonstration
  - Positive Tests (expected to PASS):
    - Document the `Disposable<T>` aliasing behavior in v1.23 (defaults to `T` unless `QL_USE_DISPOSABLE` enabled)
    - Show that with `QL_USE_DISPOSABLE` defined, v1.41-style `Array` returns cause signature mismatch against v1.23 base class `Disposable<Array>` returns
    - Enumerate all API surface differences between v1.23 and 1.42-dev operator hierarchy
  - Negative Tests (expected to FAIL):
    - Claiming v1.41-style code is drop-in compatible with v1.23 across all configurations
    - Compiling a v1.41-style operator subclass against v1.23 with `QL_USE_DISPOSABLE` enabled

- AC-7: Feature matrix document
  - Positive Tests (expected to PASS):
    - Feature matrix classifies each capability as `native` / `absent` / `partial-workaround`
    - All 9 capabilities from the gap classification are covered
    - Classifications are backed by concrete file paths and evidence
  - Negative Tests (expected to FAIL):
    - Classifying ExponentialFitting or MilevTaglianiCN as `native` in v1.23
    - Classifying `ModTripleBandLinearOp` as `absent` in v1.23 (it exists in experimental/)

- AC-8: Build reproducibility
  - Positive Tests (expected to PASS):
    - All QuantLib_huatai tests pass with exit code 0
    - All v1.23 absence tests correctly report missing files/types
    - Both versions build via single CMake invocation
  - Negative Tests (expected to FAIL):
    - v1.23 absence tests succeeding (would mean the capability exists)
    - QuantLib_huatai research tests failing (would indicate implementation bugs)

## Path Boundaries

### Upper Bound (Maximum Acceptable Scope)

The implementation includes all 5 primary acceptance criteria tests, all 3 secondary tests, the backport analysis document, and the feature matrix. Tests cover both low-volatility truncated call and discrete double barrier scenarios. The backport analysis includes per-capability dependency chains with transplant/adaptation classification. A comprehensive comparison report summarizes all findings with cross-references to specific test outputs.

### Lower Bound (Minimum Acceptable Scope)

The implementation includes the 5 primary acceptance criteria tests and the feature matrix. The backport analysis can be a structured list (not a full narrative) showing file counts and dependency chains. Secondary tests may be reduced to documentation-only (describing the API difference without a compiled test).

### Allowed Choices
- Can use: CMake build system, C++14/17 standard, Google Test or Boost.Test or standalone main() for test executables, Uniform1dMesher or FdmBlackScholesMesher for grid construction, direct solver path or engine API for pricing
- Can use: `ql/experimental/finitedifferences/modtriplebandlinearop.hpp` from v1.23 for band access tests
- Cannot use: modifications to either QuantLib_v1.23 or QuantLib_huatai source trees (tests are external programs linking against each library)
- Cannot use: time estimates, LOC counts as primary evidence (use structural dependency analysis instead)

## Feasibility Hints and Suggestions

### Conceptual Approach

Build a `comparison_tests/` directory at the repository root with two CMake targets: one linking against v1.23 headers/libs, one linking against QuantLib_huatai. Each test program is a standalone executable that outputs structured results (pass/fail with details).

For AC-1/AC-2 (oscillation/positivity):
```
1. Construct mesher, process, operator, solver for truncated call
2. Solve backward from T to 0
3. Iterate over interior grid nodes
4. Count nodes where V < -1e-10 (oscillation metric)
5. Report: total nodes, negative nodes, min V, max V
```

For AC-3 (discrete barrier):
```
1. On QuantLib_huatai: construct FdmDiscreteBarrierStepCondition with monitoring times
2. Assemble into FdmStepConditionComposite
3. Solve using Fdm1DimSolver
4. Report grid values and positivity
5. On v1.23: demonstrate #include failure for the step condition header
```

For AC-5 (backport analysis):
```
For each core research capability:
  1. Identify the primary file implementing it in QuantLib_huatai
  2. Trace its #include dependencies and constructor call chain
  3. Check which of those dependencies exist in v1.23
  4. Classify: direct transplant (file copy) vs adaptation (API changes needed)
  5. Summarize the dependency chain depth
```

### Relevant References
- `QuantLib_huatai/ql/methods/finitedifferences/operators/fdmblackscholesspatialdesc.hpp` - Spatial descriptor struct with 3 schemes
- `QuantLib_huatai/ql/methods/finitedifferences/operators/fdmblackscholesop.cpp` - Multi-path setTime() implementation
- `QuantLib_huatai/ql/methods/finitedifferences/operators/fdmhyperboliccot.hpp` - Numerically stable xCothx()
- `QuantLib_huatai/ql/methods/finitedifferences/operators/fdmmatrixdiagnostic.hpp` - M-matrix diagnostic
- `QuantLib_huatai/ql/methods/finitedifferences/operators/modtriplebandlinearop.hpp` - Mutable band access
- `QuantLib_huatai/ql/methods/finitedifferences/stepconditions/fdmdiscretebarrierstepcondition.hpp` - Discrete barrier class
- `QuantLib_huatai/ql/methods/finitedifferences/solvers/fdmblackscholessolver.cpp` - CN-equivalence validation
- `QuantLib_huatai/ql/pricingengines/barrier/fdblackscholesbarrierengine.cpp` - Dual-path dispatch
- `QuantLib_huatai/results/generate_data.cpp` - Existing research data generation harness
- `QuantLib_huatai/results/results_analysis.md` - Documented numerical experiments
- `QuantLib_v1.23/ql/methods/finitedifferences/operators/fdmblackscholesop.hpp` - v1.23 single-path operator
- `QuantLib_v1.23/ql/experimental/finitedifferences/modtriplebandlinearop.hpp` - v1.23 experimental band access
- `QuantLib_v1.23/ql/utilities/disposable.hpp` - Disposable<T> aliasing behavior
- `QuantLib_v1.23/ql/userconfig.hpp` - QL_USE_DISPOSABLE configuration

## Dependencies and Sequence

### Milestones

1. **Build Infrastructure**: Set up comparison test build system
   - Create `comparison_tests/CMakeLists.txt` with dual-target configuration
   - Verify both QuantLib versions can be linked against

2. **Core Research Gap Demonstration** (AC-1 through AC-4): Prove stock v1.23 lacks the numerical capability
   - Implement oscillation detection test (AC-1)
   - Implement positivity verification test (AC-2) — depends on AC-1 for parameter alignment
   - Implement discrete barrier replication test (AC-3)
   - Implement M-matrix diagnostic test (AC-4)

3. **Backport Scope Analysis** (AC-5): Demonstrate minimum backport recapitulates research branch structure
   - Trace dependency chains for each core research capability
   - Classify transplant vs adaptation for each file
   - Depends on Milestone 2 for concrete evidence of what's missing

4. **Supporting Evidence** (AC-6 through AC-8): Secondary tests and documentation
   - API surface analysis (AC-6) — independent of other milestones
   - Feature matrix (AC-7) — depends on Milestones 2 and 3 for classification evidence
   - Build reproducibility verification (AC-8) — depends on all tests being complete

5. **Final Report**: Synthesize findings into comparison report
   - Cross-reference test results with gap classification
   - Depends on all milestones

## Task Breakdown

| Task ID | Description | Target AC | Tag | Depends On |
|---------|-------------|-----------|-----|------------|
| task1 | Create CMakeLists.txt for comparison_tests with dual v1.23/1.42-dev targets | AC-8 | coding | - |
| task2 | Implement truncated call oscillation test (v1.23 StandardCentral, sigma=0.001) | AC-1 | coding | task1 |
| task3 | Implement positivity verification test (1.42-dev ExponentialFitting + MilevTaglianiCN) | AC-2 | coding | task1 |
| task4 | Implement discrete barrier replication test (1.42-dev with FdmDiscreteBarrierStepCondition) | AC-3 | coding | task1 |
| task5 | Implement v1.23 discrete barrier absence test (#include failure + no class available) | AC-3 | coding | task1 |
| task6 | Implement M-matrix diagnostic test (checkOffDiagonalNonNegative on both schemes) | AC-4 | coding | task1 |
| task7 | Implement v1.23 M-matrix diagnostic absence test | AC-4 | coding | task1 |
| task8 | Analyze dependency chains for FdmBlackScholesSpatialDesc backport | AC-5 | analyze | task2, task3 |
| task9 | Analyze dependency chains for FdmDiscreteBarrierStepCondition backport | AC-5 | analyze | task4, task5 |
| task10 | Analyze dependency chains for FdmMMatrixReport backport | AC-5 | analyze | task6, task7 |
| task11 | Analyze dependency chain for xCothx and operator setTime() restructuring | AC-5 | analyze | task3 |
| task12 | Synthesize backport analysis document from dependency chain analyses | AC-5 | coding | task8, task9, task10, task11 |
| task13 | Document Disposable<> API surface difference with QL_USE_DISPOSABLE analysis | AC-6 | coding | task1 |
| task14 | Generate feature matrix with native/absent/partial-workaround classifications | AC-7 | coding | task2, task3, task4, task6 |
| task15 | Run full build and test suite, verify reproducibility | AC-8 | coding | task2, task3, task4, task5, task6, task7, task13 |
| task16 | Generate final comparison report synthesizing all findings | AC-1 through AC-8 | coding | task12, task14, task15 |

## Claude-Codex Deliberation

### Agreements
- Missing native spatial-scheme dispatch in v1.23 is a real and critical gap
- FdmDiscreteBarrierStepCondition is absent in v1.23 as shipped
- FdmMMatrixReport / checkOffDiagonalNonNegative() are missing in v1.23
- Separating numerical capability, API integration, and build friction into three axes is correct
- Low-level solver replication is the primary path; engine API parity is secondary
- ModTripleBandLinearOp exists in v1.23 experimental/ (not absent as originally claimed)
- FdmStepConditionComposite is generic enough to host a new barrier step condition; the missing piece is the specific class, not the framework
- Qualitative backport-cost framing ("recapitulates same module structure") is appropriate; strict quantitative cost comparison requires a separate effort model

### Resolved Disagreements

- **Disposable<> severity**: Claude initially classified as "strong blocker." Codex pointed out that `Disposable<T>` aliases to `T` by default (unless `QL_USE_DISPOSABLE` is enabled). Resolution: downgraded to integration friction / API-surface difference. Not a hard compile barrier in default configuration.

- **"Infeasible" vs "absent/requires backport"**: Claude initially used "infeasible" for ExponentialFitting and MilevTaglianiCN in v1.23. Codex argued this overstates the case since a targeted backport could add them. Resolution: relabeled as "absent (requires new operator path)" — the capability is missing natively but not theoretically impossible to add.

- **Barrier engine as core blocker**: Claude treated FdBlackScholesBarrierEngine discrete dispatch as a core blocker. Codex noted the research branch uses low-level solver, not the engine API. Resolution: engine parity moved to secondary evidence; primary path uses direct solver.

- **AC thresholds**: Claude initially used arbitrary thresholds (file count >= 40, depth >= 5). Codex required concrete per-capability analysis with rationale. Resolution: AC-5 rewritten to require per-capability dependency chains and transplant/adaptation classification.

- **ModTripleBandLinearOp**: Draft claimed it was absent in v1.23. Codex correctly identified it in `ql/experimental/finitedifferences/`. Resolution: reclassified as "partial (experimental, not on main operator path)."

- **Version baseline**: Draft mixed "v1.41" and "v1.42-dev" labels. Codex required consistency. Resolution: QuantLib_huatai identified as "1.42-dev" throughout; v1.41 mentioned only as upstream provenance.

- **Milev-Tagliani barrier parameters**: Draft used L=90, U=110. Codex identified that the actual repo artifacts use L=95, U=110. Resolution: AC-3 uses K=100, L=95, U=110 matching the implementation.

### Convergence Status
- Final Status: `converged`
- Rounds: 3
- Round 1: 8 required changes (structural reclassification, thesis rewording)
- Round 2: 5 required changes (version consistency, AC measurability, scope splitting)
- Round 3: 2 required changes (parameter correction, Disposable<> recast)

## Pending User Decisions

No pending decisions. All items from Codex first-pass analysis and convergence rounds were resolved during the iterative process.

## Implementation Notes

### Code Style Requirements
- Implementation code and comments must NOT contain plan-specific terminology such as "AC-", "Milestone", "Step", "Phase", or similar workflow markers
- These terms are for plan documentation only, not for the resulting codebase
- Use descriptive, domain-appropriate naming in code instead
- Test output should use structured format: `RESULT: PASS/FAIL <description>` for automated parsing

### Build Configuration Notes
- v1.23 and QuantLib_huatai must be built separately before comparison tests
- Comparison tests link against installed headers/libraries, NOT modifying source trees
- CMake should detect QuantLib version from `ql/version.hpp` or `ql/qldefines.hpp`

### Testing Philosophy
- Each test is a standalone executable returning 0 on expected outcome
- "Expected outcome" includes expected failures (e.g., v1.23 correctly lacking a feature)
- Absence tests on v1.23 verify that specific headers/types/classes do not exist
- Presence tests on QuantLib_huatai verify research capabilities work as documented

--- Original Design Draft Start ---

# Draft: Demonstrating the Necessity of QuantLib v1.41 for Nonstandard FD Scheme Research

## Context

This project implements nonstandard finite difference schemes for the 1D Black-Scholes PDE to eliminate spurious oscillations in low-volatility and barrier option pricing. The research is based on three papers:

1. **Milev & Tagliani (2010)** - "Nonstandard Finite Difference Schemes with Application to Finance: Option Pricing" (Serdica Math. J., Vol. 36, pp. 75-88)
2. **Milev & Tagliani (2010)** - "Low Volatility Options and Numerical Diffusion of Finite Difference Schemes" (Serdica Math. J., Vol. 36, pp. 223-236)
3. **Duffy (2004)** - "A Critique of the Crank-Nicolson Scheme" (Wilmott Magazine, Issue 4, pp. 68-76)

The implementation lives in `QuantLib_huatai/` (based on QuantLib v1.41). A claim has been made that the work should be rebased onto vanilla QuantLib v1.23 (`QuantLib_v1.23/`). This plan demonstrates why that is infeasible: the v1.23 FD infrastructure lacks critical APIs, types, and architectural patterns required by the research, and there is no simpler workaround than upgrading to v1.41.

## Three Schemes Implemented

1. **StandardCentral** - Baseline centered-difference Crank-Nicolson (reference, oscillates in low-vol regime)
2. **ExponentialFitting** (Duffy 2004) - Peclet-dependent fitting factor `rho = x*coth(x)` that smoothly adapts diffusion coefficient
3. **MilevTaglianiCNEffectiveDiffusion** (Milev-Tagliani 2010) - CN variant with artificial diffusion `r^2*h^2/(8*sigma^2)` preserving positivity

## Goal

Create a systematic, test-driven comparison between QuantLib v1.23 and v1.41 that proves:
- The v1.23 FD infrastructure **cannot** support the three nonstandard schemes without massive, invasive modifications
- The v1.41 infrastructure provides the exact APIs, types, and patterns needed
- Attempting to backport onto v1.23 would require reimplementing the same upgrade path that v1.41 already provides
- The only reliable path forward is v1.41

## Critical Infrastructure Gaps in v1.23

### Gap 1: `Disposable<>` Return Type Removal (Breaking API Change)

**v1.23** uses `Disposable<Array>` return types throughout the FD operator hierarchy:
- `FdmLinearOp::apply()` returns `Disposable<array_type>`
- `FdmLinearOpComposite::apply_mixed()`, `apply_direction()`, `solve_splitting()`, `preconditioner()` all return `Disposable<Array>`
- `toMatrixDecomp()` returns `Disposable<std::vector<SparseMatrix>>`

**v1.41** returns plain `Array` and `std::vector<SparseMatrix>` directly.

**Impact**: Every operator class that inherits from `FdmLinearOpComposite` must match the base class signature. New code written for v1.41 will not compile against v1.23 virtual function declarations. This is NOT a simple find-replace; the `Disposable<>` template has move-semantic implications.

**Files affected**:
- `ql/methods/finitedifferences/operators/fdmlinearop.hpp`
- `ql/methods/finitedifferences/operators/fdmlinearopcomposite.hpp`
- `ql/methods/finitedifferences/operators/triplebandlinearop.hpp`
- All operator implementations (~20+ files)

### Gap 2: `FdmBlackScholesSpatialDesc` Does Not Exist

This is the central configuration struct enabling scheme selection:
```cpp
struct FdmBlackScholesSpatialDesc {
    enum class Scheme { StandardCentral, ExponentialFitting, MilevTaglianiCNEffectiveDiffusion };
    enum class HPolicy { Average, Min, Harmonic };
    enum class MMatrixPolicy { None, DiagnosticsOnly, FailFast, FallbackToExponentialFitting };
    // ... tuning parameters
};
```

In v1.23, `FdmBlackScholesOp` has a fixed constructor with no spatial descriptor parameter. Adding it requires modifying the constructor signature, which propagates to `FdmBlackScholesSolver`, `FdBlackScholesBarrierEngine`, and `FdBlackScholesVanillaEngine`.

### Gap 3: `TripleBandLinearOp` Lacks Mutable Access (No `ModTripleBandLinearOp`)

v1.23's `TripleBandLinearOp` uses private arrays (`lower_`, `diag_`, `upper_`) with NO public accessor. The M-matrix diagnostic infrastructure requires reading individual band coefficients. v1.41 adds `ModTripleBandLinearOp` with `lower(i)`, `diag(i)`, `upper(i)` accessors.

Without this, M-matrix compliance checking is impossible without extracting the full sparse matrix (O(n^2) storage vs O(n) band access).

### Gap 4: `FdmDiscreteBarrierStepCondition` Does Not Exist

v1.23 has NO discrete barrier monitoring capability. The Milev-Tagliani paper's central example (Example 4.1: discrete double barrier knock-out call) requires:
- A `StepCondition<Array>` that zeroes grid values outside [L,U] at monitoring dates
- Integration with `FdmStepConditionComposite`
- Precomputed knockout indices for efficiency

This is a completely new class that must be written from scratch.

### Gap 5: `FdmBlackScholesOp::setTime()` Has Single Code Path

v1.23's `setTime()` method computes a fixed set of coefficients (standard centered differences). v1.41's `setTime()` contains three distinct code paths dispatched by `spatialDesc_.scheme`:
- StandardCentral: original code path
- ExponentialFitting: computes per-node Peclet numbers, calls `xCothx()`, applies fitting factor
- MilevTaglianiCNEffectiveDiffusion: computes artificial diffusion `r^2*h^2/(8*sigma^2)`

This is not a trivial patch; it requires restructuring the entire operator assembly.

### Gap 6: `FdmBlackScholesSolver` Lacks Scheme Validation

v1.41 adds `isCrankNicolsonEquivalent1D()` static method that validates whether the time-stepping scheme (Douglas/CN) is compatible with the spatial scheme (Milev-Tagliani requires theta=0.5). v1.23 has no such validation.

### Gap 7: `FdBlackScholesBarrierEngine` Lacks Discrete Monitoring

v1.23's barrier engine:
- Inherits from `DividendBarrierOption::engine` (v1.41 uses `BarrierOption::engine`)
- Has a single constructor (no monitoring dates parameter)
- Has a single `calculate()` method (no continuous/discrete dispatch)

v1.41 adds four constructors (continuous/discrete x with/without dividends) and dual-path dispatch.

### Gap 8: `FdmBlackScholesMesher` Lacks Multi-Point Constructor

v1.41 adds a constructor accepting `std::vector<std::tuple<Real, Real, bool>> cPoints` for concentrating grid points at multiple locations (strike + barriers). v1.23 only has the single-concentration-point constructor.

### Gap 9: C++17/Modern C++ Incompatibilities

v1.23 uses:
- `boost::shared_array` (conditionally compiled)
- `QL_NOEXCEPT` macro instead of `noexcept`
- `Disposable<>` move semantics
- Old-style iterator loops (`FdmLinearOpIterator iter = layout->begin()`)

v1.41 uses:
- `std::unique_ptr<>` exclusively
- Native `noexcept`
- Direct returns with guaranteed copy elision
- Range-based for loops (`for (const auto& iter : *layout)`)

## Required Deliverables

### Phase 1: Compile-Time Incompatibility Tests (v1.23)

Write test programs that attempt to use v1.41-style APIs against v1.23 headers, demonstrating compilation failures:

1. **test_disposable_return.cpp**: Attempt to override `FdmLinearOpComposite::apply_direction()` returning `Array` instead of `Disposable<Array>` — must fail to compile on v1.23.

2. **test_spatial_desc_missing.cpp**: Attempt to construct `FdmBlackScholesOp` with a `FdmBlackScholesSpatialDesc` parameter — must fail (type doesn't exist).

3. **test_discrete_barrier_missing.cpp**: Attempt to include `fdmdiscretebarrierstepcondition.hpp` — must fail (file doesn't exist).

4. **test_mod_tripleband_missing.cpp**: Attempt to use `ModTripleBandLinearOp` — must fail (class doesn't exist).

5. **test_mesher_multipoint.cpp**: Attempt to use the multi-point `FdmBlackScholesMesher` constructor — must fail (overload doesn't exist).

### Phase 2: Runtime Behavioral Tests (v1.23 vs v1.41)

Programs that compile on BOTH versions but demonstrate behavioral differences:

6. **test_cn_oscillation_v123.cpp**: Price a truncated call option with low volatility (sigma=0.001, r=0.05) using v1.23's standard FD engine. Show spurious oscillations (negative prices near discontinuity). This proves v1.23 has the PROBLEM but lacks the SOLUTION.

7. **test_cn_oscillation_v141.cpp**: Same parameters on v1.41 with ExponentialFitting and MilevTaglianiCN schemes. Show smooth, positive solutions.

8. **test_barrier_continuous_only_v123.cpp**: Attempt to price a discrete barrier option on v1.23. Show that only continuous monitoring is available — discrete monitoring with specific dates cannot be configured.

### Phase 3: Infrastructure Depth Analysis

9. **test_operator_band_access.cpp**: On v1.23, demonstrate that `TripleBandLinearOp` provides no way to read individual band coefficients without extracting the full sparse matrix. On v1.41, show `ModTripleBandLinearOp::lower(i)` works in O(1).

10. **test_mmatrix_diagnostic.cpp**: On v1.41, run `checkOffDiagonalNonNegative()` on a StandardCentral operator with sigma=0.001. Show M-matrix violations. Then run with ExponentialFitting and show clean report. This is impossible on v1.23 (no diagnostic infrastructure).

11. **test_solver_cn_validation.cpp**: On v1.41, attempt to use MilevTaglianiCN with a non-CN time scheme (e.g., ImplicitEuler). Show the fallback mechanism. This safety net doesn't exist in v1.23.

### Phase 4: Backporting Effort Estimation

12. **backport_analysis.md**: A structured document listing every file that would need modification to backport the research from v1.41 to v1.23, with:
    - File count and LOC estimates
    - Dependency chain depth (how many files touched per change)
    - Risk assessment (each `Disposable<>` change touches a virtual function hierarchy)
    - Comparison: effort to backport vs. effort to upgrade (they converge to the same work)

### Phase 5: End-to-End Demonstration

13. **reproduce_paper_results.cpp**: Replicate Milev-Tagliani Example 4.1 (discrete double barrier knock-out call with K=100, L=95, U=110, sigma varies) using v1.41. Show:
    - StandardCentral oscillates at low sigma
    - ExponentialFitting is smooth and positive
    - MilevTaglianiCN is smooth and positive
    - All converge at O(h^2) for moderate sigma

14. **attempt_paper_results_v123.cpp**: Attempt the same example on v1.23. Show:
    - Only StandardCentral is available
    - No discrete barrier monitoring (must approximate with continuous)
    - No M-matrix diagnostics
    - No scheme switching capability
    - Result: oscillatory, potentially negative prices with no recourse

## Acceptance Criteria

1. All Phase 1 tests FAIL to compile on v1.23 (proving API incompatibility)
2. All Phase 2 tests demonstrate behavioral gaps (oscillations on v1.23, smooth on v1.41)
3. Phase 3 tests prove infrastructure depth differences (band access, diagnostics, validation)
4. Phase 4 analysis shows backporting effort >= upgrading effort
5. Phase 5 end-to-end tests replicate paper results on v1.41 and show impossibility on v1.23
6. All v1.41 tests pass with exit code 0
7. All v1.23 compile-time tests fail with expected error messages
8. Results are reproducible via a single `make` or `cmake` invocation in each version directory

## File Structure

```
QuantLib_comparison/
  comparison_tests/
    CMakeLists.txt                    # Build system for all comparison tests
    phase1_compile_tests/
      test_disposable_return.cpp
      test_spatial_desc_missing.cpp
      test_discrete_barrier_missing.cpp
      test_mod_tripleband_missing.cpp
      test_mesher_multipoint.cpp
    phase2_runtime_tests/
      test_cn_oscillation.cpp         # Parameterized for both versions
      test_barrier_monitoring.cpp
    phase3_infrastructure/
      test_operator_band_access.cpp
      test_mmatrix_diagnostic.cpp
      test_solver_cn_validation.cpp
    phase4_analysis/
      backport_analysis.md
    phase5_endtoend/
      reproduce_paper_results.cpp
      attempt_paper_results_v123.cpp
    results/
      comparison_report.md            # Auto-generated summary
```

--- Original Design Draft End ---
