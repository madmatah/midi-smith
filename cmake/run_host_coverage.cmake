if(NOT DEFINED MIDISMITH_REPO_ROOT)
    message(FATAL_ERROR "MIDISMITH_REPO_ROOT is required")
endif()
if(NOT DEFINED MIDISMITH_BINARY_DIR)
    message(FATAL_ERROR "MIDISMITH_BINARY_DIR is required")
endif()

set(MIDISMITH_COVERAGE_OUTPUT_DIR "${MIDISMITH_BINARY_DIR}/coverage")
file(MAKE_DIRECTORY "${MIDISMITH_COVERAGE_OUTPUT_DIR}")

file(GLOB_RECURSE MIDISMITH_GCDA_FILES "${MIDISMITH_BINARY_DIR}/*.gcda")
if(MIDISMITH_GCDA_FILES)
    file(REMOVE ${MIDISMITH_GCDA_FILES})
endif()

execute_process(
    COMMAND ctest --test-dir "${MIDISMITH_BINARY_DIR}" --output-on-failure
    WORKING_DIRECTORY "${MIDISMITH_REPO_ROOT}"
    RESULT_VARIABLE MIDISMITH_CTEST_RESULT
)
if(NOT MIDISMITH_CTEST_RESULT EQUAL 0)
    message(FATAL_ERROR "ctest failed with exit code ${MIDISMITH_CTEST_RESULT}")
endif()

set(MIDISMITH_GCOVR_COMMON_ARGS
    --root "${MIDISMITH_REPO_ROOT}"
    --object-directory "${MIDISMITH_BINARY_DIR}"
    --filter "libs/.*/src/.*"
    --filter "libs/.*/include/.*"
    --filter "firmwares/.*/app/src/.*"
    --filter "firmwares/.*/app/include/.*"
    --filter "firmwares/.*/domain/src/.*"
    --filter "firmwares/.*/domain/include/.*"
    --exclude ".*/tests/.*"
    --exclude ".*/third_party/.*"
    --exclude ".*/Core/.*"
    --exclude ".*/Drivers/.*"
    --exclude ".*/Middlewares/.*"
    --exclude ".*/USB_DEVICE/.*"
    --exclude ".*/_deps/.*"
    --gcov-exclude-directories ".*/_deps/.*"
)

execute_process(
    COMMAND gcovr
            "${MIDISMITH_BINARY_DIR}"
            ${MIDISMITH_GCOVR_COMMON_ARGS}
            --markdown-summary "${MIDISMITH_COVERAGE_OUTPUT_DIR}/summary.md"
            --markdown-heading-level 2
            --markdown-title "Code Coverage (Host tests)"
            --markdown "${MIDISMITH_COVERAGE_OUTPUT_DIR}/report.md"
            --txt-summary
            --txt "${MIDISMITH_COVERAGE_OUTPUT_DIR}/summary.txt"
            --gcov-ignore-parse-errors all
    WORKING_DIRECTORY "${MIDISMITH_REPO_ROOT}"
    RESULT_VARIABLE MIDISMITH_GCOVR_SUMMARY_RESULT
)
if(NOT MIDISMITH_GCOVR_SUMMARY_RESULT EQUAL 0)
    message(FATAL_ERROR "gcovr summary failed with exit code ${MIDISMITH_GCOVR_SUMMARY_RESULT}")
endif()

execute_process(
    COMMAND gcovr
            "${MIDISMITH_BINARY_DIR}"
            ${MIDISMITH_GCOVR_COMMON_ARGS}
            --txt "${MIDISMITH_COVERAGE_OUTPUT_DIR}/files_sorted_by_uncovered.txt"
            --sort uncovered-number
            --sort-reverse
            --gcov-ignore-parse-errors all
    WORKING_DIRECTORY "${MIDISMITH_REPO_ROOT}"
    RESULT_VARIABLE MIDISMITH_GCOVR_FILES_RESULT
)
if(NOT MIDISMITH_GCOVR_FILES_RESULT EQUAL 0)
    message(FATAL_ERROR "gcovr files report failed with exit code ${MIDISMITH_GCOVR_FILES_RESULT}")
endif()
