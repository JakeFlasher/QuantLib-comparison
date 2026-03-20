// Verify FdmMMatrixReport / checkOffDiagonalNonNegative does not exist in v1.23.
// Expected: compilation failure (header not found).
#include <ql/methods/finitedifferences/operators/fdmmatrixdiagnostic.hpp>

int main() {
    return 0;
}
