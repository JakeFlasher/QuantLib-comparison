// Truncated call oscillation test.
// Source-compatible with both v1.23 and QuantLib_huatai (1.42-dev).
// StandardCentral produces V < -1e-10 at low vol (sigma=0.001).
// Parameters: sigma=0.001, r=0.05, K=50, U=70, T=5/12, xGrid=200

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
#include <ql/methods/finitedifferences/operators/fdmlinearoplayout.hpp>
#include <ql/methods/finitedifferences/solvers/fdmbackwardsolver.hpp>
#include <ql/methods/finitedifferences/stepconditions/fdmstepconditioncomposite.hpp>
#include <ql/methods/finitedifferences/utilities/fdmdirichletboundary.hpp>

#include <iostream>
#include <cmath>
#include <limits>

using namespace QuantLib;

int main() {
    try {
        const Real sigma = 0.001;
        const Real r     = 0.05;
        const Real K     = 50.0;
        const Real T     = 5.0 / 12.0;
        const Size xGrid = 200;
        const Size tGrid = 25;

        Date today(28, March, 2004);
        Settings::instance().evaluationDate() = today;

        auto spot  = ext::make_shared<SimpleQuote>(60.0);
        auto qTS   = ext::make_shared<FlatForward>(today, 0.0, Actual365Fixed());
        auto rTS   = ext::make_shared<FlatForward>(today, r, Actual365Fixed());
        auto volTS = ext::make_shared<BlackConstantVol>(
            today, NullCalendar(), sigma, Actual365Fixed());

        auto process = ext::make_shared<BlackScholesMertonProcess>(
            Handle<Quote>(spot),
            Handle<YieldTermStructure>(qTS),
            Handle<YieldTermStructure>(rTS),
            Handle<BlackVolTermStructure>(volTS));

        const Real xMin = std::log(1.0);
        const Real xMax = std::log(140.0);
        auto mesher = ext::make_shared<FdmMesherComposite>(
            ext::make_shared<Uniform1dMesher>(xMin, xMax, xGrid));

        // Build truncated call payoff: max(S-K,0) for S<=U, 0 otherwise
        auto payoff = ext::make_shared<PlainVanillaPayoff>(Option::Call, K);
        const Real U = 70.0;
        Array rhs(mesher->layout()->size());

        // v1.23-compatible iteration (no range-based for on layout)
        const ext::shared_ptr<FdmLinearOpLayout> layout = mesher->layout();
        const FdmLinearOpIterator endIter = layout->end();
        for (FdmLinearOpIterator iter = layout->begin();
             iter != endIter; ++iter) {
            const Real x = mesher->location(iter, 0);
            const Real S = std::exp(x);
            rhs[iter.index()] = (S <= U) ? (*payoff)(S) : 0.0;
        }

        const FdmBoundaryConditionSet bcSet = {
            ext::make_shared<FdmDirichletBoundary>(
                mesher, 0.0, 0, FdmDirichletBoundary::Lower),
            ext::make_shared<FdmDirichletBoundary>(
                mesher, 0.0, 0, FdmDirichletBoundary::Upper)
        };

        auto conditions = ext::make_shared<FdmStepConditionComposite>(
            std::list<std::vector<Time>>(),
            FdmStepConditionComposite::Conditions());

        auto op = ext::make_shared<FdmBlackScholesOp>(
            mesher, process, K);

        FdmBackwardSolver solver(op, bcSet, conditions,
                                 FdmSchemeDesc::CrankNicolson());
        solver.rollback(rhs, T, 0.0, tGrid, 0);

        // Inspect raw rollback array for negative values
        Size negativeCount = 0;
        Real minV = std::numeric_limits<Real>::max();
        Real maxV = std::numeric_limits<Real>::lowest();
        const Real negThreshold = -1e-10;

        for (Size i = 1; i < rhs.size() - 1; ++i) {
            minV = std::min(minV, rhs[i]);
            maxV = std::max(maxV, rhs[i]);
            if (rhs[i] < negThreshold) ++negativeCount;
        }

        std::cout << "=== Truncated Call Oscillation Test (StandardCentral) ===" << std::endl;
        std::cout << "sigma=" << sigma << " r=" << r
                  << " K=" << K << " U=" << U << " T=" << T << std::endl;
        std::cout << "xGrid=" << xGrid << " tGrid=" << tGrid << std::endl;
        std::cout << "Interior nodes: " << (rhs.size() - 2) << std::endl;
        std::cout << "Negative nodes (V < -1e-10): " << negativeCount << std::endl;
        std::cout << "Min V: " << minV << std::endl;
        std::cout << "Max V: " << maxV << std::endl;

        if (negativeCount > 0) {
            std::cout << "RESULT: PASS - Spurious oscillations detected ("
                      << negativeCount << " negative nodes)" << std::endl;
            return 0;
        } else {
            std::cout << "RESULT: FAIL - No negative nodes found" << std::endl;
            return 1;
        }

    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }
}
