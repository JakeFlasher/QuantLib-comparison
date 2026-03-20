cmake_minimum_required(VERSION 3.16)
project(QuantLibComparison CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─────────────────────────────────────────────────────────────────
# Locate both QuantLib versions
# ─────────────────────────────────────────────────────────────────

set(QL_V123_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../QuantLib_v1.23"
    CACHE PATH "Path to QuantLib v1.23 source tree")
set(QL_V123_BUILD "${QL_V123_ROOT}/build"
    CACHE PATH "Path to QuantLib v1.23 build directory")

set(QL_HUATAI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../QuantLib_huatai"
    CACHE PATH "Path to QuantLib_huatai (1.42-dev) source tree")
set(QL_HUATAI_BUILD "${QL_HUATAI_ROOT}/build"
    CACHE PATH "Path to QuantLib_huatai (1.42-dev) build directory")

# ─────────────────────────────────────────────────────────────────
# Import v1.23 library
# ─────────────────────────────────────────────────────────────────

add_library(ql_v123 SHARED IMPORTED)
set_target_properties(ql_v123 PROPERTIES
    IMPORTED_LOCATION "${QL_V123_BUILD}/ql/libQuantLib.so"
    INTERFACE_INCLUDE_DIRECTORIES "${QL_V123_ROOT}"
)

# ─────────────────────────────────────────────────────────────────
# Import QuantLib_huatai (1.42-dev) library
# ─────────────────────────────────────────────────────────────────

add_library(ql_huatai SHARED IMPORTED)
set_target_properties(ql_huatai PROPERTIES
    IMPORTED_LOCATION "${QL_HUATAI_BUILD}/ql/libQuantLib.so"
    INTERFACE_INCLUDE_DIRECTORIES "${QL_HUATAI_ROOT}"
)

# ─────────────────────────────────────────────────────────────────
# Find Boost (required by both versions)
# ─────────────────────────────────────────────────────────────────

find_package(Boost REQUIRED)

# ─────────────────────────────────────────────────────────────────
# Helper: add a test executable linked against a specific QuantLib
# ─────────────────────────────────────────────────────────────────

function(add_ql_test TARGET SOURCE QL_TARGET)
    add_executable(${TARGET} ${SOURCE})
    target_link_libraries(${TARGET} PRIVATE ${QL_TARGET} Boost::boost)
    target_compile_definitions(${TARGET} PRIVATE
        $<$<STREQUAL:${QL_TARGET},ql_v123>:QL_VERSION_V123>
        $<$<STREQUAL:${QL_TARGET},ql_huatai>:QL_VERSION_HUATAI>
    )
endfunction()

# ─────────────────────────────────────────────────────────────────
# Phase 1: Compile-time tests (v1.23 — these are expected to fail)
# Built separately via try_compile or manual invocation
# ─────────────────────────────────────────────────────────────────

# Phase 1 tests are NOT added as normal targets because they are
# expected to fail compilation. Instead, we provide a custom target
# that runs cmake --build with individual test sources and reports
# success (expected failure) or failure (unexpected success).

# ─────────────────────────────────────────────────────────────────
# Phase 2: Runtime behavioral tests
# ─────────────────────────────────────────────────────────────────

add_ql_test(test_cn_oscillation_v123
    phase2_runtime_tests/test_cn_oscillation.cpp ql_v123)

add_ql_test(test_cn_oscillation_huatai
    phase2_runtime_tests/test_cn_oscillation.cpp ql_huatai)

add_ql_test(test_positivity_huatai
    phase2_runtime_tests/test_positivity_huatai.cpp ql_huatai)

add_ql_test(test_discrete_barrier_huatai
    phase2_runtime_tests/test_discrete_barrier_huatai.cpp ql_huatai)

# ─────────────────────────────────────────────────────────────────
# Phase 3: Infrastructure depth tests
# ─────────────────────────────────────────────────────────────────

add_ql_test(test_mmatrix_diagnostic_huatai
    phase3_infrastructure/test_mmatrix_diagnostic.cpp ql_huatai)

# ─────────────────────────────────────────────────────────────────
# Phase 5: End-to-end tests
# ─────────────────────────────────────────────────────────────────

add_ql_test(test_reproduce_paper_huatai
    phase5_endtoend/reproduce_paper_results.cpp ql_huatai)

# ─────────────────────────────────────────────────────────────────
# Phase 1: Absence / compile-fail tests (driven by CTest)
# Uses try_compile to verify expected compilation failures
# ─────────────────────────────────────────────────────────────────

enable_testing()

# Helper: test that a source file FAILS to compile against v1.23
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

add_compile_fail_test(v123_mmatrix_diagnostic_missing
    phase1_compile_tests/test_mmatrix_diagnostic_missing.cpp)

add_compile_fail_test(v123_mesher_multipoint_missing
    phase1_compile_tests/test_mesher_multipoint.cpp)

# Runtime tests
add_test(NAME oscillation_v123 COMMAND test_cn_oscillation_v123)
add_test(NAME oscillation_huatai COMMAND test_cn_oscillation_huatai)
add_test(NAME positivity_huatai COMMAND test_positivity_huatai)
add_test(NAME discrete_barrier_huatai COMMAND test_discrete_barrier_huatai)
add_test(NAME mmatrix_diagnostic_huatai COMMAND test_mmatrix_diagnostic_huatai)
add_test(NAME reproduce_paper_huatai COMMAND test_reproduce_paper_huatai)
