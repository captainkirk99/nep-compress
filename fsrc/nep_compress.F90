!> @file
!> @brief Fortran wrappers for the nep-compress LZ4 and BZIP2 filter
!> API.
!>
!> @author Edward Hartnett, Intelligent Data Design, Inc.

!> @brief Fortran module for LZ4 and BZIP2 compression of NetCDF-4
!> variables.
!>
!> These wrappers call the C functions declared in nep_compress.h and
!> netcdf_filter.h. As usual in
!> the NetCDF Fortran API, @c varid is 1-based; the wrappers subtract 1
!> before calling the C API.
!>
!> @author Edward Hartnett, Intelligent Data Design, Inc.
module nep_compress
  use iso_c_binding, only: c_int
  implicit none
  private

#ifdef NEPCOMPRESS_BUILD_LZ4
  public :: nf90_def_var_lz4, nf90_inq_var_lz4
#endif
#ifdef NEPCOMPRESS_BUILD_BZIP2
  public :: nf90_def_var_bzip2, nf90_inq_var_bzip2
#endif

  interface
#ifdef NEPCOMPRESS_BUILD_LZ4
    integer(c_int) function c_def_lz4(ncid, varid, level) bind(c, name="nc_def_var_lz4")
      import c_int
      integer(c_int), value :: ncid, varid, level
    end function
    integer(c_int) function c_inq_lz4(ncid, varid, has, level) bind(c, name="nc_inq_var_lz4")
      import c_int
      integer(c_int), value :: ncid, varid
      integer(c_int) :: has, level
    end function
#endif
#ifdef NEPCOMPRESS_BUILD_BZIP2
    integer(c_int) function c_def_bzip2(ncid, varid, level) bind(c, name="nc_def_var_bzip2")
      import c_int
      integer(c_int), value :: ncid, varid, level
    end function
    integer(c_int) function c_inq_bzip2(ncid, varid, has, level) bind(c, name="nc_inq_var_bzip2")
      import c_int
      integer(c_int), value :: ncid, varid
      integer(c_int) :: has, level
    end function
#endif
  end interface
contains
#ifdef NEPCOMPRESS_BUILD_LZ4
  !> @brief Define LZ4 compression on a NetCDF-4 variable.
  !>
  !> Must be called in define mode, before data is written. Applying
  !> the filter forces chunked storage for the variable. The LZ4 HDF5
  !> plugin must be available on HDF5_PLUGIN_PATH.
  !>
  !> @param ncid NetCDF file ID.
  !> @param varid Variable ID (1-based, as in the NetCDF Fortran API).
  !> @param level LZ4 acceleration level, 1 to 9.
  !> @return NetCDF error code: NF90_NOERR on success, NF90_ENOFILTER
  !> if the LZ4 filter plugin is unavailable (checked before the
  !> level), NF90_EINVAL for an invalid level, or another NetCDF
  !> error.
  !>
  !> @author Edward Hartnett, Intelligent Data Design, Inc.
  function nf90_def_var_lz4(ncid, varid, level) result(status)
    integer, intent(in) :: ncid, varid, level
    integer :: status
    status = c_def_lz4(ncid, varid - 1, level)
  end function

  !> @brief Inquire whether LZ4 compression is in use on a NetCDF-4
  !> variable.
  !>
  !> The whole filter pipeline is searched, so LZ4 is found whatever
  !> its position. The LZ4 HDF5 plugin does not need to be available.
  !>
  !> @param ncid NetCDF file ID.
  !> @param varid Variable ID (1-based).
  !> @param has Required output argument; set to 1 when the variable
  !> uses the LZ4 filter and 0 otherwise.
  !> @param level Required output argument; set to the LZ4 acceleration
  !> level stored with the filter. Written only when LZ4 is present
  !> with exactly one parameter.
  !> @return NetCDF error code: NF90_NOERR on success, NF90_EFILTER
  !> (with @c has set to 1) if the LZ4 filter has a parameter count
  !> other than 1, or another NetCDF error.
  !>
  !> @author Edward Hartnett, Intelligent Data Design, Inc.
  function nf90_inq_var_lz4(ncid, varid, has, level) result(status)
    integer, intent(in) :: ncid, varid
    integer, intent(inout) :: has, level
    integer :: status
    status = c_inq_lz4(ncid, varid - 1, has, level)
  end function
#endif
#ifdef NEPCOMPRESS_BUILD_BZIP2
  !> @brief Define BZIP2 compression on a NetCDF-4 variable.
  !>
  !> Must be called in define mode, before data is written. Applying
  !> the filter forces chunked storage for the variable. An external
  !> HDF5 plugin that implements filter ID 307 must be available on
  !> HDF5_PLUGIN_PATH.
  !>
  !> @param ncid NetCDF file ID.
  !> @param varid Variable ID (1-based, as in the NetCDF Fortran API).
  !> @param level BZIP2 compression level, 1 to 9 (block sizes of
  !> 100-900 KB).
  !> @return NetCDF error code: NF90_NOERR on success, NF90_ENOFILTER
  !> if the BZIP2 filter plugin is unavailable (checked before the
  !> level), NF90_EINVAL for an invalid level, or another NetCDF
  !> error.
  !>
  !> Calls NetCDF-C's nc_def_var_bzip2().
  !>
  !> @author Edward Hartnett, Intelligent Data Design, Inc.
  function nf90_def_var_bzip2(ncid, varid, level) result(status)
    integer, intent(in) :: ncid, varid, level
    integer :: status
    status = c_def_bzip2(ncid, varid - 1, level)
  end function

  !> @brief Inquire whether BZIP2 compression is in use on a NetCDF-4
  !> variable.
  !>
  !> @param ncid NetCDF file ID.
  !> @param varid Variable ID (1-based).
  !> @param has Required output argument; set to 1 when the variable
  !> uses the BZIP2 filter and 0 otherwise.
  !> @param level Required output argument; set to the BZIP2
  !> compression level stored with the filter. Written only when BZIP2
  !> is present with exactly one parameter.
  !> @return NetCDF error code: NF90_NOERR on success, NF90_ENOFILTER
  !> if the BZIP2 filter plugin is unavailable, NF90_EFILTER (with
  !> @c has set to 1) if filter 307 has a parameter count other than
  !> 1, or another NetCDF error.
  !>
  !> Calls NetCDF-C's nc_inq_var_bzip2().
  !>
  !> @author Edward Hartnett, Intelligent Data Design, Inc.
  function nf90_inq_var_bzip2(ncid, varid, has, level) result(status)
    integer, intent(in) :: ncid, varid
    integer, intent(inout) :: has, level
    integer :: status
    status = c_inq_bzip2(ncid, varid - 1, has, level)
  end function
#endif
end module nep_compress
