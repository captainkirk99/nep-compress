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
`64ecbb39c4768dac0855b32a13b02441df1cfed2`. Only comments may be added
to these files; the code must stay identical to upstream.

Files, all from `src/fcidecomp/`:

- `fcicomp-H5Zjpegls`: `src/H5Zjpegls.c`,
  `include/H5Zjpegls{,_definitions,_messages}.h`
- `fcicomp-common`: `src/fcicomp_log.c`,
  `include/fcicomp_{errors,log,options}.h`
- `fcicomp-jpegls`: `src/fcicomp_jpegls.c`,
  `include/fcicomp_jpegls{,_messages}.h`

The plugin links CharLS 2.x (`JpegLsEncode`, `JlsParameters` and friends
from `charls/charls.h`), found at configure time through the `charls`
CMake package or a header/library search. CharLS is not vendored: it is
packaged on conda-forge (`charls`), Spack and Debian/Ubuntu
(`libcharls-dev`).

The EUMETSAT license is in `plugins/COPYING.fcidecomp`, installed under
`share/doc/nepcompress/`.

The fcidecomp code is vendored only because it is not available on Spack
or conda-forge. Once it is, this duplication can be dropped.
