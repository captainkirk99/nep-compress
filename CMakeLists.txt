cmake_minimum_required(VERSION 3.18)
project(nepcompress VERSION 1.1.0 LANGUAGES C)

include(GNUInstallDirs)
include(CTest)

option(NEPCOMPRESS_BUILD_LZ4 "Build the LZ4 API and HDF5 plugin" ON)
option(NEPCOMPRESS_BUILD_BZIP2 "Build the BZIP2 API (requires an external HDF5 plugin)" OFF)
option(NEPCOMPRESS_ENABLE_FORTRAN "Build Fortran compression wrappers" ON)
option(NEPCOMPRESS_BUILD_BENCHMARKS "Build slow LZ4/BZIP2 performance examples" OFF)
option(NEPCOMPRESS_BUILD_DOCS "Build Doxygen HTML documentation" OFF)
set(NEPCOMPRESS_DOCS_WARN_AS_ERROR "NO" CACHE STRING
    "Doxygen WARN_AS_ERROR value (e.g. FAIL_ON_WARNINGS)")
set(NEPCOMPRESS_BZIP2_PLUGIN_DIR "" CACHE PATH "Directory containing an HDF5 plugin for filter 307")
set(NEPCOMPRESS_REFERENCE_LZ4_PLUGIN_DIR "" CACHE PATH "Optional previous LZ4 plugin directory for cross-plugin file tests")

if(NOT NEPCOMPRESS_BUILD_LZ4 AND NOT NEPCOMPRESS_BUILD_BZIP2)
    message(FATAL_ERROR "Enable at least one compression filter")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(NETCDF REQUIRED IMPORTED_TARGET netcdf>=4.10.1)
# HDF5 2.x package configs reject a 1.14 minimum-version request even though
# the installed library is newer. The module finder checks the actual version
# and honors HDF5_ROOT instead of a stale cached HDF5_DIR.
set(HDF5_NO_FIND_PACKAGE_CONFIG_FILE TRUE)
find_package(HDF5 1.14 REQUIRED COMPONENTS C HL)

if(NEPCOMPRESS_BUILD_LZ4)
    pkg_check_modules(LZ4 REQUIRED IMPORTED_TARGET liblz4)
endif()

