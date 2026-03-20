// Positivity verification test for QuantLib_huatai (1.42-dev).
// Demonstrates ExponentialFitting and MilevTaglianiCN produce V >= 0
// at all interior nodes under the same low-vol parameters where
// StandardCentral oscillates.
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
#include <ql/methods/finitedifferences/meshers/uniform1dmesher.hpp>
#include <ql/methods/finitedifferences/meshers/fdmmeshercomposite.hpp>
#include <ql/methods/finitedifferences/operators/fdmblackscholesop.hpp>
#include <ql/methods/finitedifferences/operators/fdmblackscholesspatialdesc.hpp>
#include <ql/methods/finitedifferences/solvers/fdm1dimsolver.hpp>
#include <ql/methods/finitedifferences/solvers/fdmbackwardsolver.hpp>
#include <ql/payoff.hpp>
#include <ql/instruments/payoffs.hpp>

#include <iostream>
#include <cmath>
#include <string>

using namespace QuantLib;

struct SchemeResult {
    std::string name;
    Size negativeCount;
    Real minV;
    Real maxV;
};

SchemeResult runScheme(
    const ext::shared_ptr<FdmMesher>& mesher,
    const ext::shared_ptr<GeneralizedBlackScholesProcess>& process,
    Real K, Real U, Real T, Size tGrid,
    FdmBlackScholesSpatialDesc spatialDesc,
    const std::string& schemeName)
{
    auto payoff = ext::make_shared<PlainVanillaPayoff>(Option::Call, K);

    Array rhs(mesher->layout()->size());
    for (const auto& iter : *(mesher->layout())) {
        const Real x = mesher->location(iter, 0);
        const Real S = std::exp(x);
        rhs[iter.index()] = (S <= U) ? (*payoff)(S) : 0.0;
    }

    auto op = ext::make_shared<FdmBlackScholesOp>(
        mesher, process, K, false, -Null<Real>(), 0,
        ext::shared_ptr<FdmQuantoHelper>(),
        spatialDesc);

    FdmSolverDesc solverDesc = {
        mesher,
        FdmBoundaryConditionSet(),
        ext::make_shared<FdmStepConditionComposite>(
            std::list<std::vector<Time>>(),
            FdmStepConditionComposite::Conditions()),
        payoff,
        T, tGrid, 0
    };

    FdmSchemeDesc schemeDesc = FdmSchemeDesc::CrankNicolson();
    Fdm1DimSolver solver(solverDesc, schemeDesc, op);

    Size negCount = 0;
    Real minV = 1e30, maxV = -1e30;
    const Size n = mesher->layout()->size();

    for (Size i = 1; i < n - 1; ++i) {
        const Real x = mesher->location(
            FdmLinearOpIterator(std::vector<Size>{n}, std::vector<Size>{i}, i), 0);
        const Real S = std::exp(x);
        const Real V = solver.interpolateAt(S);
        minV = std::min(minV, V);
        maxV = std::max(maxV, V);
        if (V < -1e-10) ++negCount;
    }

    return {schemeName, negCount, minV, maxV};
}

int main() {
    try {
        const Real sigma = 0.001;
        const Real r     = 0.05;
        const Real K     = 50.0;
        const Real U     = 70.0;
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

        const Real xMin = std::log(1.0);
        const Real xMax = std::log(U);
        auto mesher1d = ext::make_shared<Uniform1dMesher>(xMin, xMax, xGrid);
        auto mesher = ext::make_shared<FdmMesherComposite>(mesher1d);

        std::cout << "=== Positivity Verification Test (QuantLib_huatai 1.42-dev) ===" << std::endl;
        std::cout << "sigma=" << sigma << " r=" << r
                  << " K=" << K << " U=" << U << " T=" << T << std::endl;

        // Run all three schemes
        auto stdResult = runScheme(mesher, process, K, U, T, tGrid,
            FdmBlackScholesSpatialDesc::standard(), "StandardCentral");

        auto expResult = runScheme(mesher, process, K, U, T, tGrid,
            FdmBlackScholesSpatialDesc::exponentialFitting(), "ExponentialFitting");

        auto mtResult = runScheme(mesher, process, K, U, T, tGrid,
            FdmBlackScholesSpatialDesc::milevTaglianiCN(), "MilevTaglianiCN");

        bool allPass = true;

        for (const auto& res : {stdResult, expResult, mtResult}) {
            std::cout << "\n--- " << res.name << " ---" << std::endl;
            std::cout << "  Negative nodes: " << res.negativeCount << std::endl;
            std::cout << "  Min V: " << res.minV << std::endl;
            std::cout << "  Max V: " << res.maxV << std::endl;
        }

        // StandardCentral SHOULD oscillate (negative nodes expected)
        if (stdResult.negativeCount == 0) {
            std::cout << "\nWARNING: StandardCentral did not oscillate (unexpected)" << std::endl;
        }

        // ExponentialFitting must NOT have negative nodes
        if (expResult.negativeCount > 0) {
            std::cout << "\nRESULT: FAIL - ExponentialFitting has "
                      << expResult.negativeCount << " negative nodes" << std::endl;
            allPass = false;
        }

        // MilevTaglianiCN must NOT have negative nodes
        if (mtResult.negativeCount > 0) {
            std::cout << "\nRESULT: FAIL - MilevTaglianiCN has "
                      << mtResult.negativeCount << " negative nodes" << std::endl;
            allPass = false;
        }

        if (allPass) {
            std::cout << "\nRESULT: PASS - Both nonstandard schemes preserve positivity"
                      << std::endl;
            return 0;
        }
        return 1;

    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }
}
