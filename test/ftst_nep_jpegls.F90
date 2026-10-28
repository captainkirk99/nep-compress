! Test JPEG-LS compression from Fortran.
!
! Author: Edward Hartnett, Intelligent Data Design, Inc.
program ftst_nep_jpegls
  use netcdf
  use nep_compress
  implicit none

  ! This is the name of the data file we will create.
  character (len = *), parameter :: FILE_NAME = "ftst_nep_jpegls.nc"
  integer :: ncid

  ! We are writing 2D imagery data.
  integer, parameter :: NDIMS = 2
  integer, parameter :: NX = 120, NY = 100
  character (len = *), parameter :: X_NAME = "x"
  character (len = *), parameter :: Y_NAME = "y"
  integer :: x_dimid, y_dimid
  integer :: dimids(NDIMS)

  ! Variables: one lossless, one near-lossless, one of an unsupported
  ! type.
  character (len = *), parameter :: EXACT_NAME = "exact"
  character (len = *), parameter :: NEAR2_NAME = "near2"
  character (len = *), parameter :: REAL_NAME = "real_var"
  integer :: exact_varid, near2_varid, real_varid

  integer :: has, near

  ! Data to write.
  integer(kind=2), dimension(NX, NY) :: data_out
  integer(kind=2), dimension(NX, NY) :: data_in
  real, dimension(NX, NY) :: real_out

  ! Loop indices.
  integer :: x, y
  integer :: ndiff

  print *, '*** Testing NEP Fortran JPEG-LS library...'

  ! Create some data to write.
  do y = 1, NY
     do x = 1, NX
        data_out(x, y) = mod(x * 7 + y * 3, 3000) - 1500
        real_out(x, y) = x * y
     end do
  end do

  ! Create the file.
  call check( nf90_create(FILE_NAME, NF90_NETCDF4, ncid) )

  ! Define the dimensions.
  call check( nf90_def_dim(ncid, X_NAME, NX, x_dimid) )
  call check( nf90_def_dim(ncid, Y_NAME, NY, y_dimid) )
  dimids = (/ x_dimid, y_dimid /)

  ! Define the variables and their filters.
  call check( nf90_def_var(ncid, EXACT_NAME, NF90_SHORT, dimids, exact_varid) )
  call check( nf90_def_var_jpegls(ncid, exact_varid, 0) )
  call check( nf90_def_var(ncid, NEAR2_NAME, NF90_SHORT, dimids, near2_varid) )
  call check( nf90_def_var_jpegls(ncid, near2_varid, 2) )

  ! NF90_REAL is not supported by the JPEG-LS filter.
  call check( nf90_def_var(ncid, REAL_NAME, NF90_REAL, dimids, real_varid) )
  if (nf90_def_var_jpegls(ncid, real_varid, 0) .ne. nf90_ebadtype) stop 2

  ! Check the compression settings.
  call check( nf90_inq_var_jpegls(ncid, exact_varid, has, near) )
  if (has .ne. 1 .or. near .ne. 0) stop 2
  call check( nf90_inq_var_jpegls(ncid, near2_varid, has, near) )
  if (has .ne. 1 .or. near .ne. 2) stop 2
  call check( nf90_inq_var_jpegls(ncid, real_varid, has, near) )
  if (has .ne. 0) stop 2

  ! End define mode.
  call check( nf90_enddef(ncid) )

  ! Write the data.
  call check( nf90_put_var(ncid, exact_varid, data_out) )
  call check( nf90_put_var(ncid, near2_varid, data_out) )
  call check( nf90_put_var(ncid, real_varid, real_out) )

  ! Close the file.
  call check( nf90_close(ncid) )

  ! Re-open the file.
  call check( nf90_open(FILE_NAME, nf90_nowrite, ncid) )
  call check( nf90_inq_varid(ncid, EXACT_NAME, exact_varid) )
  call check( nf90_inq_varid(ncid, NEAR2_NAME, near2_varid) )

  ! Check the compression settings.
  call check( nf90_inq_var_jpegls(ncid, exact_varid, has, near) )
  if (has .ne. 1 .or. near .ne. 0) stop 2
  call check( nf90_inq_var_jpegls(ncid, near2_varid, has, near) )
  if (has .ne. 1 .or. near .ne. 2) stop 2

  ! The lossless variable must match exactly.
  call check( nf90_get_var(ncid, exact_varid, data_in) )
  do y = 1, NY
     do x = 1, NX
        if (data_in(x, y) .ne. data_out(x, y)) stop 2
     end do
  end do

  ! The near-lossless variable must be within 2, with at least one
  ! different value.
  ndiff = 0
  call check( nf90_get_var(ncid, near2_varid, data_in) )
  do y = 1, NY
     do x = 1, NX
        if (abs(data_in(x, y) - data_out(x, y)) .gt. 2) stop 2
        if (data_in(x, y) .ne. data_out(x, y)) ndiff = ndiff + 1
     end do
  end do
  if (ndiff .eq. 0) stop 2

  ! Close the file.
  call check( nf90_close(ncid) )

  print *, '*** SUCCESS!!'

contains
  ! Check a NetCDF error status and stop on error.
  !
  ! status - NetCDF error code to check.
  !
  ! Author: Edward Hartnett, Intelligent Data Design, Inc.
  subroutine check(status)
    integer, intent ( in) :: status

    if(status /= nf90_noerr) then
      print *, trim(nf90_strerror(status))
      stop 2
    end if
  end subroutine check
end program ftst_nep_jpegls
