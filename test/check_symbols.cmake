# Check that libnepcompress does not export nc_def_var_bzip2 or
# nc_inq_var_bzip2; those symbols are provided by NetCDF-C.
#
# Expected variables: NM (nm executable), LIB (library to inspect).
execute_process(COMMAND "${NM}" -D --defined-only "${LIB}"
    OUTPUT_VARIABLE symbols RESULT_VARIABLE result ERROR_VARIABLE nm_error)
if(result)
    message(FATAL_ERROR "nm failed on ${LIB}: ${nm_error}")
endif()
if(symbols MATCHES "nc_(def|inq)_var_bzip2")
    message(FATAL_ERROR "libnepcompress exports bzip2 symbols that belong to NetCDF-C")
endif()
