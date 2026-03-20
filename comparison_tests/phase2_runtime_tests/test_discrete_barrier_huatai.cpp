// Discrete double barrier replication test (Milev-Tagliani Example 4.1).
// QuantLib_huatai only: uses FdmDiscreteBarrierStepCondition.
// K=100, L=95, U=110, 5 monitoring dates, sigma=0.25/0.001

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

using namespace QuantLib;

struct BarrierResult {
    std::string schemeName;
    Real sigma;
    Real price;
    Size negativeNodes;
    Real minV;
};

BarrierResult runBarrierTest(
    Real sigma, Real r, Real K, Real L, Real U, Real T,
    Size xGrid, Size tGrid, Size nMonitoring,
    FdmBlackScholesSpatialDesc spatialDesc,
    const std::string& schemeName)
{
    Date today(28, March, 2004);

    auto spot  = ext::make_shared<SimpleQuote>(K);  // ATM
    auto qTS   = ext::make_shared<FlatForward>(today, 0.0, Actual365Fixed());
    auto rTS   = ext::make_shared<FlatForward>(today, r, Actual365Fixed());
    auto volTS = ext::make_shared<BlackConstantVol>(today, NullCalendar(),
                                                    sigma, Actual365Fixed());

    auto process = ext::make_shared<BlackScholesMertonProcess>(
        Handle<Quote>(spot),
        Handle<YieldTermStructure>(qTS),
        Handle<YieldTermStructure>(rTS),
        Handle<BlackVolTermStructure>(volTS));

    // Uniform mesher in log-space covering [L-margin, U+margin]
    const Real margin = 0.1;
    const Real xMin = std::log(L * (1.0 - margin));
    const Real xMax = std::log(U * (1.0 + margin));
    auto mesher1d = ext::make_shared<Uniform1dMesher>(xMin, xMax, xGrid);
    auto mesher = ext::make_shared<FdmMesherComposite>(mesher1d);

    auto payoff = ext::make_shared<PlainVanillaPayoff>(Option::Call, K);

    // Build monitoring times
    std::vector<Time> monitoringTimes;
    for (Size i = 1; i <= nMonitoring; ++i) {
        monitoringTimes.push_back(T * Real(i) / Real(nMonitoring));
    }

    // Discrete barrier step condition
    auto barrierCondition = ext::make_shared<FdmDiscreteBarrierStepCondition>(
        mesher, monitoringTimes, L, U, Size(0));

    // Assemble step conditions
    std::list<std::vector<Time>> stoppingTimes;
    stoppingTimes.push_back(monitoringTimes);

    FdmStepConditionComposite::Conditions conditions;
    conditions.push_back(barrierCondition);

    auto stepConditions = ext::make_shared<FdmStepConditionComposite>(
        stoppingTimes, conditions);

    // Build operator
    auto op = ext::make_shared<FdmBlackScholesOp>(
        mesher, process, K, false, -Null<Real>(), 0,
        ext::shared_ptr<FdmQuantoHelper>(),
        spatialDesc);

    FdmSolverDesc solverDesc = {
        mesher,
        FdmBoundaryConditionSet(),
        stepConditions,
        payoff,
        T, tGrid, 0
    };

    FdmSchemeDesc schemeDesc = FdmSchemeDesc::CrankNicolson();
    Fdm1DimSolver solver(solverDesc, schemeDesc, op);

    const Real price = solver.interpolateAt(K);

    // Check for negative values
    Size negCount = 0;
    Real minV = 1e30;
    const Size n = mesher->layout()->size();
    for (Size i = 1; i < n - 1; ++i) {
        const Real x = mesher->location(
            FdmLinearOpIterator(std::vector<Size>{n}, std::vector<Size>{i}, i), 0);
        const Real S = std::exp(x);
        const Real V = solver.interpolateAt(S);
        minV = std::min(minV, V);
        if (V < -1e-10) ++negCount;
    }

    return {schemeName, sigma, price, negCount, minV};
}

int main() {
    try {
        Settings::instance().evaluationDate() = Date(28, March, 2004);

        const Real r = 0.05;
        const Real K = 100.0;
        const Real L = 95.0;
        const Real U = 110.0;
        const Size nMon = 5;
        const Size xGrid = 400;
        const Size tGrid = 1600;

        std::cout << "=== Discrete Double Barrier Replication Test ===" << std::endl;
        std::cout << "K=" << K << " L=" << L << " U=" << U
                  << " r=" << r << " monitors=" << nMon << std::endl;

        bool allPass = true;

        // Test 1: Moderate volatility (sigma=0.25, T=0.5)
        {
            const Real sigma = 0.25;
            const Real T = 0.5;
            std::cout << "\n--- Moderate vol: sigma=" << sigma << " T=" << T << " ---" << std::endl;

            for (const auto& [desc, name] : {
                std::make_pair(FdmBlackScholesSpatialDesc::standard(), std::string("StandardCentral")),
                std::make_pair(FdmBlackScholesSpatialDesc::exponentialFitting(), std::string("ExponentialFitting")),
                std::make_pair(FdmBlackScholesSpatialDesc::milevTaglianiCN(), std::string("MilevTaglianiCN"))
            }) {
                auto res = runBarrierTest(sigma, r, K, L, U, T, xGrid, tGrid, nMon, desc, name);
                std::cout << "  " << res.schemeName << ": price=" << res.price
                          << " negNodes=" << res.negativeNodes
                          << " minV=" << res.minV << std::endl;
                if (!std::isfinite(res.price) || res.price < 0) {
                    std::cout << "  FAIL: non-finite or negative price" << std::endl;
                    allPass = false;
                }
            }
        }

        // Test 2: Low volatility (sigma=0.001, T=1.0)
        {
            const Real sigma = 0.001;
            const Real T = 1.0;
            std::cout << "\n--- Low vol: sigma=" << sigma << " T=" << T << " ---" << std::endl;

            for (const auto& [desc, name] : {
                std::make_pair(FdmBlackScholesSpatialDesc::standard(), std::string("StandardCentral")),
                std::make_pair(FdmBlackScholesSpatialDesc::exponentialFitting(), std::string("ExponentialFitting")),
                std::make_pair(FdmBlackScholesSpatialDesc::milevTaglianiCN(), std::string("MilevTaglianiCN"))
            }) {
                auto res = runBarrierTest(sigma, r, K, L, U, T, xGrid, tGrid, nMon, desc, name);
                std::cout << "  " << res.schemeName << ": price=" << res.price
                          << " negNodes=" << res.negativeNodes
                          << " minV=" << res.minV << std::endl;

                // ExponentialFitting and MilevTaglianiCN should be all-positive
                if (name != "StandardCentral" && res.negativeNodes > 0) {
                    std::cout << "  FAIL: " << name << " has negative nodes at low vol" << std::endl;
                    allPass = false;
                }
            }
        }

        std::cout << "\n" << (allPass ? "RESULT: PASS" : "RESULT: FAIL")
                  << " - Discrete barrier replication" << std::endl;
        return allPass ? 0 : 1;

    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }
}
