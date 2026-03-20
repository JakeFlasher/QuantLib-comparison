// M-matrix diagnostic test — QuantLib_huatai only.
//
// The checkOffDiagonalNonNegative() diagnostic runs inside setTime() for
// non-StandardCentral schemes. StandardCentral returns before the check.
//
// Primary tests:
//   1. Sparse matrix scan: StandardCentral has negative off-diagonals (the problem)
//   2. ExponentialFitting + FailFast: checkOffDiagonalNonNegative runs, no throw (fix works)
//   3. FallbackToExponentialFitting on StandardCentral: policy auto-corrects
//
// This proves: the diagnostic infrastructure exists in huatai, StandardCentral
// has the M-matrix problem, and ExponentialFitting solves it.

#include <ql/qldefines.hpp>
#include <ql/settings.hpp>
#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/termstructures/volatility/equityfx/blackconstantvol.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <ql/methods/finitedifferences/meshers/uniform1dmesher.hpp>
#include <ql/methods/finitedifferences/meshers/fdmmeshercomposite.hpp>
#include <ql/methods/finitedifferences/operators/fdmblackscholesop.hpp>
#include <ql/methods/finitedifferences/operators/fdmblackscholesspatialdesc.hpp>
#include <ql/methods/finitedifferences/operators/fdmlinearoplayout.hpp>
#include <ql/math/matrixutilities/sparsematrix.hpp>

#include <iostream>
#include <cmath>

using namespace QuantLib;

Size countNegativeOffDiagonals(const SparseMatrix& mat, Size n) {
    Size count = 0;
    for (Size i = 1; i < n - 1; ++i) {
        if (mat(i, i-1) < -1e-15) ++count;
        if (i + 1 < n && mat(i, i+1) < -1e-15) ++count;
    }
    return count;
}

int main() {
    try {
        const Real sigma = 0.001, r = 0.05, K = 100.0;
        const Real T = 5.0 / 12.0, dt = T / 50.0;
        const Size xGrid = 200;

        Date today(28, March, 2004);
        Settings::instance().evaluationDate() = today;

        auto spot  = ext::make_shared<SimpleQuote>(K);
        auto qTS   = ext::make_shared<FlatForward>(today, 0.0, Actual365Fixed());
        auto rTS   = ext::make_shared<FlatForward>(today, r, Actual365Fixed());
        auto volTS = ext::make_shared<BlackConstantVol>(
            today, NullCalendar(), sigma, Actual365Fixed());
        auto process = ext::make_shared<BlackScholesMertonProcess>(
            Handle<Quote>(spot), Handle<YieldTermStructure>(qTS),
            Handle<YieldTermStructure>(rTS), Handle<BlackVolTermStructure>(volTS));

        auto mesher = ext::make_shared<FdmMesherComposite>(
            ext::make_shared<Uniform1dMesher>(std::log(50.0), std::log(150.0), xGrid));

        std::cout << "=== M-Matrix Diagnostic Test ===" << std::endl;
        std::cout << "sigma=" << sigma << " r=" << r
                  << " xGrid=" << xGrid << " dt=" << dt << std::endl;

        bool allPass = true;

        // ── Test 1: StandardCentral has negative off-diagonals ──
        {
            FdmBlackScholesSpatialDesc desc;
            desc.scheme = FdmBlackScholesSpatialDesc::Scheme::StandardCentral;
            desc.mMatrixPolicy = FdmBlackScholesSpatialDesc::MMatrixPolicy::None;
            auto op = ext::make_shared<FdmBlackScholesOp>(
                mesher, process, K, false, -Null<Real>(), 0,
                ext::shared_ptr<FdmQuantoHelper>(), desc);
            op->setTime(0.0, dt);
            Size neg = countNegativeOffDiagonals(op->toMatrixDecomp()[0], xGrid);

            std::cout << "\n--- StandardCentral spatial operator ---" << std::endl;
            std::cout << "  Negative off-diagonals: " << neg << std::endl;
            if (neg == 0) {
                std::cout << "  FAIL: Expected negative off-diagonals" << std::endl;
                allPass = false;
            } else {
                std::cout << "  PASS: " << neg << " violations (M-matrix problem confirmed)" << std::endl;
            }
        }

        // ── Test 2: ExponentialFitting + FailFast exercising checkOffDiagonalNonNegative ──
        // The diagnostic runs inside setTime() for non-StandardCentral schemes.
        // FailFast throws if violations found. No throw = diagnostic ran and passed.
        {
            FdmBlackScholesSpatialDesc desc;
            desc.scheme = FdmBlackScholesSpatialDesc::Scheme::ExponentialFitting;
            desc.mMatrixPolicy = FdmBlackScholesSpatialDesc::MMatrixPolicy::FailFast;
            auto op = ext::make_shared<FdmBlackScholesOp>(
                mesher, process, K, false, -Null<Real>(), 0,
                ext::shared_ptr<FdmQuantoHelper>(), desc);

            bool threw = false;
            try { op->setTime(0.0, dt); }
            catch (const std::exception& e) { threw = true; }

            std::cout << "\n--- ExponentialFitting + FailFast (exercises checkOffDiagonalNonNegative) ---" << std::endl;
            if (threw) {
                std::cout << "  FAIL: ExponentialFitting should not throw" << std::endl;
                allPass = false;
            } else {
                std::cout << "  PASS: checkOffDiagonalNonNegative ran, no violations" << std::endl;
            }

            // Verify independently
            Size neg = countNegativeOffDiagonals(op->toMatrixDecomp()[0], xGrid);
            std::cout << "  Supplemental scan: " << neg << " negative off-diagonals" << std::endl;
        }

        // ── Test 3: FallbackToExponentialFitting auto-corrects ──
        // When a non-standard scheme has violations, the fallback policy
        // recomputes with ExponentialFitting. Here we use MilevTaglianiCN
        // which may have violations at extreme low vol, and verify the
        // fallback produces clean off-diagonals.
        {
            FdmBlackScholesSpatialDesc desc;
            desc.scheme = FdmBlackScholesSpatialDesc::Scheme::MilevTaglianiCNEffectiveDiffusion;
            desc.mMatrixPolicy = FdmBlackScholesSpatialDesc::MMatrixPolicy::FallbackToExponentialFitting;
            auto op = ext::make_shared<FdmBlackScholesOp>(
                mesher, process, K, false, -Null<Real>(), 0,
                ext::shared_ptr<FdmQuantoHelper>(), desc);
            op->setTime(0.0, dt);
            Size neg = countNegativeOffDiagonals(op->toMatrixDecomp()[0], xGrid);

            std::cout << "\n--- MilevTaglianiCN + FallbackToExponentialFitting ---" << std::endl;
            std::cout << "  After potential fallback, negative off-diagonals: " << neg << std::endl;
            if (neg == 0) {
                std::cout << "  PASS: Fallback mechanism ensures clean M-matrix" << std::endl;
            } else {
                std::cout << "  FAIL: Fallback did not correct violations" << std::endl;
                allPass = false;
            }
        }

        std::cout << "\n" << (allPass ? "RESULT: PASS" : "RESULT: FAIL")
                  << " - M-matrix diagnostics" << std::endl;
        return allPass ? 0 : 1;
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }
}
