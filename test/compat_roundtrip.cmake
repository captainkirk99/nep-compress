set(_file "${TEST_DIR}/compat.nc")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env "HDF5_PLUGIN_PATH=${WRITE_PLUGIN_DIR}" "NETCDF_RC=${TEST_DIR}" "NCRCENV_RC=${TEST_DIR}/no.ncrc"
        "${EXECUTABLE}" write "${_file}" "${FILTER}"
    RESULT_VARIABLE _written)
if(NOT _written EQUAL 0)
    message(FATAL_ERROR "${FILTER} write failed using ${WRITE_PLUGIN_DIR}")
endif()
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env "HDF5_PLUGIN_PATH=${READ_PLUGIN_DIR}" "NETCDF_RC=${TEST_DIR}" "NCRCENV_RC=${TEST_DIR}/no.ncrc"
        "${EXECUTABLE}" read "${_file}" "${FILTER}"
    RESULT_VARIABLE _read)
if(NOT _read EQUAL 0)
    message(FATAL_ERROR "${FILTER} read failed using ${READ_PLUGIN_DIR}")
endif()
