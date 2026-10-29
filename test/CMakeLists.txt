function(add_compression_test name source plugin_dir)
    add_executable(${name} ${source})
    if(plugin_dir STREQUAL "${PROJECT_BINARY_DIR}/plugin")
        target_sources(${name} PRIVATE lz4_provider.c)
        target_compile_definitions(${name} PRIVATE
            NEP_LZ4_PLUGIN_DIR="${PROJECT_BINARY_DIR}/plugin")
    endif()
    set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/include ${HDF5_INCLUDE_DIRS})
    target_link_libraries(${name} PRIVATE nepcompress ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${name}")
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${name}"
        ENVIRONMENT "HDF5_PLUGIN_PATH=${plugin_dir};NETCDF_RC=${CMAKE_CURRENT_BINARY_DIR}/${name};NCRCENV_RC=${PROJECT_BINARY_DIR}/no.ncrc")
    if(plugin_dir STREQUAL "${PROJECT_BINARY_DIR}/plugin")
        add_dependencies(${name} h5lz4)
    endif()
endfunction()

if(NEPCOMPRESS_BUILD_LZ4)
    add_compression_test(tst_lz4 tst_lz4.c "${PROJECT_BINARY_DIR}/plugin")
    add_compression_test(tst_h_lz4 tst_h_lz4.c "${PROJECT_BINARY_DIR}/plugin")
    add_compression_test(tst_lz4_size tst_lz4_size.c "${PROJECT_BINARY_DIR}/plugin")
    add_compression_test(tst_lz4_filters tst_lz4_filters.c "${PROJECT_BINARY_DIR}/plugin")
endif()
if(NEPCOMPRESS_BUILD_BZIP2)
    add_compression_test(tst_bzip2 tst_bzip2.c "${NEPCOMPRESS_BZIP2_PLUGIN_DIR}")
    add_compression_test(tst_h_bzip2 tst_h_bzip2.c "${NEPCOMPRESS_BZIP2_PLUGIN_DIR}")
endif()
if(NEPCOMPRESS_BUILD_JPEGLS)
    add_compression_test(tst_jpegls tst_jpegls.c "${NEPCOMPRESS_JPEGLS_PLUGIN_DIR}")
    set_tests_properties(tst_jpegls PROPERTIES FIXTURES_SETUP jpegls_file)
    add_test(NAME jpegls_missing_plugin COMMAND tst_jpegls missing)
    set_tests_properties(jpegls_missing_plugin PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tst_jpegls"
        FIXTURES_REQUIRED jpegls_file
        ENVIRONMENT "HDF5_PLUGIN_PATH=${PROJECT_BINARY_DIR}/missing;NETCDF_RC=${CMAKE_CURRENT_BINARY_DIR}/tst_jpegls;NCRCENV_RC=${PROJECT_BINARY_DIR}/no.ncrc")
endif()

