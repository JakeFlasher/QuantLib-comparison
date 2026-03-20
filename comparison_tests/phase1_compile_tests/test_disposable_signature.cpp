// Verify that with QL_USE_DISPOSABLE enabled, v1.41-style Array returns
// cause signature mismatch against v1.23 base class Disposable<Array>.
// Expected: compilation failure (return type override mismatch).
#define QL_USE_DISPOSABLE
#include <ql/methods/finitedifferences/operators/fdmlinearopcomposite.hpp>
#include <ql/math/array.hpp>

using namespace QuantLib;

// Attempt to derive from v1.23 FdmLinearOpComposite with v1.41-style
// direct Array returns (not Disposable<Array>)
class TestOp : public FdmLinearOpComposite {
public:
    Size size() const override { return 0; }
    void setTime(Time t1, Time t2) override {}

    // These return Array, but v1.23 base class with QL_USE_DISPOSABLE
    // requires Disposable<Array> — signature mismatch
    Array apply(const Array& r) const override { return r; }
    Array apply_mixed(const Array& r) const override { return r; }
    Array apply_direction(Size, const Array& r) const override { return r; }
    Array solve_splitting(Size, const Array& r, Real) const override { return r; }
    Array preconditioner(const Array& r, Real) const override { return r; }
    std::vector<SparseMatrix> toMatrixDecomp() const override { return {}; }
};

int main() {
    TestOp op;
    return 0;
}
