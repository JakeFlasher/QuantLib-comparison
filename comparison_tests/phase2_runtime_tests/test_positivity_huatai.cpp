// Positivity + accuracy verification for QuantLib_huatai (1.42-dev).
// Test 1 (positivity): sigma=0.001, xGrid=200 — nonstandard schemes V >= 0
// Test 2 (accuracy): sigma=0.20, xGrid=800 — both within 1% of fine-grid ref

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
#include <ql/methods/finitedifferences/solvers/fdmbackwardsolver.hpp>
#include <ql/methods/finitedifferences/stepconditions/fdmstepconditioncomposite.hpp>
#include <ql/methods/finitedifferences/utilities/fdmdirichletboundary.hpp>

#include "../common/fdm_test_helpers.hpp"

#include <iostream>
#include <cmath>
#include <string>

using namespace QuantLib;

struct RunResult {
    std::string name;
    Size negativeCount;
    Real minV;
    Real priceAtStrike;
};

RunResult runTruncatedCall(
    Real sigma, Real r, Real K, Real U, Real T,
    Size xGrid, Size tGrid,
    FdmBlackScholesSpatialDesc spatialDesc,
    const std::string& schemeName)
{
    Date today(28, March, 2004);
    auto spot  = ext::make_shared<SimpleQuote>(60.0);
    auto qTS   = ext::make_shared<FlatForward>(today, 0.0, Actual365Fixed());
    auto rTS   = ext::make_shared<FlatForward>(today, r, Actual365Fixed());
    auto volTS = ext::make_shared<BlackConstantVol>(
        today, NullCalendar(), sigma, Actual365Fixed());
    auto process = ext::make_shared<BlackScholesMertonProcess>(
        Handle<Quote>(spot), Handle<YieldTermStructure>(qTS),
        Handle<YieldTermStructure>(rTS), Handle<BlackVolTermStructure>(volTS));

    const Real xMin = std::log(1.0), xMax = std::log(140.0);
    auto mesher = ext::make_shared<FdmMesherComposite>(
        ext::make_shared<Uniform1dMesher>(xMin, xMax, xGrid));

    auto payoff = ext::make_shared<PlainVanillaPayoff>(Option::Call, K);
    Array rhs(mesher->layout()->size());
    for (const auto& iter : *(mesher->layout())) {
        const Real x = mesher->location(iter, 0);
        const Real S = std::exp(x);
        rhs[iter.index()] = (S <= U) ? (*payoff)(S) : 0.0;
    }

    const FdmBoundaryConditionSet bcSet = {
        ext::make_shared<FdmDirichletBoundary>(mesher, 0.0, 0, FdmDirichletBoundary::Lower),
        ext::make_shared<FdmDirichletBoundary>(mesher, 0.0, 0, FdmDirichletBoundary::Upper)
    };
    auto conds = ext::make_shared<FdmStepConditionComposite>(
        std::list<std::vector<Time>>(), FdmStepConditionComposite::Conditions());

    auto op = ext::make_shared<FdmBlackScholesOp>(
        mesher, process, K, false, -Null<Real>(), 0,
        ext::shared_ptr<FdmQuantoHelper>(), spatialDesc);

    FdmBackwardSolver solver(op, bcSet, conds, FdmSchemeDesc::CrankNicolson());
    solver.rollback(rhs, T, 0.0, tGrid, 0);

    auto stats = countNegativeInterior(rhs);
    return {schemeName, stats.negativeCount, stats.minV,
            interpolateOnMesh(rhs, mesher, K)};
}

int main() {
    try {
        Settings::instance().evaluationDate() = Date(28, March, 2004);
        bool allPass = true;

        // ── Test 1: Positivity at low vol (same params as AC-1) ──
        std::cout << "=== Test 1: Positivity at sigma=0.001, xGrid=200 ===" << std::endl;
        {
            auto stdD = FdmBlackScholesSpatialDesc::standard();
            stdD.mMatrixPolicy = FdmBlackScholesSpatialDesc::MMatrixPolicy::None;
            auto expD = FdmBlackScholesSpatialDesc::exponentialFitting();
            expD.mMatrixPolicy = FdmBlackScholesSpatialDesc::MMatrixPolicy::None;
            auto mtD = FdmBlackScholesSpatialDesc::milevTaglianiCN();
            mtD.mMatrixPolicy = FdmBlackScholesSpatialDesc::MMatrixPolicy::None;

            auto std_ = runTruncatedCall(0.001, 0.05, 50.0, 70.0, 5.0/12.0, 200, 25, stdD, "StandardCentral");
            auto exp_ = runTruncatedCall(0.001, 0.05, 50.0, 70.0, 5.0/12.0, 200, 25, expD, "ExponentialFitting");
            auto mt_  = runTruncatedCall(0.001, 0.05, 50.0, 70.0, 5.0/12.0, 200, 25, mtD, "MilevTaglianiCN");

            for (const auto& r : {std_, exp_, mt_})
                std::cout << "  " << r.name << ": negNodes=" << r.negativeCount
                          << " minV=" << r.minV << std::endl;

            if (exp_.negativeCount > 0) {
                std::cout << "  FAIL: ExponentialFitting has negative nodes" << std::endl;
                allPass = false;
            }
            if (mt_.negativeCount > 0) {
                std::cout << "  FAIL: MilevTaglianiCN has negative nodes" << std::endl;
                allPass = false;
            }
        }

        // ── Test 2: Accuracy at moderate vol (sigma=0.20) ──
        std::cout << "\n=== Test 2: Accuracy at sigma=0.20, xGrid=800 ===" << std::endl;
        {
            auto expD = FdmBlackScholesSpatialDesc::exponentialFitting();
            expD.mMatrixPolicy = FdmBlackScholesSpatialDesc::MMatrixPolicy::None;
            auto mtD = FdmBlackScholesSpatialDesc::milevTaglianiCN();
            mtD.mMatrixPolicy = FdmBlackScholesSpatialDesc::MMatrixPolicy::None;

            // Fine-grid reference
            auto ref = runTruncatedCall(0.20, 0.05, 50.0, 70.0, 5.0/12.0, 6400, 400, expD, "Ref");
            auto exp_ = runTruncatedCall(0.20, 0.05, 50.0, 70.0, 5.0/12.0, 800, 50, expD, "ExponentialFitting");
            auto mt_  = runTruncatedCall(0.20, 0.05, 50.0, 70.0, 5.0/12.0, 800, 50, mtD, "MilevTaglianiCN");

            std::cout << "  Reference price at K: " << ref.priceAtStrike << std::endl;
            for (const auto& r : {exp_, mt_}) {
                Real relErr = (ref.priceAtStrike > 1e-12)
                    ? std::fabs(r.priceAtStrike - ref.priceAtStrike) / ref.priceAtStrike
                    : 0.0;
                std::cout << "  " << r.name << ": price=" << r.priceAtStrike
                          << " relErr=" << relErr * 100 << "%" << std::endl;
                if (relErr > 0.01) {
                    std::cout << "  FAIL: " << r.name << " exceeds 1% tolerance" << std::endl;
                    allPass = false;
                }
            }
        }

        std::cout << "\n" << (allPass ? "RESULT: PASS" : "RESULT: FAIL")
                  << " - Positivity and accuracy" << std::endl;
        return allPass ? 0 : 1;
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }
}
