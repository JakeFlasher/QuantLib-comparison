// Discrete double barrier replication — QuantLib_huatai only.
// Follows fdmdiscretebarrierengine.cpp and generate_data.cpp patterns.
// K=100, L=95, U=110, 5 monitoring dates.

#include <ql/qldefines.hpp>
#include <ql/settings.hpp>
#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/termstructures/volatility/equityfx/blackconstantvol.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <ql/instruments/payoffs.hpp>
#include <ql/methods/finitedifferences/meshers/fdmblackscholesmesher.hpp>
#include <ql/methods/finitedifferences/meshers/fdmmeshercomposite.hpp>
#include <ql/methods/finitedifferences/meshers/uniform1dmesher.hpp>
#include <ql/methods/finitedifferences/operators/fdmblackscholesop.hpp>
#include <ql/methods/finitedifferences/operators/fdmblackscholesspatialdesc.hpp>
#include <ql/methods/finitedifferences/operators/fdmlinearoplayout.hpp>
#include <ql/methods/finitedifferences/solvers/fdmbackwardsolver.hpp>
#include <ql/methods/finitedifferences/stepconditions/fdmdiscretebarrierstepcondition.hpp>
#include <ql/methods/finitedifferences/stepconditions/fdmstepconditioncomposite.hpp>
#include <ql/methods/finitedifferences/utilities/fdminnervaluecalculator.hpp>

#include "../common/fdm_test_helpers.hpp"

#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <iomanip>

using namespace QuantLib;

struct BarrierResult {
    std::string schemeName;
    Real price;
    Size negativeNodes;
    Real minV;
};

BarrierResult runBarrierTest(
    Real sigma, Real r, Real K, Real L, Real U, Real T,
    Size xGrid, Size tGrid, Size nMon,
    FdmBlackScholesSpatialDesc desc,
    const std::string& schemeName,
    bool useUniformMesh)
{
    Date today(28, March, 2004);

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

    ext::shared_ptr<FdmMesherComposite> mesher;
    if (useUniformMesh) {
        const Real xMin = std::log(L - 15.0);
        const Real xMax = std::log(U + 20.0);
        mesher = ext::make_shared<FdmMesherComposite>(
            ext::make_shared<Uniform1dMesher>(xMin, xMax, xGrid));
    } else {
        std::vector<std::tuple<Real, Real, bool>> cPoints = {
            {K, 0.1, true}, {L, 0.1, true}, {U, 0.1, true}
        };
        mesher = ext::make_shared<FdmMesherComposite>(
            ext::make_shared<FdmBlackScholesMesher>(
                xGrid, process, T, K,
                Null<Real>(), Null<Real>(), 0.0001, 1.5, cPoints));
    }

    auto payoff = ext::make_shared<PlainVanillaPayoff>(Option::Call, K);

    std::vector<Time> monTimes;
    for (Size i = 1; i <= nMon; ++i)
        monTimes.push_back(T * Real(i) / Real(nMon));

    auto barrierCondition = ext::make_shared<FdmDiscreteBarrierStepCondition>(
        mesher, monTimes, L, U);

    auto conditions = ext::make_shared<FdmStepConditionComposite>(
        std::list<std::vector<Time>>(1, monTimes),
        FdmStepConditionComposite::Conditions(1, barrierCondition));

    auto op = ext::make_shared<FdmBlackScholesOp>(
        mesher, process, K, false, -Null<Real>(), Size(0),
        ext::shared_ptr<FdmQuantoHelper>(), desc);

    Array rhs = buildPayoffOnMesh(mesher, payoff);
    const FdmBoundaryConditionSet bcSet;

    FdmBackwardSolver solver(op, bcSet, conditions,
                             FdmSchemeDesc::CrankNicolson());
    solver.rollback(rhs, T, 0.0, tGrid, 0);

    Real price = interpolateOnMesh(rhs, mesher, K);

    auto stats = countNegativeInterior(rhs);
    return {schemeName, price, stats.negativeCount, stats.minV};
}

int main() {
    try {
        Settings::instance().evaluationDate() = Date(28, March, 2004);

        const Real r = 0.05, K = 100.0, L = 95.0, U = 110.0;
        const Size nMon = 5;

        std::cout << "=== Discrete Double Barrier Replication ===" << std::endl;
        std::cout << std::fixed << std::setprecision(6);

        bool allPass = true;

        // Test 1: Moderate vol (sigma=0.25, T=0.5), FdmBlackScholesMesher
        {
            std::cout << "\n--- Moderate vol: sigma=0.25 T=0.5 ---" << std::endl;
            for (const auto& [d, name] : {
                std::make_pair(FdmBlackScholesSpatialDesc::standard(), std::string("StandardCentral")),
                std::make_pair(FdmBlackScholesSpatialDesc::exponentialFitting(), std::string("ExponentialFitting")),
                std::make_pair(FdmBlackScholesSpatialDesc::milevTaglianiCN(), std::string("MilevTaglianiCN"))
            }) {
                auto res = runBarrierTest(0.25, r, K, L, U, 0.5,
                    2000, 500, nMon, d, name, false);
                std::cout << "  " << res.schemeName << ": price=" << res.price
                          << " negNodes=" << res.negativeNodes
                          << " minV=" << res.minV << std::endl;
                if (!std::isfinite(res.price) || res.price < 0) {
                    std::cout << "  FAIL: non-finite or negative price" << std::endl;
                    allPass = false;
                }
            }
        }

        // Test 2: Low vol (sigma=0.001, T=1.0), Uniform mesh
        {
            std::cout << "\n--- Low vol: sigma=0.001 T=1.0 ---" << std::endl;
            for (const auto& [d, name] : {
                std::make_pair(FdmBlackScholesSpatialDesc::standard(), std::string("StandardCentral")),
                std::make_pair(FdmBlackScholesSpatialDesc::exponentialFitting(), std::string("ExponentialFitting")),
                std::make_pair(FdmBlackScholesSpatialDesc::milevTaglianiCN(), std::string("MilevTaglianiCN"))
            }) {
                auto res = runBarrierTest(0.001, r, K, L, U, 1.0,
                    800, 200, nMon, d, name, true);
                std::cout << "  " << res.schemeName << ": price=" << res.price
                          << " negNodes=" << res.negativeNodes
                          << " minV=" << res.minV << std::endl;

                if (name == "StandardCentral") {
                    if (res.negativeNodes == 0) {
                        std::cout << "  NOTE: StandardCentral no negative nodes at low vol" << std::endl;
                    }
                } else {
                    if (res.negativeNodes > 0) {
                        std::cout << "  FAIL: " << name << " has negative nodes" << std::endl;
                        allPass = false;
                    }
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
