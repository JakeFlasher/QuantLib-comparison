// Truncated call oscillation test.
// Compiles against both v1.23 and QuantLib_huatai.
// On v1.23: demonstrates spurious oscillations (V < -1e-10 at some nodes).
// On huatai: same StandardCentral scheme also oscillates (the problem exists
// regardless of version; the SOLUTION is what differs).
//
// Parameters: sigma=0.001, r=0.05, K=50, U=70, T=5/12, xGrid=200

#include <ql/qldefines.hpp>
#include <ql/settings.hpp>
#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/termstructures/volatility/equityfx/blackconstantvol.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <ql/methods/finitedifferences/meshers/fdmblackscholesmesher.hpp>
#include <ql/methods/finitedifferences/meshers/fdmmeshercomposite.hpp>
#include <ql/methods/finitedifferences/operators/fdmblackscholesop.hpp>
#include <ql/methods/finitedifferences/solvers/fdm1dimsolver.hpp>
#include <ql/methods/finitedifferences/solvers/fdmbackwardsolver.hpp>
#include <ql/payoff.hpp>
#include <ql/instruments/payoffs.hpp>

#include <iostream>
#include <cmath>
#include <algorithm>

using namespace QuantLib;

int main() {
    try {
        const Real sigma = 0.001;
        const Real r     = 0.05;
        const Real K     = 50.0;
        const Real U     = 70.0;   // truncation barrier
        const Real T     = 5.0/12.0;
        const Size xGrid = 200;
        const Size tGrid = 800;

        Date today(28, March, 2004);
        Settings::instance().evaluationDate() = today;

        auto spot  = ext::make_shared<SimpleQuote>(60.0);
        auto qTS   = ext::make_shared<FlatForward>(today, 0.0, Actual365Fixed());
        auto rTS   = ext::make_shared<FlatForward>(today, r, Actual365Fixed());
        auto volTS = ext::make_shared<BlackConstantVol>(today, NullCalendar(),
                                                        sigma, Actual365Fixed());

        auto process = ext::make_shared<BlackScholesMertonProcess>(
            Handle<Quote>(spot),
            Handle<YieldTermStructure>(qTS),
            Handle<YieldTermStructure>(rTS),
            Handle<BlackVolTermStructure>(volTS));

        // Build mesher with uniform grid in log-space
        const Real xMin = std::log(1.0);
        const Real xMax = std::log(U);
        auto mesher1d = ext::make_shared<Uniform1dMesher>(xMin, xMax, xGrid);
        auto mesher = ext::make_shared<FdmMesherComposite>(mesher1d);

        // Truncated call payoff: max(S-K, 0) for S in [0, U], 0 otherwise
        auto payoff = ext::make_shared<PlainVanillaPayoff>(Option::Call, K);

        // Build initial condition (truncated call at maturity)
        Array rhs(mesher->layout()->size());
        for (const auto& iter : *(mesher->layout())) {
            const Real x = mesher->location(iter, 0);
            const Real S = std::exp(x);
            rhs[iter.index()] = (S <= U) ? (*payoff)(S) : 0.0;
        }

        // Build operator
        auto op = ext::make_shared<FdmBlackScholesOp>(
            mesher, process, K, false, -Null<Real>(), 0);

        // Solver description
        FdmSolverDesc solverDesc = {
            mesher,
            FdmBoundaryConditionSet(),
            ext::make_shared<FdmStepConditionComposite>(
                std::list<std::vector<Time>>(),
                FdmStepConditionComposite::Conditions()),
            payoff,
            T, tGrid, 0
        };

        // Use Crank-Nicolson (theta=0.5)
        FdmSchemeDesc schemeDesc = FdmSchemeDesc::CrankNicolson();

        Fdm1DimSolver solver(solverDesc, schemeDesc, op);

        // Scan grid for negative values
        Size negativeCount = 0;
        Real minV = 1e30;
        Real maxV = -1e30;
        const Size n = mesher->layout()->size();

        for (Size i = 1; i < n - 1; ++i) {  // interior nodes only
            const Real x = mesher->location(
                FdmLinearOpIterator(std::vector<Size>{n}, std::vector<Size>{i}, i), 0);
            const Real S = std::exp(x);
            const Real V = solver.interpolateAt(S);

            minV = std::min(minV, V);
            maxV = std::max(maxV, V);
            if (V < -1e-10) {
                ++negativeCount;
            }
        }

        std::cout << "=== Truncated Call Oscillation Test (StandardCentral) ===" << std::endl;
        std::cout << "sigma=" << sigma << " r=" << r
                  << " K=" << K << " U=" << U << " T=" << T << std::endl;
        std::cout << "xGrid=" << xGrid << " tGrid=" << tGrid << std::endl;
        std::cout << "Interior nodes: " << (n - 2) << std::endl;
        std::cout << "Negative nodes (V < -1e-10): " << negativeCount << std::endl;
        std::cout << "Min V: " << minV << std::endl;
        std::cout << "Max V: " << maxV << std::endl;

        if (negativeCount > 0) {
            std::cout << "RESULT: PASS - Spurious oscillations detected ("
                      << negativeCount << " negative nodes)" << std::endl;
            return 0;
        } else {
            std::cout << "RESULT: FAIL - No negative nodes found (expected oscillations)"
                      << std::endl;
            return 1;
        }

    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }
}
