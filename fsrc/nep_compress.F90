!> @file
!> @brief Fortran wrappers for the nep-compress LZ4, BZIP2 and JPEG-LS
!> filter API.
!>
!> @author Edward Hartnett, Intelligent Data Design, Inc.

!> @brief Fortran module for LZ4, BZIP2 and JPEG-LS compression of
!> NetCDF-4 variables.
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
#ifdef NEPCOMPRESS_BUILD_JPEGLS
  public :: nf90_def_var_jpegls, nf90_inq_var_jpegls
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
#ifdef NEPCOMPRESS_BUILD_JPEGLS
    integer(c_int) function c_def_jpegls(ncid, varid, near) bind(c, name="nc_def_var_jpegls")
      import c_int
      integer(c_int), value :: ncid, varid, near
    end function
    integer(c_int) function c_inq_jpegls(ncid, varid, has, near) bind(c, name="nc_inq_var_jpegls")
      import c_int
      integer(c_int), value :: ncid, varid
      integer(c_int) :: has, near
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
#ifdef NEPCOMPRESS_BUILD_JPEGLS
  !> @brief Define JPEG-LS compression on a NetCDF-4 variable.
  !>
  !> Must be called in define mode, before data is written. The
  !> variable must be of type NF90_BYTE, NF90_UBYTE, NF90_SHORT or
  !> NF90_USHORT, and its rank must be 2 or 3. If the variable is not
  !> chunked yet, NetCDF-C's default chunk sizes are applied first. The
  !> resulting chunks must satisfy the plugin's requirements: in rank 3
  !> the first chunk dimension is the number of image components and
  !> must be 1-4, the last two chunk dimensions are each at most 65535,
  !> and there must be at least 16 pixels per chunk. Changing the
  !> chunking after this call to a shape outside these limits makes
  !> HDF5 store the data uncompressed without an error. An external
  !> HDF5 plugin that implements filter ID 32018 (EUMETSAT fcidecomp or
  !> hdf5plugin) must be available on HDF5_PLUGIN_PATH.
  !>
  !> @param ncid NetCDF file ID.
  !> @param varid Variable ID (1-based, as in the NetCDF Fortran API).
  !> @param near JPEG-LS near-lossless bound: 0 for lossless; 1-127
  !> (1-byte types) or 1-255 (2-byte types) for near-lossless (lossy)
  !> compression, where every decoded value is within @c near of the
  !> original.
  !> @return NetCDF error code: NF90_NOERR on success, NF90_ENOFILTER
  !> if the JPEG-LS filter plugin is unavailable (checked first),
  !> NF90_EBADTYPE for an unsupported type, NF90_EINVAL for a bad rank,
  !> @c near or chunk shape, or another NetCDF error.
  !>
  !> Calls nc_def_var_jpegls().
  !>
  !> @author Edward Hartnett, Intelligent Data Design, Inc.
  function nf90_def_var_jpegls(ncid, varid, near) result(status)
    integer, intent(in) :: ncid, varid, near
    integer :: status
    status = c_def_jpegls(ncid, varid - 1, near)
  end function

  !> @brief Inquire whether JPEG-LS compression is in use on a NetCDF-4
  !> variable.
  !>
  !> The whole filter pipeline is searched, so filter 32018 is found
  !> whatever its position. The JPEG-LS HDF5 plugin does not need to be
  !> available.
  !>
  !> @param ncid NetCDF file ID.
  !> @param varid Variable ID (1-based).
  !> @param has Required output argument; set to 1 when the variable
  !> uses the JPEG-LS filter and 0 otherwise.
  !> @param near Required output argument; set to the JPEG-LS
  !> near-lossless bound stored with the filter when 32018 is present
  !> with 0, 9 or 13 parameters.
  !> @return NetCDF error code: NF90_NOERR on success, NF90_EFILTER
  !> (with @c has set to 1) if the JPEG-LS filter has a parameter count
  !> other than 0, 9 or 13, or another NetCDF error.
  !>
  !> Calls nc_inq_var_jpegls().
  !>
  !> @author Edward Hartnett, Intelligent Data Design, Inc.
  function nf90_inq_var_jpegls(ncid, varid, has, near) result(status)
    integer, intent(in) :: ncid, varid
    integer, intent(inout) :: has, near
    integer :: status
    status = c_inq_jpegls(ncid, varid - 1, has, near)
  end function
#endif
end module nep_compress
