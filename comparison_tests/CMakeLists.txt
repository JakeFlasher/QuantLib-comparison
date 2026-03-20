cmake_minimum_required(VERSION 3.16)
project(QuantLibComparison CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─────────────────────────────────────────────────────────────────
# Locate both QuantLib versions
# ─────────────────────────────────────────────────────────────────

set(QL_V123_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../QuantLib_v1.23"
    CACHE PATH "Path to QuantLib v1.23 source tree")

set(QL_HUATAI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../QuantLib_huatai"
    CACHE PATH "Path to QuantLib_huatai (1.42-dev) source tree")
set(QL_HUATAI_BUILD "${QL_HUATAI_ROOT}/build"
    CACHE PATH "Path to QuantLib_huatai (1.42-dev) build directory")

# ─────────────────────────────────────────────────────────────────
# Import QuantLib_huatai (1.42-dev) library
# ─────────────────────────────────────────────────────────────────

add_library(ql_huatai SHARED IMPORTED)
set_target_properties(ql_huatai PROPERTIES
    IMPORTED_LOCATION "${QL_HUATAI_BUILD}/ql/libQuantLib.so"
    INTERFACE_INCLUDE_DIRECTORIES "${QL_HUATAI_ROOT}"
)

# ─────────────────────────────────────────────────────────────────
# Build minimal v1.23 static library from compilable sources
# (excludes cashflows/indexes/experimental that fail on timeseries.hpp)
# ─────────────────────────────────────────────────────────────────

include(cmake/v123_minimal_sources.cmake)

add_library(ql_v123_minimal OBJECT ${V123_MINIMAL_SOURCES})
target_include_directories(ql_v123_minimal PUBLIC "${QL_V123_ROOT}")
target_compile_options(ql_v123_minimal PRIVATE -fpermissive -w)
target_compile_definitions(ql_v123_minimal PRIVATE QL_VERSION_V123)

# ─────────────────────────────────────────────────────────────────
# Find Boost (required by QuantLib)
# ─────────────────────────────────────────────────────────────────

find_package(Boost REQUIRED)

# ─────────────────────────────────────────────────────────────────
# Helpers
# ─────────────────────────────────────────────────────────────────

function(add_huatai_test TARGET SOURCE)
    add_executable(${TARGET} ${SOURCE})
    target_link_libraries(${TARGET} PRIVATE ql_huatai Boost::boost)
    target_compile_definitions(${TARGET} PRIVATE QL_VERSION_HUATAI)
endfunction()

function(add_v123_test TARGET SOURCE)
    add_executable(${TARGET} ${SOURCE})
    target_link_libraries(${TARGET} PRIVATE ql_v123_minimal Boost::boost)
    target_compile_definitions(${TARGET} PRIVATE QL_VERSION_V123)
    target_compile_options(${TARGET} PRIVATE -fpermissive -w)
    target_link_options(${TARGET} PRIVATE -Wl,--gc-sections -Wl,--allow-multiple-definition)
endfunction()

# ─────────────────────────────────────────────────────────────────
# Dual-version runtime tests
# ─────────────────────────────────────────────────────────────────

# v1.23 runtime (from minimal source library)
add_v123_test(test_cn_oscillation_v123
    phase2_runtime_tests/test_cn_oscillation.cpp)

# QuantLib_huatai runtime
add_huatai_test(test_cn_oscillation_huatai
    phase2_runtime_tests/test_cn_oscillation.cpp)

add_huatai_test(test_positivity_huatai
    phase2_runtime_tests/test_positivity_huatai.cpp)

add_huatai_test(test_discrete_barrier_huatai
    phase2_runtime_tests/test_discrete_barrier_huatai.cpp)

add_huatai_test(test_mmatrix_diagnostic_huatai
    phase3_infrastructure/test_mmatrix_diagnostic.cpp)

add_huatai_test(test_reproduce_paper_huatai
    phase5_endtoend/reproduce_paper_results.cpp)

# ─────────────────────────────────────────────────────────────────
# CTest registration
# ─────────────────────────────────────────────────────────────────

enable_testing()

# Phase 1: compile-fail tests (v1.23 headers — expected to fail)
function(add_compile_fail_test NAME SOURCE)
    add_test(NAME ${NAME}
        COMMAND ${CMAKE_COMMAND}
            --log-level=ERROR
            -DSOURCE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}
            -DQL_INCLUDE=${QL_V123_ROOT}
            -DBOOST_INCLUDE=${Boost_INCLUDE_DIRS}
            -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/try_compile_fail.cmake
    )
endfunction()

add_compile_fail_test(v123_spatial_desc_missing
    phase1_compile_tests/test_spatial_desc_missing.cpp)

add_compile_fail_test(v123_discrete_barrier_missing
    phase1_compile_tests/test_discrete_barrier_missing.cpp)

add_compile_fail_test(v123_discrete_barrier_construct
    phase1_compile_tests/test_discrete_barrier_construct.cpp)

add_compile_fail_test(v123_mmatrix_diagnostic_missing
    phase1_compile_tests/test_mmatrix_diagnostic_missing.cpp)

add_compile_fail_test(v123_mesher_multipoint_missing
    phase1_compile_tests/test_mesher_multipoint.cpp)

add_compile_fail_test(v123_disposable_signature_mismatch
    phase1_compile_tests/test_disposable_signature.cpp)

# v1.23 full library build failure (separate toolchain evidence)
add_test(NAME v123_library_build_fails
    COMMAND ${CMAKE_COMMAND}
        --log-level=ERROR
        -DQL_V123_ROOT=${QL_V123_ROOT}
        -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/try_build_v123_lib.cmake
)

# Dual-version runtime tests
add_test(NAME oscillation_v123 COMMAND test_cn_oscillation_v123)
add_test(NAME oscillation_huatai COMMAND test_cn_oscillation_huatai)
add_test(NAME positivity_huatai COMMAND test_positivity_huatai)
add_test(NAME discrete_barrier_huatai COMMAND test_discrete_barrier_huatai)
add_test(NAME mmatrix_diagnostic_huatai COMMAND test_mmatrix_diagnostic_huatai)
add_test(NAME reproduce_paper_huatai COMMAND test_reproduce_paper_huatai)
