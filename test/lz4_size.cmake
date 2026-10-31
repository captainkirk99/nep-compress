set(_file "${TEST_DIR}/large.nc")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env "HDF5_PLUGIN_PATH=${PLUGIN_DIR}" "NETCDF_RC=${TEST_DIR}" "NCRCENV_RC=${TEST_DIR}/no.ncrc"
        "${EXECUTABLE}" large "${_file}" lz4
    RESULT_VARIABLE _large)
if(NOT _large EQUAL 0)
    message(FATAL_ERROR "lz4 large write/read failed using ${PLUGIN_DIR}")
endif()
file(SIZE "${_file}" _size)
if(_size GREATER MAX_BYTES)
    message(FATAL_ERROR "lz4 file is ${_size} bytes, over the ${MAX_BYTES} limit")
endif()
