// Verify FdmBlackScholesSpatialDesc does not exist in v1.23.
// Expected: compilation failure (type not found).
#include <ql/methods/finitedifferences/operators/fdmblackscholesspatialdesc.hpp>

int main() {
    QuantLib::FdmBlackScholesSpatialDesc desc;
    desc.scheme = QuantLib::FdmBlackScholesSpatialDesc::Scheme::ExponentialFitting;
    return 0;
}
