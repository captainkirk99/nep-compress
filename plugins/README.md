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
