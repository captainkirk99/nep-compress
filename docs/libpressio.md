# A generic LibPressio-backed NetCDF filter

## Status and goal

This is a proposed design, not an implemented feature or a committed release
schedule. Source inspection was performed on 2026-11-11; the upstream
LibPressio HDF5 integration has not been built or tested here. Before
implementation, pin an upstream commit and repeat the relevant checks.

The goal is to let an application choose a LibPressio compressor and its
options during NetCDF variable definition, then use ordinary NetCDF reads
and writes. The same HDF5 filter would dispatch to different LibPressio
backends, initially a small supported set such as SZ3 and ZFP.

For example, an application could request an absolute error bound on a
floating-point variable, write it with `nc_put_var_float()`, close the
file, and later read reconstructed values with `nc_get_var_float()`.
The reading application would not call LibPressio itself. It would need
the filter plugin, LibPressio, and the backend used to write the file.

This would extend nep-compress, not replace its existing LZ4 and JPEG-LS
filters. It must use a distinct filter identity and format. A LibPressio
backend using a particular codec is not necessarily compatible with a
standalone HDF5 filter using that codec.

## Why this is useful

Today, adding another compressor generally means integrating a specific
HDF5 plugin, understanding its parameter encoding, and providing C and
Fortran convenience functions. A generic LibPressio filter could expose
multiple backends through one configuration interface and one HDF5 plugin.
It could eventually support selected LibPressio preprocessing pipelines
as well as individual compressors.

The main benefit is access to error-bounded floating-point compression
while preserving the NetCDF programming model. The costs are a larger
runtime dependency graph, a more complex on-disk contract, and dependence
on the availability and compatibility of backend decoders.

A generic interface must not promise that every LibPressio compressor or
option works with every NetCDF datatype, rank, chunk shape, or error target.
Support should be explicit and tested.

## What upstream already provides

