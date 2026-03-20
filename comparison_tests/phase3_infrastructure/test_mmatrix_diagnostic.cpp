// M-matrix diagnostic test (QuantLib_huatai only).
// Demonstrates M-matrix violations on StandardCentral at sigma=0.001
// by extracting the sparse matrix and checking off-diagonal signs.
// Shows ExponentialFitting has no violations.

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

#include <iostream>
#include <cmath>

using namespace QuantLib;

struct MMatrixCheck {
    Size negativeLower = 0;
    Size negativeUpper = 0;
    Real minOffDiag = 0.0;
};

MMatrixCheck checkMMatrix(const SparseMatrix& mat, Size n) {
    MMatrixCheck result;
    result.minOffDiag = 0.0;
    for (Size i = 1; i < n - 1; ++i) {  // interior rows
        // Lower off-diagonal: mat(i, i-1)
        Real lower = mat(i, i-1);
        if (lower < -1e-15) {
            result.negativeLower++;
            result.minOffDiag = std::min(result.minOffDiag, lower);
        }
        // Upper off-diagonal: mat(i, i+1)
        Real upper = mat(i, i+1);
        if (upper < -1e-15) {
            result.negativeUpper++;
            result.minOffDiag = std::min(result.minOffDiag, upper);
        }
    }
    return result;
}

int main() {
    try {
        const Real sigma = 0.001;
        const Real r     = 0.05;
        const Real K     = 100.0;
        const Size xGrid = 200;

        Date today(28, March, 2004);
        Settings::instance().evaluationDate() = today;

        auto spot  = ext::make_shared<SimpleQuote>(K);
        auto qTS   = ext::make_shared<FlatForward>(today, 0.0, Actual365Fixed());
        auto rTS   = ext::make_shared<FlatForward>(today, r, Actual365Fixed());
        auto volTS = ext::make_shared<BlackConstantVol>(today, NullCalendar(),
                                                        sigma, Actual365Fixed());

        auto process = ext::make_shared<BlackScholesMertonProcess>(
            Handle<Quote>(spot),
            Handle<YieldTermStructure>(qTS),
            Handle<YieldTermStructure>(rTS),
            Handle<BlackVolTermStructure>(volTS));

        const Real xMin = std::log(50.0);
        const Real xMax = std::log(150.0);
        auto mesher1d = ext::make_shared<Uniform1dMesher>(xMin, xMax, xGrid);
        auto mesher = ext::make_shared<FdmMesherComposite>(mesher1d);

        std::cout << "=== M-Matrix Diagnostic Test ===" << std::endl;
        std::cout << "sigma=" << sigma << " r=" << r << " xGrid=" << xGrid << std::endl;

        bool allPass = true;

        // Test 1: StandardCentral - extract sparse matrix, check off-diagonals
        {
            auto desc = FdmBlackScholesSpatialDesc::standard();
            auto op = ext::make_shared<FdmBlackScholesOp>(
                mesher, process, K, false, -Null<Real>(), 0,
                ext::shared_ptr<FdmQuantoHelper>(), desc);

            op->setTime(0.0, 0.0);
            SparseMatrix mat = op->toMatrix();

            auto check = checkMMatrix(mat, xGrid);

            std::cout << "\n--- StandardCentral ---" << std::endl;
            std::cout << "  negativeLower: " << check.negativeLower << std::endl;
            std::cout << "  negativeUpper: " << check.negativeUpper << std::endl;
            std::cout << "  minOffDiag: " << check.minOffDiag << std::endl;

            bool hasViolation = (check.negativeLower > 0 || check.negativeUpper > 0);
            if (!hasViolation) {
                std::cout << "  FAIL: Expected M-matrix violation for StandardCentral" << std::endl;
                allPass = false;
            } else {
                std::cout << "  PASS: M-matrix violation detected ("
                          << check.negativeLower << " lower, "
                          << check.negativeUpper << " upper)" << std::endl;
            }
        }

        // Test 2: ExponentialFitting - should have no violations
        {
            auto desc = FdmBlackScholesSpatialDesc::exponentialFitting();
            auto op = ext::make_shared<FdmBlackScholesOp>(
                mesher, process, K, false, -Null<Real>(), 0,
                ext::shared_ptr<FdmQuantoHelper>(), desc);

            op->setTime(0.0, 0.0);
            SparseMatrix mat = op->toMatrix();

            auto check = checkMMatrix(mat, xGrid);

            std::cout << "\n--- ExponentialFitting ---" << std::endl;
            std::cout << "  negativeLower: " << check.negativeLower << std::endl;
            std::cout << "  negativeUpper: " << check.negativeUpper << std::endl;
            std::cout << "  minOffDiag: " << check.minOffDiag << std::endl;

            bool hasViolation = (check.negativeLower > 0 || check.negativeUpper > 0);
            if (hasViolation) {
                std::cout << "  FAIL: ExponentialFitting should not have violations" << std::endl;
                allPass = false;
            } else {
                std::cout << "  PASS: M-matrix property satisfied" << std::endl;
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
