# Disposable<> API Surface Analysis

## Overview

The `Disposable<T>` template is a move-optimization wrapper used in QuantLib v1.23 for returning large objects (Array, SparseMatrix) from functions. In v1.42-dev (QuantLib_huatai), it has been completely removed in favor of direct returns with C++11/17 guaranteed copy/move elision.

## v1.23 Behavior

### Default Configuration (QL_USE_DISPOSABLE NOT defined)

Source: `QuantLib_v1.23/ql/utilities/disposable.hpp`, line 34:
```cpp
template <class T>
using Disposable = T;
```

When `QL_USE_DISPOSABLE` is **not** defined (the default), `Disposable<T>` is simply an alias for `T`. This means:
- `Disposable<Array>` = `Array`
- `Disposable<SparseMatrix>` = `SparseMatrix`
- Virtual function signatures using `Disposable<Array>` are ABI-compatible with `Array` returns

**Implication**: In default configuration, v1.41-style code returning `Array` directly **will compile** against v1.23 headers. This is NOT a hard compile barrier.

### With QL_USE_DISPOSABLE Defined

Source: `QuantLib_v1.23/ql/userconfig.hpp`, line 111:
```cpp
//#    define QL_USE_DISPOSABLE
```

When enabled (by uncommenting), `Disposable<T>` becomes a distinct class inheriting from `T`:
```cpp
template <class T>
class Disposable : public T {
    Disposable(T& t);
    Disposable(const Disposable<T>& t);
    Disposable<T>& operator=(const Disposable<T>& t);
};
```

With this enabled, `Disposable<Array>` ≠ `Array`, and:
- Virtual function override returning `Array` instead of `Disposable<Array>` WILL fail
- The return type must exactly match the base class signature

## v1.42-dev Behavior

`Disposable<>` is completely removed. All functions return `Array`, `SparseMatrix`, etc. directly. The `ql/utilities/disposable.hpp` header may not even exist.

## Impact Assessment

| Scenario | Compile Impact | Risk Level |
|----------|---------------|------------|
| Default v1.23 (no QL_USE_DISPOSABLE) | No compile barrier — alias makes types identical | Low |
| v1.23 with QL_USE_DISPOSABLE | Hard compile failure for mismatched return types | High |
| v1.42-dev code against v1.23 default | Compatible at type level, but older API conventions differ | Low-Medium |

## Affected Files in v1.23

Virtual functions using `Disposable<>` return types:
- `ql/methods/finitedifferences/operators/fdmlinearop.hpp` — `apply()`, `toMatrix()`
- `ql/methods/finitedifferences/operators/fdmlinearopcomposite.hpp` — `apply_mixed()`, `apply_direction()`, `solve_splitting()`, `preconditioner()`, `toMatrixDecomp()`
- `ql/methods/finitedifferences/operators/triplebandlinearop.hpp` — `apply()`, `mult()`, `multR()`, `add()`, `toMatrix()`
- All concrete operator implementations (~20+ files)

## Conclusion

The `Disposable<>` difference is **API-surface friction**, not a core research blocker. In the default v1.23 configuration, it poses no compile barrier. However, it remains a significant difference between the two codebases and contributes to the overall integration effort of any backport attempt.