LibPressio already has an HDF5 filter under
[`tools/hdf5_filter`](https://github.com/robertu94/libpressio/tree/master/tools/hdf5_filter).
Its top-level build enables that directory when both
`LIBPRESSIO_HAS_HDF` and `LIBPRESSIO_HAS_JSON` are enabled. This is distinct
from its HDF5 I/O plugin.

The filter uses `H5Pset_libpressio()` to attach a compressor name and
options to custom dataset-creation properties. Its `set_local` callback
obtains the datatype and chunk dimensions, reads those properties, and
serializes configuration into HDF5 filter parameters. Its data callback
instantiates the selected compressor and compresses or decompresses a
chunk.

This is useful starting material, but not a ready-made NetCDF interface:

- NetCDF's public `nc_def_var_filter()` accepts a filter ID and an array
  of unsigned integer parameters. It does not expose the HDF5
  dataset-creation property list needed by `H5Pset_libpressio()`.
- The upstream `set_local` callback obtains compressor configuration
  from its custom properties. Merely passing a serialized configuration
  to NetCDF is insufficient unless that callback is adapted to accept it.
- The upstream header defines filter ID 21023 with a comment calling it a
  made-up value and a TODO to replace it. Do not treat this as a
  registered production identity.
- The inspected serializer uses `value & 0x0000FFFF` for the low half of a
  64-bit word. That does not preserve all 32 low bits. Its corresponding
  parser uses a cast to a 64-bit pointer rather than a portable explicit
  reconstruction. Both need review and regression tests.
- Error checking, parameter bounds, exception containment, buffer
  ownership, and compressor creation per chunk need review before
  adopting the implementation.

The repository skill's statement that LibPressio has no HDF5 filter
integration is therefore inaccurate for the inspected upstream source.
That does not establish that the existing filter is production-ready.

## Proposed architecture

```text
Application: C or Fortran
    |
    | define variable, chunks, compressor and typed options
    v
nep-compress convenience API
    |
    | validate and encode configuration
    | nc_def_var_filter(ncid, varid, filter_id, nparams, params)
    v
NetCDF-C -> HDF5 dataset creation
    |
    | plugin set_local: validate type/chunks, finalize stored configuration
    v
Generic LibPressio HDF5 plugin
    |
    | one HDF5 chunk at a time
    v
LibPressio -> selected backend
```

The decoder follows the reverse path. All information needed to select
and configure it must come from the file, not from process-global state,
an environment variable, a sidecar file, or a writer's configuration file.

### The configuration bridge

The preferred approach is to extend the upstream filter so it can accept
a versioned configuration envelope in `cd_values`, in addition to its
existing direct-HDF5 property-list interface. Both paths should converge
on the same validated internal representation and stored format.

The NetCDF wrapper would encode the requested compressor and options into
that envelope. During dataset creation, `set_local` would read it, obtain
the actual HDF5 datatype and chunk dimensions, validate the combination,
and write the finalized parameters using `H5Pmodify_filter()`.

This avoids modifying NetCDF-C merely to pass configuration. It also
avoids accessing private NetCDF HDF5 handles, which would couple the
feature to implementation details and dataset-creation timing.

Changing the upstream plugin requires upstream agreement on the format
and identity. If a separate NEP-maintained plugin is necessary, it needs
its own registered ID and an explicit compatibility policy. Do not ship
an incompatible implementation under upstream's identity. Experimental
files must be clearly identified as disposable and not archival.

### Public C and Fortran interfaces

Design the public API after the configuration experiment works. A possible
C interface would provide:

- A define operation taking `ncid`, `varid`, a compressor name, and typed
  options or a documented configuration string.
- An inquiry operation returning whether the filter is present and the
  stored compressor/configuration, using count-first buffer sizing.
- A capability query distinguishing plugin availability from backend
  availability and supported configurations.

Prefer a NEP-owned interface rather than exposing LibPressio C++ objects
or HDF5 handles. A documented JSON configuration string is convenient for
Fortran and command-line tooling, but requires a precise schema, typed
value rules, size limits, and strict rejection of unknown fields. Typed
C options avoid some ambiguity but require more binding work. Choose one
canonical representation rather than implementing two unrelated systems.

Inquiry should be able to interpret supported metadata without loading
the compressor backend. Defining and writing a variable require a usable
encoder; reading its data requires a usable decoder. Finding the HDF5
filter alone does not prove either backend is available.

Fortran wrappers should follow the existing `nep_compress` module's C
interoperability approach. Define explicit string-length and ownership
rules; do not pass opaque LibPressio objects through Fortran.

## The on-disk contract

A format specification is a prerequisite for persistent production files.
It should define:

1. A magic value, envelope version, and requested/finalized representation.
2. Compressor identity and the exact supported option types.
3. Datatype, rank, chunk dimensions, and dimension-order convention.
4. Required decoder settings, including nested compressor configuration
   when supported.
5. Length encoding, maximum configuration size, and reserved fields.
6. Compatibility rules for future envelope and backend versions.

HDF5 filter parameters are unsigned integer words, not an arbitrary
native-memory structure. Define logical 32-bit word values and explicit
packing/unpacking rules. Reconstruct multiword integers with shifts and
masks; do not cast a word array to wider pointers. Define string and
binary payload packing so HDF5's per-word byte-order conversion does not
change the recovered bytes. Verify actual HDF5 parameter-count and
message-size limits on supported versions before freezing the design.

Do not serialize raw pointers, native structure padding, arbitrary
LibPressio object memory, or a human-readable options dump. If using
LibPressio's JSON conversion and MessagePack, establish whether they
preserve every supported option type and nested configuration. A stable
outer envelope does not automatically make backend bitstreams stable.

Record effective decoder-relevant settings, not just the options the user
supplied. Defaults can change between backend releases. Record software
versions for provenance, but define compatibility through tested decoder
behavior, not merely by comparing version strings. Some backend streams
already contain their own decoding information; document what the outer
format still needs for each supported backend.

## Plugin behavior and correctness

### Datatypes, dimensions, and pipelines

Begin with atomic `NC_FLOAT` and `NC_DOUBLE`, finite data, and backend-
supported ranks and chunk shapes. Reject strings, variable-length types,
compound types, and unsupported integers rather than treating them as raw
bytes. State clearly which special values are supported before accepting
NaN, infinity, or fill-heavy fields.

HDF5 passes file-representation bytes to a filter, not necessarily native
host-order values. Either implement explicit byte-order handling or reject
non-native-endian configurations in the first prototype. Production
support needs a portable policy and cross-endian tests.

Verify LibPressio's dimension convention and each backend adapter's
expectations. Test non-square and asymmetric shapes; a mistaken dimension
order can produce plausible output while changing compression behavior.

Initially support the LibPressio filter alone. In particular, HDF5 shuffle
changes typed values into a byte-reordered stream, so it cannot simply be
placed ahead of a typed lossy compressor. Reject unsupported filter
combinations in `set_local`. Treat NetCDF quantization combined with lossy
compression as a separate accuracy policy, not an implicitly safe default.

HDF5 already provides chunking. Do not add a LibPressio chunking wrapper
by default. The backend sees a complete HDF5 chunk, including padding at
partial dataset edges; test those edges and unlimited-dimension growth.
Chunk-local range-relative error bounds may differ from a user's intended
whole-variable bound. Define these semantics explicitly and start with an
absolute bound to reduce ambiguity.

### Buffer ownership and failures

The filter must distinguish input data length from allocation capacity,
use HDF5-compatible allocation for buffers returned to HDF5, and free the
old buffer only after successful replacement. Check decompressed output
size against the expected chunk size before accepting it.

Validate lengths and arithmetic before allocating. Bound rank, options,
chunk sizes, and decompression sizes. Reject unknown envelope versions,
truncated metadata, missing backends, invalid options, and corrupt streams.
No C++ exception may escape an HDF5 C callback. Report useful HDF5 errors
without printing to stdout or terminating the application.

A significant NetCDF constraint is that the inspected generic-filter
creation path in `libhdf5/nc4hdf.c` calls `H5Pset_filter()` with
`H5Z_FLAG_OPTIONAL`. Returning zero from the compression callback can
therefore allow HDF5 to store a chunk without applying the filter. A
filter listed in metadata is not proof that every chunk was compressed.

Consequently:

- Validate the compressor and options before writing, and fail dataset
  creation for unsupported configurations.
- Test runtime encoder failures and inspect per-chunk filter masks through
  HDF5 in the test suite.
- Decide whether optional uncompressed fallback is acceptable and document
  it. If strict encoder-failure propagation is required, investigate an
  upstream-supported mandatory-filter mechanism, including whether a
  `set_local` flag change is supported reliably. Do not claim strict
  behavior until demonstrated on supported NetCDF/HDF5 versions.

An unfiltered chunk is not inherently corrupt: HDF5 records a filter mask
and bypasses the decoder for that chunk. The problem is silently failing
to meet the requested compression policy or hiding a backend failure.

### Lifecycle and concurrency

Use a fresh compressor per operation in the initial prototype unless the
backend contract requires otherwise. This is simpler to validate, but may
be expensive for small chunks. Measure initialization cost before adding
caches. Any cache must isolate configurations, bound memory, and respect
backend thread-safety and plugin unloading.

Keep automatic tuning out of filter callbacks. Searches on every chunk
would make write latency unpredictable and could select settings that are
not persisted correctly. Tune offline, then store an explicit configuration.

## Implementation steps and acceptance gates

### Phase 1: upstream agreement and a reproducible baseline

- Pin LibPressio and one backend; build the existing HDF5 filter with the
  same HDF5 installation as NetCDF-C.
- Reproduce a direct-HDF5 round trip and document the actual build and
  runtime dependencies.
- Review the serializer, parser, errors, ownership, and current format.
- Discuss the NetCDF parameter bridge, filter registration, and maintenance
  responsibility with upstream.

**Exit gate:** a working baseline and an agreed experimental format path.
Registration and upstream review are external dependencies, not coding
steps with predictable completion dates.

### Phase 2: prove transparent NetCDF operation

- Implement the versioned `cd_values` input path and shared configuration
  validation in an experimental plugin.
- Use `nc_def_var_filter()` from a small C test; do not design the full
  public API yet.
- Write one floating-point variable with explicit chunks and an absolute
  error bound using a single backend.
- Close it, launch a separate reader process, and read it using only normal
  NetCDF calls and the installed plugin stack.
- Inspect finalized parameters and chunk filter masks. Verify missing
  backend behavior, unsupported options, and injected encoder failure.

**Exit gate:** a reproducible fresh-process round trip, a checked error
bound, and a demonstrated policy for optional-filter fallback. This is
the most important feasibility milestone.

### Phase 3: specify and harden the format

- Freeze a reviewed envelope specification and compatibility policy.
- Add strict parsing, size limits, datatype/shape checks, byte-order rules,
  and callback error handling.
- Add a second backend to establish that the design is genuinely generic.
- Restrict initial options and backend versions to combinations exercised
  by tests. Exclude arbitrary scripts, external-command launchers, GPU
  dependencies, and complex meta-compressors from the initial support set.

**Exit gate:** both supported backends work without out-of-file state and
malformed inputs fail safely. Do not turn an untrusted file's configuration
into an unrestricted mechanism for loading or executing arbitrary plugins.

### Phase 4: integrate with nep-compress

- Add OFF-by-default build support for the API and plugin, keeping existing
  LZ4 and JPEG-LS targets unchanged.
- Add C define/inquiry functions and then matching Fortran wrappers.
- Install the plugin in the project's plugin location and test an installed
  consumer with isolated `HDF5_PLUGIN_PATH` settings.
- Decide whether the implementation lives upstream or is a separately
  maintained NEP plugin; avoid duplicate providers of the same ID.
- Add Spack and conda variants only after the dependency and runtime model
  is demonstrated. Check compatibility with each package's HDF5 stack.

**Exit gate:** clean optional builds, installed C/Fortran consumers, and no
new dependency requirement for users who disable the feature.

### Phase 5: validation and performance

Tests should cover:

- Lossless bit-exact and lossy error-bounded round trips, as appropriate.
- Asymmetric dimensions, minimum/maximum supported shapes, partial chunks,
  hyperslabs, unlimited dimensions, and repeated reopen/append operations.
- Fill values and special floating-point values under an explicit policy;
  unchanged missing-value locations and exact categorical data where
  promised. GDAS `land` should not be treated as an ordinary lossy field.
- Two variables using different configurations in one file, and two files
  written in one process, to detect configuration leakage.
- Missing plugin, missing backend, invalid options, truncated parameters,
  corrupt chunks, unsupported versions, and encoder failure/fallback.
- Inspection and configuration inquiry without the backend, where the API
  promises this behavior.
- C and Fortran dimension conventions, non-native-endian data, sanitizers,
  and parser fuzzing.
- Decoder compatibility across the explicitly supported library versions,
  plus NetCDF tools such as `ncdump` and data-copying workflows.

Measure actual NetCDF create/write/close and open/read/close throughput,
whole-file size, error statistics, and memory usage. Compare against
uncompressed data, current lossless filters, and NetCDF quantization plus
zstd or LZ4 at comparable accuracy. Keep in-memory LibPressio codec timings
separate from end-to-end I/O results.

**Exit gate:** correctness and compatibility tests pass; performance is
reported, not enforced through machine-dependent speed thresholds.

### Phase 6: production release

- Obtain the production filter registration and publish the format.
- Resolve any required NetCDF/HDF5 upstream changes and document minimum
  versions.
- Publish reader installation instructions and the supported backend matrix.
- Keep reference files for decoder regression tests and define how old
  files remain readable when dependencies change.
- State that this design covers NetCDF-4/HDF5, not classic NetCDF or NCZarr.
  Parallel I/O is a separate qualification task requiring compatible
  NetCDF/HDF5 builds and backend concurrency tests; it is not implied by
  a successful serial implementation.

**Exit gate:** a stable identity, reproducible reader environment, and an
explicit long-term maintenance commitment.

## How long would it take?

A reliable calendar estimate is not established by source inspection.
The transparent-NetCDF prototype is substantially narrower than a supported
production filter. The largest uncertainties are the configuration bridge,
upstream format agreement and registration, optional-filter failure
semantics, backend compatibility, and cross-platform packaging.

Use the phases above to track progress rather than assigning an unsupported
number of days or weeks:

| Deliverable | What must be resolved first |
| --- | --- |
| Experimental round trip | Matching dependencies and a working parameter bridge |
| Usable opt-in NEP feature | Format, failure policy, validated backend set, C/Fortran API |
| Production release | Registration, compatibility tests, packaging, maintenance ownership |
| Parallel or advanced pipelines | Separate correctness and performance qualification |

Complete Phase 2 before setting a release target. At that point there will
be concrete evidence about whether a small upstream extension is enough
or a separately maintained filter is necessary. Track engineering work
separately from waiting for upstream review, registration, and package
availability. No particular nep-compress release should depend on this
feature until those gates have been resolved.

## Recommendation

Proceed first with an upstream-coordinated, single-backend prototype whose
only goal is a fresh-process NetCDF round trip with persisted options and
a verified absolute error bound. Keep it optional and explicitly
experimental. Do not redesign the existing filters or promise a generic
public API before proving the bridge and failure semantics.

If that experiment succeeds, the remaining work is principally turning a
working adapter into a stable file format and a maintainable reader
installation. Those obligations, rather than the number of compressor
calls in the plugin, determine whether the feature is ready to ship.

## Source references

- [LibPressio top-level build](https://github.com/robertu94/libpressio/blob/master/CMakeLists.txt)
- [HDF5 filter implementation](https://github.com/robertu94/libpressio/blob/master/tools/hdf5_filter/src/libpressio_hdf5_filter.cc)
- [HDF5 property interface](https://github.com/robertu94/libpressio/blob/master/tools/hdf5_filter/src/libpressio_hdf5_props.cc)
- [HDF5 filter header and provisional ID](https://github.com/robertu94/libpressio/blob/master/tools/hdf5_filter/include/libpressio_hdf5_filter.h)
- Local NetCDF-C source inspected: `/fast/ed/netcdf-c/libhdf5/hdf5filter.c`
  and `/fast/ed/netcdf-c/libhdf5/nc4hdf.c`.
- Existing NEP implementation: `src/nep_compress.c`, `CMakeLists.txt`,
  `fsrc/nep_compress.F90`, and `performance/compare_filters.c`.