if(NEPCOMPRESS_BUILD_BZIP2)
    if(NOT IS_DIRECTORY "${NEPCOMPRESS_BZIP2_PLUGIN_DIR}")
        message(FATAL_ERROR "Set NEPCOMPRESS_BZIP2_PLUGIN_DIR to a directory with an HDF5 filter 307 plugin")
    endif()
    include(CheckCSourceRuns)
    set(_saved_plugin_path "$ENV{HDF5_PLUGIN_PATH}")
    set(ENV{HDF5_PLUGIN_PATH} "${NEPCOMPRESS_BZIP2_PLUGIN_DIR}")
    set(CMAKE_REQUIRED_INCLUDES "${HDF5_INCLUDE_DIRS}")
    set(CMAKE_REQUIRED_LIBRARIES "${HDF5_LIBRARIES}")
    unset(NEPCOMPRESS_BZIP2_PLUGIN_OK CACHE)
    check_c_source_runs("
#include <hdf5.h>
int main(void) {
    unsigned int flags = 0;
    if (H5open() < 0 || H5Zfilter_avail(307) <= 0 ||
        H5Zget_filter_info(307, &flags) < 0)
        return 1;
    return (flags & (H5Z_FILTER_CONFIG_ENCODE_ENABLED | H5Z_FILTER_CONFIG_DECODE_ENABLED)) ==
           (H5Z_FILTER_CONFIG_ENCODE_ENABLED | H5Z_FILTER_CONFIG_DECODE_ENABLED) ? 0 : 1;
}" NEPCOMPRESS_BZIP2_PLUGIN_OK)
    set(ENV{HDF5_PLUGIN_PATH} "${_saved_plugin_path}")
    unset(CMAKE_REQUIRED_INCLUDES)
    unset(CMAKE_REQUIRED_LIBRARIES)
    if(NOT NEPCOMPRESS_BZIP2_PLUGIN_OK)
        message(FATAL_ERROR "HDF5 filter 307 is unavailable or lacks encode/decode support in NEPCOMPRESS_BZIP2_PLUGIN_DIR")
    endif()
endif()

add_library(nepcompress SHARED src/nep_compress.c)
add_library(NEPCompress::nepcompress ALIAS nepcompress)
target_compile_definitions(nepcompress PRIVATE
    $<$<BOOL:${NEPCOMPRESS_BUILD_LZ4}>:NEPCOMPRESS_BUILD_LZ4>
    $<$<BOOL:${NEPCOMPRESS_BUILD_BZIP2}>:NEPCOMPRESS_BUILD_BZIP2>)
target_include_directories(nepcompress PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_include_directories(nepcompress PRIVATE ${HDF5_INCLUDE_DIRS})
target_link_libraries(nepcompress PUBLIC PkgConfig::NETCDF PRIVATE ${HDF5_LIBRARIES})
set_target_properties(nepcompress PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 1)
install(TARGETS nepcompress EXPORT NEPCompressTargets LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES include/nep_compress.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(NEPCOMPRESS_BUILD_LZ4)
    add_library(h5lz4 MODULE plugins/H5Zlz4.c)
    set_target_properties(h5lz4 PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugin"
        BUILD_RPATH "${HDF5_LIBRARY_DIRS}")
    target_include_directories(h5lz4 PRIVATE plugins ${HDF5_INCLUDE_DIRS})
    target_link_libraries(h5lz4 PRIVATE PkgConfig::LZ4 ${HDF5_LIBRARIES})
    install(TARGETS h5lz4 LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/plugin)
endif()

if(NEPCOMPRESS_ENABLE_FORTRAN)
    enable_language(Fortran)
    pkg_check_modules(NETCDF_FORTRAN REQUIRED IMPORTED_TARGET netcdf-fortran)
    add_library(nepcompressf SHARED fsrc/nep_compress.F90)
    add_library(NEPCompress::nepcompressf ALIAS nepcompressf)
    set_target_properties(nepcompressf PROPERTIES
        VERSION ${PROJECT_VERSION} SOVERSION 1
        Fortran_MODULE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/modules")
    target_compile_definitions(nepcompressf PRIVATE
        $<$<BOOL:${NEPCOMPRESS_BUILD_LZ4}>:NEPCOMPRESS_BUILD_LZ4>
        $<$<BOOL:${NEPCOMPRESS_BUILD_BZIP2}>:NEPCOMPRESS_BUILD_BZIP2>)
    target_include_directories(nepcompressf PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/modules>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/nep-compress>)
    target_link_libraries(nepcompressf PUBLIC nepcompress PkgConfig::NETCDF_FORTRAN)
    install(TARGETS nepcompressf EXPORT NEPCompressTargets LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/modules/nep_compress.mod"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/nep-compress")
endif()

include(CMakePackageConfigHelpers)
configure_package_config_file(NEPCompressConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/NEPCompressConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/NEPCompress")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/NEPCompressConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/NEPCompressConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/NEPCompressConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/NEPCompress")
install(EXPORT NEPCompressTargets
    NAMESPACE NEPCompress:: DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/NEPCompress")

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
if(NEPCOMPRESS_BUILD_BENCHMARKS)
    add_subdirectory(examples/performance)
endif()

if(NEPCOMPRESS_BUILD_DOCS)
    add_custom_target(docs
        COMMAND ${CMAKE_COMMAND}
            -DNEPC_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -DNEPC_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/docs
            -DNEPC_WARN_AS_ERROR=${NEPCOMPRESS_DOCS_WARN_AS_ERROR}
            -DNEPC_VERSION=${PROJECT_VERSION}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/docs/build_docs.cmake
        COMMENT "Building Doxygen documentation")
endif()
