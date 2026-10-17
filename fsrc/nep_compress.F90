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
  function nf90_def_var_lz4(ncid, varid, level) result(status)
    integer, intent(in) :: ncid, varid, level
    integer :: status
    status = c_def_lz4(ncid, varid - 1, level)
  end function

  function nf90_inq_var_lz4(ncid, varid, has, level) result(status)
    integer, intent(in) :: ncid, varid
    integer, intent(inout) :: has, level
    integer :: status
    status = c_inq_lz4(ncid, varid - 1, has, level)
  end function
#endif
#ifdef NEPCOMPRESS_BUILD_BZIP2
  function nf90_def_var_bzip2(ncid, varid, level) result(status)
    integer, intent(in) :: ncid, varid, level
    integer :: status
    status = c_def_bzip2(ncid, varid - 1, level)
  end function

  function nf90_inq_var_bzip2(ncid, varid, has, level) result(status)
    integer, intent(in) :: ncid, varid
    integer, intent(inout) :: has, level
    integer :: status
    status = c_inq_bzip2(ncid, varid - 1, has, level)
  end function
#endif
end module nep_compress
