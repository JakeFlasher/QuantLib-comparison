// Verify FdmDiscreteBarrierStepCondition does not exist in v1.23.
// Expected: compilation failure (header not found).
#include <ql/methods/finitedifferences/stepconditions/fdmdiscretebarrierstepcondition.hpp>

int main() {
    return 0;
}
