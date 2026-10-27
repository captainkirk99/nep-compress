# The performance test can only read its zstd-compressed input sample
# when a filter 32015 plugin directory is provided.
if(NOT NEPCOMPRESS_ZSTD_PLUGIN_DIR)
    message(STATUS "NEPCOMPRESS_ZSTD_PLUGIN_DIR not set; skipping performance test")
    return()
endif()

add_executable(compare_filters compare_filters.c)
set_target_properties(compare_filters PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
target_link_libraries(compare_filters PRIVATE nepcompress m)

# One ordered HDF5_PLUGIN_PATH for this test: the build-tree LZ4
# plugin first so its 32004 provider wins over any other on the path,
# then the BZIP2 directory, then the zstd directory. The ':' separator
# is Unix-only; Windows would need ';'.
set(_perf_path "")
if(NEPCOMPRESS_BUILD_LZ4)
    target_sources(compare_filters PRIVATE ${PROJECT_SOURCE_DIR}/test/lz4_provider.c)
    target_include_directories(compare_filters PRIVATE ${PROJECT_SOURCE_DIR}/test)
    target_compile_definitions(compare_filters PRIVATE NEPCOMPRESS_BUILD_LZ4
        NEP_LZ4_PLUGIN_DIR="${PROJECT_BINARY_DIR}/plugin")
    add_dependencies(compare_filters h5lz4)
    list(APPEND _perf_path "${PROJECT_BINARY_DIR}/plugin")
endif()
if(NEPCOMPRESS_BUILD_BZIP2)
    list(APPEND _perf_path "${NEPCOMPRESS_BZIP2_PLUGIN_DIR}")
endif()
list(APPEND _perf_path "${NEPCOMPRESS_ZSTD_PLUGIN_DIR}")
string(JOIN ":" _perf_path ${_perf_path})

set(_wd "${CMAKE_CURRENT_BINARY_DIR}/perf_compare_filters")
file(MAKE_DIRECTORY "${_wd}")
add_test(NAME perf_compare_filters COMMAND compare_filters
    "${PROJECT_SOURCE_DIR}/test/data/gdas_sfc_sample.nc" "${_wd}/compare_filters.csv")
set_tests_properties(perf_compare_filters PROPERTIES
    WORKING_DIRECTORY "${_wd}"
    LABELS performance
    TIMEOUT 900
    ENVIRONMENT "HDF5_PLUGIN_PATH=${_perf_path};NETCDF_RC=${_wd};NCRCENV_RC=${PROJECT_BINARY_DIR}/no.ncrc")
