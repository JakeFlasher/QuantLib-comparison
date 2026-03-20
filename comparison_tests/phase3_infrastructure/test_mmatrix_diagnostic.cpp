// M-matrix diagnostic test — QuantLib_huatai only.
// Demonstrates:
// 1. The spatial operator matrix has negative off-diagonals for StandardCentral
//    at sigma=0.001, proving M-matrix violation.
// 2. ExponentialFitting has non-negative off-diagonals (M-matrix satisfied).
// 3. The FdmBlackScholesSpatialDesc with MMatrixPolicy::FallbackToExponentialFitting
//    automatically switches from StandardCentral to ExponentialFitting.

#include <ql/qldefines.hpp>
#include <ql/settings.hpp>
#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/termstructures/volatility/equityfx/blackconstantvol.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <ql/instruments/payoffs.hpp>
#include <ql/methods/finitedifferences/meshers/uniform1dmesher.hpp>
#include <ql/methods/finitedifferences/meshers/fdmmeshercomposite.hpp>
#include <ql/methods/finitedifferences/operators/fdmblackscholesop.hpp>
#include <ql/methods/finitedifferences/operators/fdmblackscholesspatialdesc.hpp>
#include <ql/methods/finitedifferences/operators/fdmlinearoplayout.hpp>
#include <ql/math/matrixutilities/sparsematrix.hpp>

#include <iostream>
#include <cmath>

using namespace QuantLib;

// Count negative off-diagonal entries in the spatial operator matrix
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
        const Real sigma = 0.001;
        const Real r     = 0.05;
        const Real K     = 100.0;
        const Real T     = 5.0 / 12.0;
        const Size xGrid = 200;

        Date today(28, March, 2004);
        Settings::instance().evaluationDate() = today;

        auto spot  = ext::make_shared<SimpleQuote>(K);
        auto qTS   = ext::make_shared<FlatForward>(today, 0.0, Actual365Fixed());
        auto rTS   = ext::make_shared<FlatForward>(today, r, Actual365Fixed());
        auto volTS = ext::make_shared<BlackConstantVol>(
            today, NullCalendar(), sigma, Actual365Fixed());

        auto process = ext::make_shared<BlackScholesMertonProcess>(
            Handle<Quote>(spot),
            Handle<YieldTermStructure>(qTS),
            Handle<YieldTermStructure>(rTS),
            Handle<BlackVolTermStructure>(volTS));

        const Real xMin = std::log(50.0);
        const Real xMax = std::log(150.0);
        auto mesher = ext::make_shared<FdmMesherComposite>(
            ext::make_shared<Uniform1dMesher>(xMin, xMax, xGrid));

        std::cout << "=== M-Matrix Diagnostic Test ===" << std::endl;
        std::cout << "sigma=" << sigma << " r=" << r << " xGrid=" << xGrid << std::endl;

        bool allPass = true;

        // Test 1: StandardCentral spatial operator — extract via toMatrixDecomp()
        {
            FdmBlackScholesSpatialDesc desc;
            desc.scheme = FdmBlackScholesSpatialDesc::Scheme::StandardCentral;
            desc.mMatrixPolicy = FdmBlackScholesSpatialDesc::MMatrixPolicy::None;

            auto op = ext::make_shared<FdmBlackScholesOp>(
                mesher, process, K, false, -Null<Real>(), 0,
                ext::shared_ptr<FdmQuantoHelper>(), desc);

            op->setTime(0.0, T/50.0);
            auto decomp = op->toMatrixDecomp();
            const SparseMatrix& spatialMat = decomp[0];

            Size negCount = countNegativeOffDiagonals(spatialMat, xGrid);

            std::cout << "\n--- StandardCentral spatial operator ---" << std::endl;
            std::cout << "  Negative off-diagonals: " << negCount << std::endl;

            if (negCount == 0) {
                std::cout << "  FAIL: Expected negative off-diagonals" << std::endl;
                allPass = false;
            } else {
                std::cout << "  PASS: " << negCount
                          << " negative off-diagonal entries (M-matrix violated)" << std::endl;
            }
        }

        // Test 2: ExponentialFitting spatial operator — should have no violations
        {
            FdmBlackScholesSpatialDesc desc;
            desc.scheme = FdmBlackScholesSpatialDesc::Scheme::ExponentialFitting;
            desc.mMatrixPolicy = FdmBlackScholesSpatialDesc::MMatrixPolicy::None;

            auto op = ext::make_shared<FdmBlackScholesOp>(
                mesher, process, K, false, -Null<Real>(), 0,
                ext::shared_ptr<FdmQuantoHelper>(), desc);

            op->setTime(0.0, T/50.0);
            auto decomp = op->toMatrixDecomp();
            const SparseMatrix& spatialMat = decomp[0];

            Size negCount = countNegativeOffDiagonals(spatialMat, xGrid);

            std::cout << "\n--- ExponentialFitting spatial operator ---" << std::endl;
            std::cout << "  Negative off-diagonals: " << negCount << std::endl;

            if (negCount > 0) {
                std::cout << "  FAIL: ExponentialFitting should have no violations" << std::endl;
                allPass = false;
            } else {
                std::cout << "  PASS: No negative off-diagonals (M-matrix satisfied)" << std::endl;
            }
        }

        // Test 3: FallbackToExponentialFitting — auto-switches when violations detected
        {
            std::cout << "\n--- FallbackToExponentialFitting policy ---" << std::endl;

            // Build truncated call with FallbackToExponentialFitting (default)
            FdmBlackScholesSpatialDesc fbDesc;
            fbDesc.scheme = FdmBlackScholesSpatialDesc::Scheme::StandardCentral;
            fbDesc.mMatrixPolicy = FdmBlackScholesSpatialDesc::MMatrixPolicy::FallbackToExponentialFitting;

            auto fbOp = ext::make_shared<FdmBlackScholesOp>(
                mesher, process, K, false, -Null<Real>(), 0,
                ext::shared_ptr<FdmQuantoHelper>(), fbDesc);

            fbOp->setTime(T/2.0, T/2.0);
            auto fbDecomp = fbOp->toMatrixDecomp();
            const SparseMatrix& fbMat = fbDecomp[0];
            Size fbNeg = countNegativeOffDiagonals(fbMat, xGrid);

            std::cout << "  After fallback, negative off-diagonals: " << fbNeg << std::endl;
            if (fbNeg == 0) {
                std::cout << "  PASS: Fallback mechanism corrected M-matrix violations" << std::endl;
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
