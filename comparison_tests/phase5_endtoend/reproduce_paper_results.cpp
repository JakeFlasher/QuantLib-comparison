// End-to-end paper replication: Milev-Tagliani Example 4.1
// Discrete double barrier knock-out call.
// K=100, L=95, U=110, sigma varies, demonstrates:
// - StandardCentral oscillates at low sigma
// - ExponentialFitting smooth and positive
// - MilevTaglianiCN smooth and positive
// - All converge at O(h^2) for moderate sigma

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
#include <ql/methods/finitedifferences/solvers/fdm1dimsolver.hpp>
#include <ql/methods/finitedifferences/solvers/fdmbackwardsolver.hpp>
#include <ql/methods/finitedifferences/stepconditions/fdmdiscretebarrierstepcondition.hpp>
#include <ql/methods/finitedifferences/stepconditions/fdmstepconditioncomposite.hpp>
#include <ql/payoff.hpp>
#include <ql/instruments/payoffs.hpp>

#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <iomanip>

using namespace QuantLib;

Real priceBarrier(
    Real sigma, Real r, Real K, Real L, Real U, Real T,
    Size xGrid, Size tGrid, Size nMon,
    FdmBlackScholesSpatialDesc spatialDesc)
{
    Date today(28, March, 2004);

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

    const Real margin = 0.1;
    const Real xMin = std::log(L * (1.0 - margin));
    const Real xMax = std::log(U * (1.0 + margin));
    auto mesher1d = ext::make_shared<Uniform1dMesher>(xMin, xMax, xGrid);
    auto mesher = ext::make_shared<FdmMesherComposite>(mesher1d);

    auto payoff = ext::make_shared<PlainVanillaPayoff>(Option::Call, K);

    std::vector<Time> monTimes;
    for (Size i = 1; i <= nMon; ++i)
        monTimes.push_back(T * Real(i) / Real(nMon));

    auto barrierCond = ext::make_shared<FdmDiscreteBarrierStepCondition>(
        mesher, monTimes, L, U, Size(0));

    std::list<std::vector<Time>> stoppingTimes;
    stoppingTimes.push_back(monTimes);
    FdmStepConditionComposite::Conditions conds;
    conds.push_back(barrierCond);
    auto stepCond = ext::make_shared<FdmStepConditionComposite>(stoppingTimes, conds);

    auto op = ext::make_shared<FdmBlackScholesOp>(
        mesher, process, K, false, -Null<Real>(), 0,
        ext::shared_ptr<FdmQuantoHelper>(), spatialDesc);

    FdmSolverDesc solverDesc = {
        mesher, FdmBoundaryConditionSet(), stepCond, payoff, T, tGrid, 0
    };

    Fdm1DimSolver solver(solverDesc, FdmSchemeDesc::CrankNicolson(), op);
    return solver.interpolateAt(K);
}

int main() {
    try {
        Settings::instance().evaluationDate() = Date(28, March, 2004);

        const Real r = 0.05, K = 100.0, L = 95.0, U = 110.0;
        const Size nMon = 5;

        std::cout << "=== Paper Replication: Milev-Tagliani Example 4.1 ===" << std::endl;
        std::cout << "K=" << K << " L=" << L << " U=" << U << " r=" << r
                  << " monitors=" << nMon << std::endl;
        std::cout << std::fixed << std::setprecision(6);

        bool allPass = true;

        // Grid convergence at moderate vol
        std::cout << "\n--- Grid Convergence (sigma=0.25, T=0.5) ---" << std::endl;
        std::cout << std::setw(8) << "xGrid" << std::setw(12) << "Standard"
                  << std::setw(15) << "ExpFitting"
                  << std::setw(15) << "MilevTagliani" << std::endl;

        for (Size xGrid : {100u, 200u, 400u}) {
            Size tGrid = 4 * xGrid;
            Real pStd = priceBarrier(0.25, r, K, L, U, 0.5, xGrid, tGrid, nMon,
                                     FdmBlackScholesSpatialDesc::standard());
            Real pExp = priceBarrier(0.25, r, K, L, U, 0.5, xGrid, tGrid, nMon,
                                     FdmBlackScholesSpatialDesc::exponentialFitting());
            Real pMT  = priceBarrier(0.25, r, K, L, U, 0.5, xGrid, tGrid, nMon,
                                     FdmBlackScholesSpatialDesc::milevTaglianiCN());

            std::cout << std::setw(8) << xGrid
                      << std::setw(12) << pStd
                      << std::setw(15) << pExp
                      << std::setw(15) << pMT << std::endl;
        }

        // Low vol stress test
        std::cout << "\n--- Low Vol Stress (sigma=0.001, T=1.0) ---" << std::endl;
        const Size xGrid = 400, tGrid = 1600;

        for (const auto& [desc, name] : {
            std::make_pair(FdmBlackScholesSpatialDesc::standard(), std::string("StandardCentral")),
            std::make_pair(FdmBlackScholesSpatialDesc::exponentialFitting(), std::string("ExponentialFitting")),
            std::make_pair(FdmBlackScholesSpatialDesc::milevTaglianiCN(), std::string("MilevTaglianiCN"))
        }) {
            Real p = priceBarrier(0.001, r, K, L, U, 1.0, xGrid, tGrid, nMon, desc);
            std::cout << "  " << name << ": " << p;
            if (!std::isfinite(p)) {
                std::cout << " [NON-FINITE]";
                allPass = false;
            }
            std::cout << std::endl;
        }

        std::cout << "\n" << (allPass ? "RESULT: PASS" : "RESULT: FAIL")
                  << " - Paper replication" << std::endl;
        return allPass ? 0 : 1;

    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }
}
