# v123_minimal_sources.cmake
# Collects the v1.23 .cpp files that compile on the current toolchain,
# excluding modules that fail due to timeseries.hpp / boost::mpl::if_.
# Used to build a minimal static library for v1.23 runtime tests.

file(GLOB_RECURSE _V123_ALL_SOURCES "${QL_V123_ROOT}/ql/*.cpp")

# Exclude modules that fail to compile due to timeseries.hpp dependency chain:
# cashflows (via couponpricer → iborindex → index → timeseries)
# indexes (via interestrateindex → index → timeseries)
# Some pricing engines that depend on cashflows
set(_V123_EXCLUDE_PATTERNS
    "/cashflows/"
    "/indexes/"
    "/experimental/"
    "/legacy/"
    "/instruments/"
    "/currencies/"
    "/index\\.cpp"
    "/indexmanager"
    "/prices\\.cpp"
    "/quotes/futuresconv"
    "/quotes/lastfixing"
    "/termstructures/volatility/swaption/"
    "/termstructures/volatility/optionlet/"
    "/termstructures/volatility/gaussian1d"
    "/termstructures/credit/"
    "/termstructures/inflation/"
    "/fdmaffinemodelswapinnervalue"
    "/fdmaffinemodeltermstructure"
    "/ratehelpers"
    "/oisratehelper"
    "/bondhelpers"
    "/nonlinearfittingmethods"
    "/fittedbonddiscountcurve"
    "/forwardswapquote"
    "/forwardvaluequote"
    "/overnightindexfuture"
    "/inflationtermstructure"
    "/models/"
    "/pricingengines/"
)

# Explicitly include payoffs.cpp (excluded by /instruments/ pattern but needed for vtables)
set(V123_MINIMAL_SOURCES "${QL_V123_ROOT}/ql/instruments/payoffs.cpp")

foreach(_src ${_V123_ALL_SOURCES})
    set(_exclude FALSE)
    foreach(_pat ${_V123_EXCLUDE_PATTERNS})
        if(_src MATCHES "${_pat}")
            set(_exclude TRUE)
            break()
        endif()
    endforeach()
    if(NOT _exclude)
        list(APPEND V123_MINIMAL_SOURCES "${_src}")
    endif()
endforeach()
