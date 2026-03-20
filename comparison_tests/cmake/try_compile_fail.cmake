# try_compile_fail.cmake
# Expects: SOURCE_FILE, QL_INCLUDE, BOOST_INCLUDE, CXX_COMPILER
# Returns success (exit 0) if compilation FAILS (expected).
# Returns failure (exit 1) if compilation SUCCEEDS (unexpected).

execute_process(
    COMMAND ${CXX_COMPILER}
        -std=c++17
        -I${QL_INCLUDE}
        -I${BOOST_INCLUDE}
        -c ${SOURCE_FILE}
        -o /dev/null
        -fsyntax-only
    RESULT_VARIABLE COMPILE_RESULT
    OUTPUT_VARIABLE COMPILE_OUT
    ERROR_VARIABLE COMPILE_ERR
)

if(COMPILE_RESULT EQUAL 0)
    message(FATAL_ERROR
        "RESULT: FAIL - Source compiled successfully (expected failure)\n"
        "File: ${SOURCE_FILE}")
else()
    message(STATUS
        "RESULT: PASS - Source failed to compile as expected\n"
        "File: ${SOURCE_FILE}\n"
        "Compiler output: ${COMPILE_ERR}")
endif()
