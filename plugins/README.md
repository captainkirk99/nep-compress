# LZ4 plugin provenance

`plugins/H5Zlz4.c` is vendored unchanged from HDF Group hdf5_plugins,
path `LZ4/src/H5Zlz4.c`, at upstream `master` commit
`4d6b8cec9263cf4b328a671cc85239fd058541b7`. Only comments may be added
to it (Doxygen blocks and the vendoring header); the code must stay
byte-for-byte identical to upstream.

Check that rule with:

```sh
diff <(gcc -fpreprocessed -dD -E plugins/H5Zlz4.c | grep -v '^# [0-9]' | grep -v '^[[:space:]]*$') \
     <(gcc -fpreprocessed -dD -E /path/to/hdf5_plugins/LZ4/src/H5Zlz4.c | grep -v '^# [0-9]' | grep -v '^[[:space:]]*$')
```

The output must be empty.

To update to a newer upstream version: copy the new upstream file,
re-add the comment blocks (the `@file` block naming the commit, and
the Doxygen blocks on the static functions, plugin entry points and
macros), rerun the diff above, update the commit hash here and in the
`@file` block, and run ctest.

History: nep-compress 1.1.0 and earlier shipped a fork
(`captainkirk99/hdf5_plugins` branch `e1`, commit `bcb40e0`) that took
`cd_values[0]` as the LZ4 acceleration. Upstream uses `cd_values[0]`
as the block size and `cd_values[1]` as the encoder selector, which is
the registered definition of filter 32004.

The HDF Group license notice is in `plugins/COPYING.hdf5_plugins`; it
is installed under `share/doc/nepcompress/`.

# JPEG-LS plugin provenance

`plugins/fcidecomp/` holds the EUMETSAT fcidecomp JPEG-LS filter code
(HDF5 filter 32018, "FCIDECOMP"), vendored from
`gaubert/fcidecomp-open-repo` (Apache-2.0) at commit
`30ec37d647aeccce5e893ab48957dc30170d6a54`, the last revision before the
CharLS 2.x port. Only comments may be added to these files; the code
must stay identical to upstream, with one exception:

- `fcicomp_jpegls.c` carries one marked patch (search for
  "nep-compress patch"): when the image is multi-component and the
  user left `ilv = 0`, the CharLS call uses `ILV_LINE`. CharLS 1.x
  encodes multi-component `ILV_NONE` streams it cannot decode; the
  JPEG-LS stream header records the mode actually used, so output
  still decodes under CharLS 1.x and 2.x providers.

Files, all from `src/fcidecomp/`:

- `fcicomp-H5Zjpegls`: `src/H5Zjpegls.c`,
  `include/H5Zjpegls{,_definitions,_messages}.h`
- `fcicomp-common`: `src/fcicomp_log.c`,
  `include/fcicomp_{errors,log,options}.h`
- `fcicomp-jpegls`: `src/fcicomp_jpegls.c`,
  `include/fcicomp_jpegls{,_messages}.h`

# CharLS provenance

`plugins/charls-1.1.0/` is the CharLS 1.1.0 source tree
(`team-charls/charls` tag `1.1.0`, commit
`f752a3e31bdf82dff2e1d9f3310950cbe49edeaa`, BSD-3-Clause). CharLS 1.x is
pinned because `fcicomp_jpegls.c` uses the CharLS 1.x C API
(`JlsParameters`, `JpegLsEncode`, `JpegLsDecode`); the plugin must keep
EUMETSAT's exact compression semantics. The `.cpp` files are compiled
into the plugin; no system CharLS is needed.

The EUMETSAT license is in `plugins/COPYING.fcidecomp` and the CharLS
license in `plugins/COPYING.charls`; both are installed under
`share/doc/nepcompress/`.

The fcidecomp and CharLS code is vendored only because neither package
is available on Spack or conda-forge. Once they are, this duplication
can be dropped.
