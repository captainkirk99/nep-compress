# Build the nep-compress Doxygen HTML documentation.
#
# Usage:
#   cmake -DNEPC_OUTPUT_DIR=<dir> [-DNEPC_SOURCE_DIR=<dir>]
#         [-DNEPC_WARN_AS_ERROR=NO] [-DNEPC_VERSION=x.y.z]
#         -P docs/build_docs.cmake
#
# NEPC_SOURCE_DIR defaults to the parent directory of this script.
# NEPC_OUTPUT_DIR is required; HTML output lands in
# ${NEPC_OUTPUT_DIR}/html. NEPC_VERSION defaults to the project()
# version parsed from the top-level CMakeLists.txt.

if(NOT DEFINED NEPC_SOURCE_DIR)
    get_filename_component(NEPC_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

if(NOT DEFINED NEPC_OUTPUT_DIR)
    message(FATAL_ERROR "Set -DNEPC_OUTPUT_DIR to the documentation output directory")
endif()

if(NOT DEFINED NEPC_WARN_AS_ERROR)
    set(NEPC_WARN_AS_ERROR NO)
endif()

if(NOT DEFINED NEPC_VERSION)
    file(STRINGS "${NEPC_SOURCE_DIR}/CMakeLists.txt" _nepc_version_line
        REGEX "project\\(nepcompress VERSION")
    string(REGEX MATCH "VERSION[ ]+([0-9]+\\.[0-9]+\\.[0-9]+)" _ "${_nepc_version_line}")
    set(NEPC_VERSION "${CMAKE_MATCH_1}")
    if(NOT NEPC_VERSION)
        message(FATAL_ERROR "Could not parse project version from CMakeLists.txt")
    endif()
endif()

find_program(DOXYGEN_EXECUTABLE doxygen REQUIRED)

configure_file("${NEPC_SOURCE_DIR}/docs/Doxyfile.in"
    "${NEPC_OUTPUT_DIR}/Doxyfile" @ONLY)

execute_process(COMMAND "${DOXYGEN_EXECUTABLE}" "${NEPC_OUTPUT_DIR}/Doxyfile"
    RESULT_VARIABLE _doxygen_result)
if(_doxygen_result)
    message(FATAL_ERROR "Doxygen failed with exit code ${_doxygen_result}")
endif()
