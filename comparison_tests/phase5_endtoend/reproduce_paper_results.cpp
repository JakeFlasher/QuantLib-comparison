// End-to-end paper replication: Milev-Tagliani Example 4.1
// Uses validated FdmBackwardSolver + FdmDiscreteBarrierStepCondition pattern.

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
#include <iomanip>

using namespace QuantLib;

struct Result { std::string name; Real price; Size negNodes; };

Result runBarrier(
    Real sigma, Real r, Real K, Real L, Real U, Real T,
    Size xGrid, Size tGrid, Size nMon,
    FdmBlackScholesSpatialDesc desc, const std::string& name,
    bool uniformMesh)
{
    Date today(28, March, 2004);
    auto spot  = ext::make_shared<SimpleQuote>(K);
    auto qTS   = ext::make_shared<FlatForward>(today, 0.0, Actual365Fixed());
    auto rTS   = ext::make_shared<FlatForward>(today, r, Actual365Fixed());
    auto volTS = ext::make_shared<BlackConstantVol>(
        today, NullCalendar(), sigma, Actual365Fixed());
    auto process = ext::make_shared<BlackScholesMertonProcess>(
        Handle<Quote>(spot), Handle<YieldTermStructure>(qTS),
        Handle<YieldTermStructure>(rTS), Handle<BlackVolTermStructure>(volTS));

    ext::shared_ptr<FdmMesherComposite> mesher;
    if (uniformMesh) {
        mesher = ext::make_shared<FdmMesherComposite>(
            ext::make_shared<Uniform1dMesher>(
                std::log(L - 15.0), std::log(U + 20.0), xGrid));
    } else {
        std::vector<std::tuple<Real, Real, bool>> cPts = {
            {K, 0.1, true}, {L, 0.1, true}, {U, 0.1, true}};
        mesher = ext::make_shared<FdmMesherComposite>(
            ext::make_shared<FdmBlackScholesMesher>(
                xGrid, process, T, K,
                Null<Real>(), Null<Real>(), 0.0001, 1.5, cPts));
    }

    auto payoff = ext::make_shared<PlainVanillaPayoff>(Option::Call, K);
    std::vector<Time> mon;
    for (Size i = 1; i <= nMon; ++i) mon.push_back(T * Real(i) / nMon);

    auto bc = ext::make_shared<FdmDiscreteBarrierStepCondition>(mesher, mon, L, U);
    auto conds = ext::make_shared<FdmStepConditionComposite>(
        std::list<std::vector<Time>>(1, mon),
        FdmStepConditionComposite::Conditions(1, bc));

    auto op = ext::make_shared<FdmBlackScholesOp>(
        mesher, process, K, false, -Null<Real>(), Size(0),
        ext::shared_ptr<FdmQuantoHelper>(), desc);

    Array rhs = buildPayoffOnMesh(mesher, payoff);
    FdmBackwardSolver solver(op, FdmBoundaryConditionSet(), conds,
                             FdmSchemeDesc::CrankNicolson());
    solver.rollback(rhs, T, 0.0, tGrid, 0);

    Size neg = 0;
    for (Size i = 1; i < rhs.size() - 1; ++i)
        if (rhs[i] < -1e-10) ++neg;

    return {name, interpolateOnMesh(rhs, mesher, K), neg};
}

int main() {
    try {
        Settings::instance().evaluationDate() = Date(28, March, 2004);
        const Real r = 0.05, K = 100.0, L = 95.0, U = 110.0;

        std::cout << "=== Paper Replication: Milev-Tagliani Example 4.1 ===" << std::endl;
        std::cout << std::fixed << std::setprecision(6);
        bool allPass = true;

        // Grid convergence at moderate vol
        std::cout << "\n--- Grid Convergence (sigma=0.25, T=0.5) ---" << std::endl;
        std::cout << std::setw(8) << "xGrid"
                  << std::setw(14) << "Standard"
                  << std::setw(14) << "ExpFitting"
                  << std::setw(14) << "MilevTagliani" << std::endl;

        for (Size xG : {200u, 500u, 1000u}) {
            Size tG = 4 * xG;
            auto pS = runBarrier(0.25, r, K, L, U, 0.5, xG, tG, 5,
                FdmBlackScholesSpatialDesc::standard(), "S", false);
            auto pE = runBarrier(0.25, r, K, L, U, 0.5, xG, tG, 5,
                FdmBlackScholesSpatialDesc::exponentialFitting(), "E", false);
            auto pM = runBarrier(0.25, r, K, L, U, 0.5, xG, tG, 5,
                FdmBlackScholesSpatialDesc::milevTaglianiCN(), "M", false);
            std::cout << std::setw(8) << xG
                      << std::setw(14) << pS.price
                      << std::setw(14) << pE.price
                      << std::setw(14) << pM.price << std::endl;
        }

        // Low-vol stress test
        std::cout << "\n--- Low Vol Stress (sigma=0.001, T=1.0) ---" << std::endl;
        for (const auto& [d, nm] : {
            std::make_pair(FdmBlackScholesSpatialDesc::standard(), std::string("StandardCentral")),
            std::make_pair(FdmBlackScholesSpatialDesc::exponentialFitting(), std::string("ExponentialFitting")),
            std::make_pair(FdmBlackScholesSpatialDesc::milevTaglianiCN(), std::string("MilevTaglianiCN"))
        }) {
            auto res = runBarrier(0.001, r, K, L, U, 1.0, 800, 200, 5, d, nm, true);
            std::cout << "  " << res.name << ": price=" << res.price
                      << " negNodes=" << res.negNodes << std::endl;

            if (nm != "StandardCentral" && res.negNodes > 0) {
                std::cout << "  FAIL: " << nm << " has negative nodes" << std::endl;
                allPass = false;
            }
        }

        std::cout << "\n" << (allPass ? "RESULT: PASS" : "RESULT: FAIL")
                  << " - Paper replication" << std::endl;
        return allPass ? 0 : 1;
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }
}