if(NEPCOMPRESS_ENABLE_FORTRAN)
    if(NEPCOMPRESS_BUILD_LZ4)
        add_executable(ftst_nep_lz4 ftst_nep_lz4.F90)
        set_target_properties(ftst_nep_lz4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
        target_link_libraries(ftst_nep_lz4 PRIVATE nepcompressf ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})
        add_dependencies(ftst_nep_lz4 nepcompressf h5lz4)
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ftst_nep_lz4")
        add_test(NAME ftst_nep_lz4 COMMAND ftst_nep_lz4)
        set_tests_properties(ftst_nep_lz4 PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ftst_nep_lz4"
            ENVIRONMENT "HDF5_PLUGIN_PATH=${PROJECT_BINARY_DIR}/plugin;NETCDF_RC=${CMAKE_CURRENT_BINARY_DIR}/ftst_nep_lz4;NCRCENV_RC=${PROJECT_BINARY_DIR}/no.ncrc")
    endif()
    if(NEPCOMPRESS_BUILD_BZIP2)
        add_executable(ftst_nep_bzip2 ftst_nep_bzip2.F90)
        set_target_properties(ftst_nep_bzip2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
        target_link_libraries(ftst_nep_bzip2 PRIVATE nepcompressf ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ftst_nep_bzip2")
        add_test(NAME ftst_nep_bzip2 COMMAND ftst_nep_bzip2)
        set_tests_properties(ftst_nep_bzip2 PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ftst_nep_bzip2"
            ENVIRONMENT "HDF5_PLUGIN_PATH=${NEPCOMPRESS_BZIP2_PLUGIN_DIR};NETCDF_RC=${CMAKE_CURRENT_BINARY_DIR}/ftst_nep_bzip2;NCRCENV_RC=${PROJECT_BINARY_DIR}/no.ncrc")
    endif()
    if(NEPCOMPRESS_BUILD_JPEGLS)
        add_executable(ftst_nep_jpegls ftst_nep_jpegls.F90)
        set_target_properties(ftst_nep_jpegls PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
        target_link_libraries(ftst_nep_jpegls PRIVATE nepcompressf ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ftst_nep_jpegls")
        add_test(NAME ftst_nep_jpegls COMMAND ftst_nep_jpegls)
        set_tests_properties(ftst_nep_jpegls PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ftst_nep_jpegls"
            ENVIRONMENT "HDF5_PLUGIN_PATH=${NEPCOMPRESS_JPEGLS_PLUGIN_DIR};NETCDF_RC=${CMAKE_CURRENT_BINARY_DIR}/ftst_nep_jpegls;NCRCENV_RC=${PROJECT_BINARY_DIR}/no.ncrc")
    endif()
endif()

add_executable(compat_file compat_file.c)
target_link_libraries(compat_file PRIVATE nepcompress ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})
target_compile_definitions(compat_file PRIVATE
    $<$<BOOL:${NEPCOMPRESS_BUILD_LZ4}>:NEPCOMPRESS_BUILD_LZ4>
    $<$<BOOL:${NEPCOMPRESS_BUILD_BZIP2}>:NEPCOMPRESS_BUILD_BZIP2>)

function(add_compat_test name filter writer reader)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${name}")
    add_test(NAME ${name} COMMAND "${CMAKE_COMMAND}"
        "-DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/${name}"
        "-DEXECUTABLE=$<TARGET_FILE:compat_file>"
        "-DFILTER=${filter}"
        "-DWRITE_PLUGIN_DIR=${writer}"
        "-DREAD_PLUGIN_DIR=${reader}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/compat_roundtrip.cmake")
endfunction()

if(NEPCOMPRESS_BUILD_LZ4)
    add_compat_test(lz4_roundtrip lz4 "${PROJECT_BINARY_DIR}/plugin" "${PROJECT_BINARY_DIR}/plugin")
    add_test(NAME lz4_missing_plugin COMMAND "${CMAKE_COMMAND}" -E env
        "HDF5_PLUGIN_PATH=${PROJECT_BINARY_DIR}/missing" "NETCDF_RC=${PROJECT_BINARY_DIR}" "NCRCENV_RC=${PROJECT_BINARY_DIR}/no.ncrc"
        "$<TARGET_FILE:compat_file>" missing
        "${CMAKE_CURRENT_BINARY_DIR}/lz4_missing.nc" lz4)
    if(IS_DIRECTORY "${NEPCOMPRESS_REFERENCE_LZ4_PLUGIN_DIR}")
        add_compat_test(lz4_new_to_reference lz4
            "${PROJECT_BINARY_DIR}/plugin" "${NEPCOMPRESS_REFERENCE_LZ4_PLUGIN_DIR}")
        add_compat_test(lz4_reference_to_new lz4
            "${NEPCOMPRESS_REFERENCE_LZ4_PLUGIN_DIR}" "${PROJECT_BINARY_DIR}/plugin")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lz4_wrong_provider")
        add_test(NAME lz4_wrong_provider COMMAND tst_lz4)
        set_tests_properties(lz4_wrong_provider PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lz4_wrong_provider"
            ENVIRONMENT "HDF5_PLUGIN_PATH=${NEPCOMPRESS_REFERENCE_LZ4_PLUGIN_DIR};NETCDF_RC=${CMAKE_CURRENT_BINARY_DIR}/lz4_wrong_provider;NCRCENV_RC=${PROJECT_BINARY_DIR}/no.ncrc"
            PASS_REGULAR_EXPRESSION "unexpected LZ4 provider")
    endif()
endif()
if(NEPCOMPRESS_BUILD_BZIP2)
    add_compat_test(bzip2_roundtrip bzip2
        "${NEPCOMPRESS_BZIP2_PLUGIN_DIR}" "${NEPCOMPRESS_BZIP2_PLUGIN_DIR}")
    add_test(NAME bzip2_missing_plugin COMMAND "${CMAKE_COMMAND}" -E env
        "HDF5_PLUGIN_PATH=${PROJECT_BINARY_DIR}/missing" "NETCDF_RC=${PROJECT_BINARY_DIR}" "NCRCENV_RC=${PROJECT_BINARY_DIR}/no.ncrc"
        "$<TARGET_FILE:compat_file>" missing
        "${CMAKE_CURRENT_BINARY_DIR}/bzip2_missing.nc" bzip2)
endif()

if(CMAKE_NM AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_test(NAME nepcompress_no_bzip2_symbols COMMAND "${CMAKE_COMMAND}"
        "-DNM=${CMAKE_NM}" "-DLIB=$<TARGET_FILE:nepcompress>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_symbols.cmake")
endif()
