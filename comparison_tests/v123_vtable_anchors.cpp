// Force vtable emission for v1.23 header-only classes by instantiating them.
#include <ql/instruments/payoffs.hpp>
#include <ql/processes/eulerdiscretization.hpp>

// Explicitly instantiate templates / force vtable emission
namespace {
    void forceVtableEmission() {
        QuantLib::PlainVanillaPayoff p1(QuantLib::Option::Call, 100.0);
        QuantLib::EulerDiscretization ed;
        (void)p1; (void)ed;
    }
}
