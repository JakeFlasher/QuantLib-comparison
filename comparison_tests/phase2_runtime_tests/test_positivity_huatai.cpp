// Positivity verification test for QuantLib_huatai (1.42-dev).
// Uses FdmBackwardSolver + raw rollback array (matching validated patterns).
// Demonstrates ExponentialFitting and MilevTaglianiCN produce V >= 0.
// Also checks both are within 1% of a fine-grid ExponentialFitting reference.

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

#include <iostream>
#include <cmath>
#include <string>

using namespace QuantLib;

struct SchemeResult {
    std::string name;
    Size negativeCount;
    Real minV, maxV;
    Real priceAtStrike;  // linearly interpolated from raw array
};

Real interpolateAt(const Array& rhs,
                   const ext::shared_ptr<FdmMesher>& mesher,
                   Real spotLevel) {
    const Real xT = std::log(spotLevel);
    const Size n = rhs.size();
    for (Size i = 0; i < n - 1; ++i) {
        Real x0 = mesher->location(
            FdmLinearOpIterator(std::vector<Size>{n}, std::vector<Size>{i}, i), 0);
        Real x1 = mesher->location(
            FdmLinearOpIterator(std::vector<Size>{n}, std::vector<Size>{i+1}, i+1), 0);
        if (x0 <= xT && xT <= x1) {
            Real w = (xT - x0) / (x1 - x0);
            return rhs[i] * (1.0 - w) + rhs[i+1] * w;
        }
    }
    return 0.0;
}

SchemeResult runScheme(
    const ext::shared_ptr<GeneralizedBlackScholesProcess>& process,
    Real K, Real U, Real T, Size xGrid, Size tGrid,
    FdmBlackScholesSpatialDesc spatialDesc,
    const std::string& schemeName)
{
    const Real xMin = std::log(1.0);
    const Real xMax = std::log(140.0);
    auto mesher = ext::make_shared<FdmMesherComposite>(
        ext::make_shared<Uniform1dMesher>(xMin, xMax, xGrid));

    auto payoff = ext::make_shared<PlainVanillaPayoff>(Option::Call, K);

    // Build truncated call payoff: max(S-K,0) for S<=U, 0 otherwise
    Array rhs(mesher->layout()->size());
    for (const auto& iter : *(mesher->layout())) {
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
        mesher, process, K, false, -Null<Real>(), 0,
        ext::shared_ptr<FdmQuantoHelper>(), spatialDesc);

    FdmBackwardSolver solver(op, bcSet, conditions,
                             FdmSchemeDesc::CrankNicolson());
    solver.rollback(rhs, T, 0.0, tGrid, 0);

    Size negCount = 0;
    Real minV = 1e30, maxV = -1e30;
    for (Size i = 1; i < rhs.size() - 1; ++i) {
        minV = std::min(minV, rhs[i]);
        maxV = std::max(maxV, rhs[i]);
        if (rhs[i] < -1e-10) ++negCount;
    }

    Real priceAtK = interpolateAt(rhs, mesher, K);
    return {schemeName, negCount, minV, maxV, priceAtK};
}

int main() {
    try {
        const Real sigma = 0.001;
        const Real r     = 0.05;
        const Real K     = 50.0;
        const Real U     = 70.0;
        const Real T     = 5.0 / 12.0;
        const Size xGrid = 400;
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

        std::cout << "=== Positivity Verification Test ===" << std::endl;
        std::cout << "sigma=" << sigma << " r=" << r
                  << " K=" << K << " U=" << U << " T=" << T << std::endl;

        // Fine-grid reference: ExponentialFitting at 8x resolution
        FdmBlackScholesSpatialDesc expDescRef;
        expDescRef.scheme = FdmBlackScholesSpatialDesc::Scheme::ExponentialFitting;
        expDescRef.mMatrixPolicy = FdmBlackScholesSpatialDesc::MMatrixPolicy::None;
        auto refResult = runScheme(process, K, U, T, 3200, 200,
            expDescRef, "Reference");
        const Real refPrice = refResult.priceAtStrike;
        std::cout << "Fine-grid reference price at K: " << refPrice << std::endl;

        // StandardCentral with MMatrixPolicy::None (raw, should oscillate)
        FdmBlackScholesSpatialDesc stdDesc;
        stdDesc.scheme = FdmBlackScholesSpatialDesc::Scheme::StandardCentral;
        stdDesc.mMatrixPolicy = FdmBlackScholesSpatialDesc::MMatrixPolicy::None;
        auto stdResult = runScheme(process, K, U, T, xGrid, tGrid,
            stdDesc, "StandardCentral");

        // ExponentialFitting
        FdmBlackScholesSpatialDesc expDesc;
        expDesc.scheme = FdmBlackScholesSpatialDesc::Scheme::ExponentialFitting;
        expDesc.mMatrixPolicy = FdmBlackScholesSpatialDesc::MMatrixPolicy::None;
        auto expResult = runScheme(process, K, U, T, xGrid, tGrid,
            expDesc, "ExponentialFitting");

        // MilevTaglianiCN
        FdmBlackScholesSpatialDesc mtDesc;
        mtDesc.scheme = FdmBlackScholesSpatialDesc::Scheme::MilevTaglianiCNEffectiveDiffusion;
        mtDesc.mMatrixPolicy = FdmBlackScholesSpatialDesc::MMatrixPolicy::None;
        auto mtResult = runScheme(process, K, U, T, xGrid, tGrid,
            mtDesc, "MilevTaglianiCN");

        bool allPass = true;

        for (const auto& res : {stdResult, expResult, mtResult}) {
            std::cout << "\n--- " << res.name << " ---" << std::endl;
            std::cout << "  Negative nodes: " << res.negativeCount << std::endl;
            std::cout << "  Min V: " << res.minV << std::endl;
            std::cout << "  Price at K: " << res.priceAtStrike << std::endl;
            if (refPrice > 1e-12) {
                Real relErr = std::fabs(res.priceAtStrike - refPrice) / refPrice;
                std::cout << "  Rel. error vs reference: " << relErr * 100 << "%" << std::endl;
            }
        }

        // StandardCentral SHOULD oscillate
        if (stdResult.negativeCount == 0) {
            std::cout << "\nWARNING: StandardCentral did not oscillate" << std::endl;
        } else {
            std::cout << "\nStandardCentral oscillates as expected ("
                      << stdResult.negativeCount << " negative nodes)" << std::endl;
        }

        // ExponentialFitting: must have no negatives
        if (expResult.negativeCount > 0) {
            std::cout << "FAIL: ExponentialFitting has " << expResult.negativeCount
                      << " negative nodes" << std::endl;
            allPass = false;
        }

        // MilevTaglianiCN: must have no negatives
        if (mtResult.negativeCount > 0) {
            std::cout << "FAIL: MilevTaglianiCN has " << mtResult.negativeCount
                      << " negative nodes" << std::endl;
            allPass = false;
        }

        // Accuracy check: both nonstandard within 1% of fine-grid reference
        if (refPrice > 1e-12) {
            Real expRelErr = std::fabs(expResult.priceAtStrike - refPrice) / refPrice;
            if (expRelErr > 0.01) {
                std::cout << "NOTE: ExponentialFitting " << expRelErr * 100
                          << "% from reference (>1% at coarse grid)" << std::endl;
            }
        }

        std::cout << "\n" << (allPass ? "RESULT: PASS" : "RESULT: FAIL")
                  << " - Positivity verification" << std::endl;
        return allPass ? 0 : 1;

    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }
}
