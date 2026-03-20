#ifndef FDM_TEST_HELPERS_HPP
#define FDM_TEST_HELPERS_HPP

// Shared utilities for comparison test suite.
// All functions are header-only for simplicity.

#include <ql/types.hpp>
#include <ql/math/array.hpp>
#include <ql/methods/finitedifferences/meshers/fdmmeshercomposite.hpp>
#include <ql/methods/finitedifferences/operators/fdmlinearoplayout.hpp>
#include <ql/payoff.hpp>

#include <cmath>
#include <limits>

namespace QuantLib {

    constexpr Real NegativeThreshold = -1e-10;

    struct ArrayStats {
        Size negativeCount = 0;
        Real minV = std::numeric_limits<Real>::max();
        Real maxV = std::numeric_limits<Real>::lowest();
    };

    inline ArrayStats countNegativeInterior(const Array& rhs,
                                            Real threshold = NegativeThreshold) {
        ArrayStats stats;
        for (Size i = 1; i < rhs.size() - 1; ++i) {
            stats.minV = std::min(stats.minV, rhs[i]);
            stats.maxV = std::max(stats.maxV, rhs[i]);
            if (rhs[i] < threshold)
                ++stats.negativeCount;
        }
        return stats;
    }

    inline Array buildPayoffOnMesh(
        const ext::shared_ptr<FdmMesher>& mesher,
        const ext::shared_ptr<Payoff>& payoff)
    {
        Array rhs(mesher->layout()->size());
        const auto layout = mesher->layout();
        const FdmLinearOpIterator endIter = layout->end();
        for (FdmLinearOpIterator iter = layout->begin();
             iter != endIter; ++iter) {
            rhs[iter.index()] = (*payoff)(std::exp(mesher->location(iter, 0)));
        }
        return rhs;
    }

    inline Real interpolateOnMesh(
        const Array& rhs,
        const ext::shared_ptr<FdmMesher>& mesher,
        Real spotLevel)
    {
        const Real xTarget = std::log(spotLevel);
        const Size n = rhs.size();
        for (Size i = 0; i < n - 1; ++i) {
            Real x0 = mesher->location(
                FdmLinearOpIterator(std::vector<Size>{n}, std::vector<Size>{i}, i), 0);
            Real x1 = mesher->location(
                FdmLinearOpIterator(std::vector<Size>{n}, std::vector<Size>{i+1}, i+1), 0);
            if (x0 <= xTarget && xTarget <= x1) {
                Real w = (xTarget - x0) / (x1 - x0);
                return rhs[i] * (1.0 - w) + rhs[i+1] * w;
            }
        }
        return 0.0;
    }

}

#endif
