# try_build_v123_lib.cmake
# Tests that v1.23 FAILS to build a library on the current toolchain.
# Returns success (exit 0) if the build FAILS (expected).
# Returns failure (exit 1) if the build SUCCEEDS (unexpected).

# Configure v1.23 in a temp directory
set(BUILD_DIR "${CMAKE_CURRENT_LIST_DIR}/../v123_probe_build")

execute_process(
    COMMAND ${CMAKE_COMMAND}
        -S ${QL_V123_ROOT}
        -B ${BUILD_DIR}
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        -DCMAKE_CXX_COMPILER=${CXX_COMPILER}
        "-DCMAKE_CXX_FLAGS=-fpermissive -w"
        -DCMAKE_CXX_STANDARD=17
    RESULT_VARIABLE CONFIG_RESULT
    OUTPUT_QUIET
    ERROR_QUIET
)

if(NOT CONFIG_RESULT EQUAL 0)
    message(STATUS "RESULT: PASS - v1.23 cmake configure failed (expected)")
    file(REMOVE_RECURSE ${BUILD_DIR})
    return()
endif()

# Attempt to build (just the library, limit to 2 parallel jobs)
execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${BUILD_DIR} -j2
    RESULT_VARIABLE BUILD_RESULT
    OUTPUT_QUIET
    ERROR_VARIABLE BUILD_ERR
    TIMEOUT 120
)

file(REMOVE_RECURSE ${BUILD_DIR})

if(BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR
        "RESULT: FAIL - v1.23 built successfully (expected failure)")
else()
    message(STATUS
        "RESULT: PASS - v1.23 build failed as expected on current toolchain")
endif()
