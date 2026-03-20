// Verify FdmBlackScholesMesher multi-point cPoints constructor does not exist in v1.23.
// Expected: compilation failure (no matching constructor).
#include <ql/methods/finitedifferences/meshers/fdmblackscholesmesher.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/termstructures/volatility/equityfx/blackconstantvol.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <vector>
#include <tuple>

using namespace QuantLib;

int main() {
    Date today(28, March, 2004);
    Settings::instance().evaluationDate() = today;

    auto spot = ext::make_shared<SimpleQuote>(100.0);
    auto qTS = ext::make_shared<FlatForward>(today, 0.0, Actual365Fixed());
    auto rTS = ext::make_shared<FlatForward>(today, 0.05, Actual365Fixed());
    auto volTS = ext::make_shared<BlackConstantVol>(today, NullCalendar(), 0.20, Actual365Fixed());

    auto process = ext::make_shared<BlackScholesMertonProcess>(
        Handle<Quote>(spot),
        Handle<YieldTermStructure>(qTS),
        Handle<YieldTermStructure>(rTS),
        Handle<BlackVolTermStructure>(volTS));

    // This constructor with std::vector<std::tuple<Real,Real,bool>> does NOT exist in v1.23
    std::vector<std::tuple<Real, Real, bool>> cPoints = {
        {100.0, 0.1, true},
        {95.0, 0.05, false},
        {110.0, 0.05, false}
    };

    FdmBlackScholesMesher mesher(
        200, process, 1.0, 100.0,
        Null<Real>(), Null<Real>(),
        0.0001, 1.5,
        cPoints);

    return 0;
}
