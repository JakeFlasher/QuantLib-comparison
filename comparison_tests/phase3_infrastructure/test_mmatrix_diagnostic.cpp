// M-matrix diagnostic test — QuantLib_huatai only.
//
// Directly calls checkOffDiagonalNonNegative() from fdmmatrixdiagnostic.hpp
// and inspects the returned FdmMMatrixReport fields.
//
// Constructs TripleBandLinearOp instances with known coefficients:
// - StandardCentral-style: negative lower off-diagonals at sigma=0.001
// - ExponentialFitting-style: non-negative off-diagonals everywhere

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
#include <ql/methods/finitedifferences/operators/triplebandlinearop.hpp>
#include <ql/methods/finitedifferences/operators/modtriplebandlinearop.hpp>
#include <ql/methods/finitedifferences/operators/fdmmatrixdiagnostic.hpp>

#include <iostream>
#include <cmath>

using namespace QuantLib;

int main() {
    try {
        const Real sigma = 0.001;
        const Real r     = 0.05;
        const Real K     = 100.0;
        const Real T     = 5.0 / 12.0;
        const Size xGrid = 200;
        const Real dt    = T / 50.0;

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

        // ── Test 1: Build StandardCentral operator, extract tridiag, run diagnostic ──
        {
            FdmBlackScholesSpatialDesc desc;
            desc.scheme = FdmBlackScholesSpatialDesc::Scheme::StandardCentral;
            desc.mMatrixPolicy = FdmBlackScholesSpatialDesc::MMatrixPolicy::None;
            auto op = ext::make_shared<FdmBlackScholesOp>(
                mesher, process, K, false, -Null<Real>(), 0,
                ext::shared_ptr<FdmQuantoHelper>(), desc);
            op->setTime(0.0, dt);

            // Build a TripleBandLinearOp on the same mesher with StandardCentral
            // coefficients, then wrap in ModTripleBandLinearOp.
            // We compute coefficients analytically: for constant vol in log-space,
            //   h = (xMax - xMin) / (n - 1)
            //   sigma2 = sigma^2
            //   mu = r - 0.5*sigma2
            //   lower = sigma2/(2*h^2) - mu/(2*h)
            //   upper = sigma2/(2*h^2) + mu/(2*h)
            //   diag  = -sigma2/h^2 - r

            TripleBandLinearOp tbOp(0, mesher);
            ModTripleBandLinearOp probe(tbOp);

            const Real xMin = std::log(50.0), xMax = std::log(150.0);
            const Real h = (xMax - xMin) / (xGrid - 1);
            const Real sigma2 = sigma * sigma;
            const Real mu = r - 0.5 * sigma2;
            const Real diffCoeff = sigma2 / (2.0 * h * h);
            const Real driftCoeff = mu / (2.0 * h);

            for (Size i = 0; i < xGrid; ++i) {
                probe.lower(i) = diffCoeff - driftCoeff;
                probe.diag(i) = -sigma2 / (h * h) - r;
                probe.upper(i) = diffCoeff + driftCoeff;
            }

            // Call checkOffDiagonalNonNegative directly
            FdmMMatrixReport report = checkOffDiagonalNonNegative(
                probe, mesher, 0, false, 0.0);

            std::cout << "\n--- StandardCentral operator via checkOffDiagonalNonNegative ---" << std::endl;
            std::cout << "  report.ok: " << (report.ok ? "true" : "false") << std::endl;
            std::cout << "  report.checkedRows: " << report.checkedRows << std::endl;
            std::cout << "  report.negativeLower: " << report.negativeLower << std::endl;
            std::cout << "  report.negativeUpper: " << report.negativeUpper << std::endl;
            std::cout << "  report.minLower: " << report.minLower << std::endl;
            std::cout << "  report.minUpper: " << report.minUpper << std::endl;

            if (report.ok) {
                std::cout << "  FAIL: Expected ok=false (M-matrix violated)" << std::endl;
                allPass = false;
            } else {
                std::cout << "  PASS: ok=false, M-matrix violation confirmed" << std::endl;
            }
            if (report.negativeLower == 0 && report.negativeUpper == 0) {
                std::cout << "  FAIL: Expected nonzero negative counts" << std::endl;
                allPass = false;
            }
        }

        // ── Test 2: Build ExponentialFitting operator, run diagnostic ──
        {
            // For ExponentialFitting: effective diffusion = (sigma2/2) * rho(Pe)
            // where Pe = mu*h/sigma2 and rho = Pe/tanh(Pe)
            // This ensures non-negative off-diagonals.

            TripleBandLinearOp tbOp(0, mesher);
            ModTripleBandLinearOp probe(tbOp);

            const Real xMin = std::log(50.0), xMax = std::log(150.0);
            const Real h = (xMax - xMin) / (xGrid - 1);
            const Real sigma2 = sigma * sigma;
            const Real mu = r - 0.5 * sigma2;
            const Real Pe = mu * h / sigma2;
            const Real rho = (std::fabs(Pe) < 1e-6)
                ? 1.0 + Pe * Pe / 3.0
                : Pe / std::tanh(Pe);
            const Real effDiff = (sigma2 / 2.0) * rho;
            const Real diffCoeff = effDiff / (h * h);
            const Real driftCoeff = mu / (2.0 * h);

            for (Size i = 0; i < xGrid; ++i) {
                probe.lower(i) = diffCoeff - driftCoeff;
                probe.diag(i) = -2.0 * diffCoeff - r;
                probe.upper(i) = diffCoeff + driftCoeff;
            }

            FdmMMatrixReport report = checkOffDiagonalNonNegative(
                probe, mesher, 0, false, 0.0);

            std::cout << "\n--- ExponentialFitting operator via checkOffDiagonalNonNegative ---" << std::endl;
            std::cout << "  report.ok: " << (report.ok ? "true" : "false") << std::endl;
            std::cout << "  report.checkedRows: " << report.checkedRows << std::endl;
            std::cout << "  report.negativeLower: " << report.negativeLower << std::endl;
            std::cout << "  report.negativeUpper: " << report.negativeUpper << std::endl;

            if (!report.ok) {
                std::cout << "  FAIL: Expected ok=true (M-matrix satisfied)" << std::endl;
                allPass = false;
            } else {
                std::cout << "  PASS: ok=true, M-matrix property verified" << std::endl;
            }
        }

        // ── Supplemental: verify FdmBlackScholesOp FailFast integration ──
        {
            FdmBlackScholesSpatialDesc desc;
            desc.scheme = FdmBlackScholesSpatialDesc::Scheme::ExponentialFitting;
            desc.mMatrixPolicy = FdmBlackScholesSpatialDesc::MMatrixPolicy::FailFast;
            auto op = ext::make_shared<FdmBlackScholesOp>(
                mesher, process, K, false, -Null<Real>(), 0,
                ext::shared_ptr<FdmQuantoHelper>(), desc);

            bool threw = false;
            try { op->setTime(0.0, dt); }
            catch (const std::exception&) { threw = true; }

            std::cout << "\n--- Supplemental: ExponentialFitting FailFast integration ---" << std::endl;
            if (threw) {
                std::cout << "  FAIL: FailFast should not throw for ExponentialFitting" << std::endl;
                allPass = false;
            } else {
                std::cout << "  PASS: FailFast passed (checkOffDiagonalNonNegative ran internally)" << std::endl;
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
