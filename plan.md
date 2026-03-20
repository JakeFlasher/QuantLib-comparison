# <Plan Title>

## Goal Description
<Clear, direct description of what needs to be accomplished>

## Acceptance Criteria

Following TDD philosophy, each criterion includes positive and negative tests for deterministic verification.

- AC-1: <First criterion>
  - Positive Tests (expected to PASS):
    - <Test case that should succeed when criterion is met>
    - <Another success case>
  - Negative Tests (expected to FAIL):
    - <Test case that should fail/be rejected when working correctly>
    - <Another failure/rejection case>
  - AC-1.1: <Sub-criterion if needed>
    - Positive: <...>
    - Negative: <...>
- AC-2: <Second criterion>
  - Positive Tests: <...>
  - Negative Tests: <...>
...

## Path Boundaries

Path boundaries define the acceptable range of implementation quality and choices.

### Upper Bound (Maximum Acceptable Scope)
<Affirmative description of the most comprehensive acceptable implementation>
<This represents completing the goal without over-engineering>
Example: "The implementation includes X, Y, and Z features with full test coverage"

### Lower Bound (Minimum Acceptable Scope)
<Affirmative description of the minimum viable implementation>
<This represents the least effort that still satisfies all acceptance criteria>
Example: "The implementation includes core feature X with basic validation"

### Allowed Choices
<Options that are acceptable for implementation decisions>
- Can use: <technologies, approaches, patterns that are allowed>
- Cannot use: <technologies, approaches, patterns that are prohibited>

> **Note on Deterministic Designs**: If the draft specifies a highly deterministic design with no choices (e.g., "must use JSON format", "must use algorithm X"), then the path boundaries should reflect this narrow constraint. In such cases, upper and lower bounds may converge to the same point, and "Allowed Choices" should explicitly state that the choice is fixed per the draft specification.

## Feasibility Hints and Suggestions

> **Note**: This section is for reference and understanding only. These are conceptual suggestions, not prescriptive requirements.

### Conceptual Approach
<Text description, pseudocode, or diagrams showing ONE possible implementation path>

### Relevant References
<Code paths and concepts that might be useful>
- <path/to/relevant/component> - <brief description>

## Dependencies and Sequence

### Milestones
1. <Milestone 1>: <Description>
   - Phase A: <...>
   - Phase B: <...>
2. <Milestone 2>: <Description>
   - Step 1: <...>
   - Step 2: <...>

<Describe relative dependencies between components, not time estimates>

## Task Breakdown

Each task must include exactly one routing tag:
- `coding`: implemented by Claude
- `analyze`: executed via Codex (`/humanize:ask-codex`)

| Task ID | Description | Target AC | Tag (`coding`/`analyze`) | Depends On |
|---------|-------------|-----------|----------------------------|------------|
| task1 | <...> | AC-1 | coding | - |
| task2 | <...> | AC-2 | analyze | task1 |

## Claude-Codex Deliberation

### Agreements
- <Point both sides agree on>

### Resolved Disagreements
- <Topic>: Claude vs Codex summary, chosen resolution, and rationale

### Convergence Status
- Final Status: `converged` or `partially_converged`

## Pending User Decisions

- DEC-1: <Decision topic>
  - Claude Position: <...>
  - Codex Position: <...>
  - Tradeoff Summary: <...>
  - Decision Status: `PENDING` or `<User's final decision>`

## Implementation Notes

### Code Style Requirements
- Implementation code and comments must NOT contain plan-specific terminology such as "AC-", "Milestone", "Step", "Phase", or similar workflow markers
- These terms are for plan documentation only, not for the resulting codebase
- Use descriptive, domain-appropriate naming in code instead

## Output File Convention

This template is used to produce the main output file (e.g., `plan.md`).

### Translated Language Variant

When `alternative_plan_language` resolves to a supported language name through merged config loading, a translated variant of the output file is also written after the main file. Humanize loads config from merged layers in this order: default config, optional user config, then optional project config; `alternative_plan_language` may be set at any of those layers. The variant filename is constructed by inserting `_<code>` (the ISO 639-1 code from the built-in mapping table) immediately before the file extension:

- `plan.md` becomes `plan_<code>.md` (e.g. `plan_zh.md` for Chinese, `plan_ko.md` for Korean)
- `docs/my-plan.md` becomes `docs/my-plan_<code>.md`
- `output` (no extension) becomes `output_<code>`

The translated variant file contains a full translation of the main plan file's current content in the configured language. All identifiers (`AC-*`, task IDs, file paths, API names, command flags) remain unchanged, as they are language-neutral.

When `alternative_plan_language` is empty, absent, set to `"English"`, or set to an unsupported language, no translated variant is written. Humanize does not auto-create `.humanize/config.json` when no project config file is present.

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
2. **ExponentialFitting** (Duffy 2004) - Péclet-dependent fitting factor `rho = x*coth(x)` that smoothly adapts diffusion coefficient
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
- ExponentialFitting: computes per-node Péclet numbers, calls `xCothx()`, applies fitting factor
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

13. **reproduce_paper_results.cpp**: Replicate Milev-Tagliani Example 4.1 (discrete double barrier knock-out call with K=100, L=90, U=110, sigma varies) using v1.41. Show:
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
