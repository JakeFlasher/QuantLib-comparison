// Verify that constructing FdmDiscreteBarrierStepCondition is impossible on v1.23.
// Expected: compilation failure (class does not exist, not just header).
#include <ql/methods/finitedifferences/meshers/uniform1dmesher.hpp>
#include <ql/methods/finitedifferences/meshers/fdmmeshercomposite.hpp>
#include <ql/methods/finitedifferences/stepconditions/fdmdiscretebarrierstepcondition.hpp>
#include <vector>

using namespace QuantLib;

int main() {
    auto mesher = ext::make_shared<FdmMesherComposite>(
        ext::make_shared<Uniform1dMesher>(0.0, 1.0, 100));
    std::vector<Time> times = {0.1, 0.2, 0.3};

    // Attempt to construct the discrete barrier step condition
    auto cond = ext::make_shared<FdmDiscreteBarrierStepCondition>(
        mesher, times, 95.0, 110.0);

    return 0;
}
