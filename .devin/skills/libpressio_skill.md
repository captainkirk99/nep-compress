# LibPressio

"Pressio" is Latin for compression. LibPressio is an open-source C++ library with C-compatible bindings that provides a generic abstraction layer for both lossless and lossy data compression, specifically targeting dense numerical tensors (arrays). Developed by Robert Underwood and collaborators at Clemson University and Argonne National Laboratory, it solves the problem of having to write separate application-level code for each compressor. Instead, users write code once against the LibPressio API, and the library dispatches to the correct underlying compressor.

## Problem Solved

Scientific simulation codes (climate, combustion, cosmology, etc.) generate data sets that are too large to store or transfer without compression. Many error-bounded lossy compressors exist (SZ, ZFP, MGARD, etc.), but each has a unique API, configuration model, and build system. LibPressio rationalizes this complexity by providing:

* A single, stable API for compress/decompress across all supported backends.
* A uniform options system (`pressio_options`) for configuring any compressor.
* A metrics framework for measuring compression quality, speed, and error statistics without extra application code.
* An extensible plugin architecture so new compressors and metrics can be added without changing the core library.

## Architecture

LibPressio has four major abstractions:

### 1. `pressio_data`
Represents input and output data — including buffer pointer, data type (float, double, int8, etc.), number of dimensions, and dimension sizes. Handles memory ownership.

### 2. `pressio_compressor`
A plugin handle that exposes three operations:
* `compress(input, output)` — compress the input buffer into the output buffer.
* `decompress(input, output)` — decompress.
* `set_options` / `get_options` — configure or inspect the compressor.

### 3. `pressio_options`
A key-value map where keys are namespaced strings (e.g., `sz:abs_err_bound`, `zfp:rate`, `pressio:abs`) and values are typed scalars or arrays. Options are the universal configuration mechanism for both compressors and metrics.

### 4. `pressio_metrics`
A composable set of measurement plugins that run alongside compression/decompression and report results (compression ratio, PSNR, SSIM, compression/decompression time, error statistics, etc.) without requiring changes to application code.

### IO Extension
`pressio_io` plugins handle loading and storing data from/to persistent storage (binary files, HDF5, etc.), enabling full pipelines from file to compressed output.

## Key Features

* **Language bindings**: C (stable ABI), C++ (idiomatic), Python (`+python`), HDF5 (`+hdf5+json`), R (separate package), Bash/CLI (`pressio-tools`), Julia (experimental), Rust (experimental).
* **Composable compressor pipeline**: Meta-compressors like `chunking`, `transpose`, `delta_encoding`, `log_transform`, `linear_quantizer`, `binning`, and `roibin` can wrap any backend compressor, enabling preprocessing + compression pipelines defined entirely through options.
* **Distributed/parallel support**: `many_independent`, `many_dependent`, and `many_independent_threaded` plugins compress multiple fields in parallel (optionally via MPI).
* **Auto-tuning**: The `opt` meta-compressor wraps any backend and a search algorithm to automatically find the best configuration to meet a target metric (e.g., minimize compressed size subject to `pressio:abs` error bound).
* **Negligible overhead**: Benchmarks in the original IEEE TPDS paper show less than 1% overhead versus calling compressors natively, while reducing application lines of code by 30–70%.
* **Composable metrics**: Multiple metrics modules can be combined into a single `composite` metric; results are extracted from `pressio_options` after compression.

## Supported Compressors (1st-party plugins)

| Plugin | Type | Notes |
|--------|------|-------|
| `sz` | Lossy | SZ 1/2 error-bounded prediction-based compressor |
| `sz3` | Lossy | SZ3 with interpolation-based prediction |
| `sz_omp` / `sz_threadsafe` | Lossy | OpenMP and thread-safe SZ variants |
| `zfp` | Lossy | ZFP fixed-rate, fixed-precision, fixed-accuracy modes |
| `qoz` | Lossy | QoZ quality-oriented SZ3 variant |
| `cusz` | Lossy | CUDA GPU-accelerated SZ-family compressor |
| `sperr` | Lossy | Wavelet-based, supports absolute error and PSNR targets |
| `tthresh` | Lossy | Tucker-decomposition based, targets PSNR/RMSE |
| `fpzip` | Lossy/Lossless | Floating-point data compressor |
| `mgard` | Lossy | Multigrid-based, rigorous QoI error bounds |
| `blosc` | Lossless | Multi-codec framework (LZ4, Zstd, Blosclz, etc.) |
| `bzip2` | Lossless | BZip2 |
| `ndzip` | Lossless | GPU-accelerated lossless compressor for floating-point |
| `nvcomp` | Lossless | NVIDIA GPU compression (Snappy, LZ4, Deflate, Bitcomp, ANS) |
| `magick` | Lossy | ImageMagick-based (JPEG, PNG, etc. — for 2D image data) |
| `noop` | — | Pass-through (for benchmarking pipeline overhead) |

### Compositing / Meta-Compressors

| Plugin | Purpose |
|--------|---------|
| `chunking` | Compress data in fixed-size chunks with a sub-compressor |
| `transpose` | Reorder array dimensions before compression |
| `delta_encoding` | Difference encoding pre-pass |
| `log_transform` | Log/exp pre/post-processing |
| `linear_quantizer` | Quantize to integers, then apply a sub-compressor |
| `binning` | Spatial downsampling pre-pass |
| `roibin` | Region-of-interest binning (different quality in/outside ROI) |
| `pw_rel` | Point-wise relative error bound wrapper |
| `opt` | Automated parameter search/tuning |
| `many_independent` | Compress independent fields in parallel (MPI-aware) |
| `many_dependent` | Compress dependent fields sequentially with MPI scatter/gather |
| `many_independent_threaded` | Threaded parallel compression of multiple fields |
| `switch` | Runtime selection among multiple named sub-compressors |
| `manifest` | Records compression lineage/provenance metadata |
| `repeat` | Repeat compression N times (for timing benchmarks) |
| `resize` | Reshape data before/after compression |
| `sample` | Subsample data for fast estimation |
| `lambda_fn` | Inject custom Lua or Python scripts as a compressor step |

## Common Pressio-Namespace Options

These options are recognized across all compressors that support them:

| Option | Meaning |
|--------|---------|
| `pressio:abs` | Absolute error bound |
| `pressio:rel` | Relative error bound (fraction of value range) |
| `pressio:pw_rel` | Point-wise relative error bound |
| `pressio:lossless` | Force lossless mode (0 = lossy, 1 = lossless) |
| `pressio:nthreads` | Number of threads |
| `pressio:metric` | Comma-separated list of metrics to collect |
| `pressio:version` | Version string (read-only) |

## Core API Headers

| Header | Purpose |
|--------|---------|
| `pressio.h` | Error reporting and acquiring compressor handles |
| `pressio_compressor.h` | Compress/decompress operations |
| `pressio_data.h` | Data buffer representation |
| `pressio_options.h` | Key-value configuration map |
| `pressio_metrics.h` | Metrics collection framework |
| `pressio_io.h` | Persistent storage I/O plugins |
| `libpressio.h` | Convenience header including all of the above |
| `libpressio_ext/io/posix.h` | POSIX file I/O helper (`pressio_io_data_path_read`) |

## C API Usage

The C API is the stable ABI interface. All types are opaque pointers managed through LibPressio's lifecycle functions. The general pattern is:

1. Create a `pressio` library instance.
2. Allocate `pressio_data` descriptors for input, compressed output, and decompressed output.
3. Obtain a `pressio_compressor` plugin by name.
4. Configure metrics and compressor settings via `pressio_options`.
5. Call `pressio_compressor_compress` and `pressio_compressor_decompress`.
6. Retrieve metric results from a `pressio_options` struct.
7. Free all resources in reverse order.

### Listing Available Plugins

```c
#include <libpressio.h>
#include <libpressio_ext/io/pressio_io.h>
#include <stdio.h>

int main(void)
{
    printf("compressors [%s]\n", pressio_supported_compressors());
    printf("metrics    [%s]\n", pressio_supported_metrics());
    printf("io         [%s]\n", pressio_supported_io_modules());
    return 0;
}
```

### Querying Compressor Options

```c
#include <libpressio.h>
#include <pressio_version.h>
#include <stdio.h>
#include <stdlib.h>

void print_options(struct pressio_options* options) {
    char* s = pressio_options_to_string(options);
    fprintf(stdout, "%s\n\n", s);
    free(s);
}

int main(int argc, char *argv[])
{
    struct pressio* library = pressio_instance();
    struct pressio_compressor* comp = pressio_get_compressor(library, argv[1]);

    struct pressio_options* opts = pressio_compressor_get_options(comp);
    printf("runtime configuration\n");
    print_options(opts);
    pressio_options_free(opts);

    struct pressio_options* cfg = pressio_compressor_get_configuration(comp);
    printf("compile-time configuration\n");
    print_options(cfg);
    pressio_options_free(cfg);

#if LIBPRESSIO_MAJOR_VERSION > 1 || LIBPRESSIO_MINOR_VERSION >= 65
    struct pressio_options* doc = pressio_compressor_get_documentation(comp);
    printf("documentation\n");
    print_options(doc);
    pressio_options_free(doc);
#endif

    pressio_compressor_release(comp);
    pressio_release(library);
    return 0;
}
```

### Compress and Decompress with Metrics (Full Example)

This example reads a 3-D float32 array from disk, compresses it with SZ at several absolute error bounds, decompresses, and prints time/size/error statistics for each bound.

```c
#include <stdio.h>
#include <stdlib.h>
#include <libpressio.h>
#include <libpressio_ext/io/posix.h>

int main(void)
{
    /* --- describe the input data --- */
    size_t dims[] = {100, 500, 500};
    size_t ndims = sizeof(dims) / sizeof(dims[0]);
    struct pressio_data* metadata =
        pressio_data_new_empty(pressio_float_dtype, ndims, dims);
    struct pressio_data* input_data =
        pressio_io_data_path_read(metadata, "CLOUDf48.bin.f32");

    /* --- allocate output buffers --- */
    struct pressio_data* compressed =
        pressio_data_new_empty(pressio_byte_dtype, 0, NULL);
    struct pressio_data* output = pressio_data_new_clone(input_data);

    /* --- get a compressor plugin --- */
    struct pressio* library = pressio_instance();
    struct pressio_compressor* comp =
        pressio_get_compressor(library, "sz");

    /* --- configure composite metrics --- */
    struct pressio_options* metrics_opts = pressio_options_new();
    const char* metrics_ids[] = {"time", "size", "error_stat"};
    size_t n = sizeof(metrics_ids) / sizeof(metrics_ids[0]);
    pressio_options_set_string(metrics_opts, "pressio:metric", "composite");
    pressio_options_set_strings(metrics_opts, "composite:plugins", n, metrics_ids);

    if (pressio_compressor_check_options(comp, metrics_opts)) {
        fprintf(stderr, "%s\n", pressio_compressor_error_msg(comp));
        return pressio_compressor_error_code(comp);
    }
    if (pressio_compressor_set_options(comp, metrics_opts)) {
        fprintf(stderr, "%s\n", pressio_compressor_error_msg(comp));
        return pressio_compressor_error_code(comp);
    }
    pressio_options_free(metrics_opts);

    /* --- sweep error bounds --- */
    double bounds[] = {1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1};
    size_t n_bounds = sizeof(bounds) / sizeof(bounds[0]);
    for (size_t i = 0; i < n_bounds; ++i) {
        struct pressio_options* opts = pressio_options_new();
        pressio_options_set_double(opts, "pressio:abs", bounds[i]);

        if (pressio_compressor_check_options(comp, opts)) {
            fprintf(stderr, "%s\n", pressio_compressor_error_msg(comp));
            return pressio_compressor_error_code(comp);
        }
        if (pressio_compressor_set_options(comp, opts)) {
            fprintf(stderr, "%s\n", pressio_compressor_error_msg(comp));
            return pressio_compressor_error_code(comp);
        }
        pressio_options_free(opts);

        if (pressio_compressor_compress(comp, input_data, compressed)) {
            fprintf(stderr, "%s\n", pressio_compressor_error_msg(comp));
            return pressio_compressor_error_code(comp);
        }
        if (pressio_compressor_decompress(comp, compressed, output)) {
            fprintf(stderr, "%s\n", pressio_compressor_error_msg(comp));
            return pressio_compressor_error_code(comp);
        }

        struct pressio_options* results =
            pressio_compressor_get_metrics_results(comp);
        char* results_str = pressio_options_to_string(results);
        printf("bound=%1.1e\n%s\n", bounds[i], results_str);
        free(results_str);
        pressio_options_free(results);
    }

    /* --- cleanup --- */
    pressio_data_free(metadata);
    pressio_data_free(input_data);
    pressio_data_free(compressed);
    pressio_data_free(output);
    pressio_compressor_release(comp);
    pressio_release(library);
    return 0;
}
```

### Key C API Functions

| Function | Purpose |
|----------|---------|
| `pressio_instance()` | Create the library handle (one per process) |
| `pressio_release(lib)` | Free the library handle |
| `pressio_get_compressor(lib, id)` | Obtain a compressor plugin by string ID |
| `pressio_compressor_release(comp)` | Free a compressor handle |
| `pressio_data_new_empty(dtype, ndims, dims)` | Allocate a data descriptor with no buffer |
| `pressio_data_new_clone(src)` | Deep-copy a data descriptor and its buffer |
| `pressio_data_free(data)` | Free a data descriptor |
| `pressio_options_new()` | Create an empty options map |
| `pressio_options_set_double(opts, key, val)` | Set a double option |
| `pressio_options_set_string(opts, key, val)` | Set a string option |
| `pressio_options_set_strings(opts, key, n, vals)` | Set a string array option |
| `pressio_options_get_double(opts, key, &val)` | Read a double option |
| `pressio_options_to_string(opts)` | Serialize all options to a human-readable string (caller must `free`) |
| `pressio_options_free(opts)` | Free an options map |
| `pressio_compressor_get_options(comp)` | Get current runtime options |
| `pressio_compressor_get_configuration(comp)` | Get compile-time configuration |
| `pressio_compressor_get_documentation(comp)` | Get option documentation (≥ v0.65) |
| `pressio_compressor_check_options(comp, opts)` | Validate options before setting |
| `pressio_compressor_set_options(comp, opts)` | Apply options to the compressor |
| `pressio_compressor_compress(comp, in, out)` | Compress; returns non-zero on error |
| `pressio_compressor_decompress(comp, in, out)` | Decompress; returns non-zero on error |
| `pressio_compressor_get_metrics_results(comp)` | Retrieve metrics results after compress/decompress |
| `pressio_compressor_error_msg(comp)` | Get the last error message string |
| `pressio_compressor_error_code(comp)` | Get the last error code (non-zero = error) |
| `pressio_io_data_path_read(meta, path)` | Read a raw binary file into a data buffer |
| `pressio_supported_compressors()` | Comma-separated list of compiled-in compressor IDs |
| `pressio_supported_metrics()` | Comma-separated list of compiled-in metrics IDs |
| `pressio_supported_io_modules()` | Comma-separated list of compiled-in IO IDs |

### Building with Make

LibPressio ships a `pkg-config` file. A minimal `Makefile`:

```makefile
LDLIBS  = $(shell pkg-config --libs libpressio)
CFLAGS  = -std=c99 -g $(shell pkg-config --cflags libpressio)
ALL     = basics list_compressors get_compressor_options

all: $(ALL)
clean:
	$(RM) $(ALL) $(wildcard *.o)
```

### Building with CMake

```cmake
find_package(LibPressio REQUIRED)
target_link_libraries(my_target PRIVATE LibPressio::libpressio)
```

## CLI Usage (pressio-tools)

```bash
pressio -i /path/to/data.bin.f32 \
    -b compressor=sz3 -o abs=1e-4 -O all \
    -m time -m size -m error_stat -M all \
    -w /path/to/output.dec
```

## Installation

### Spack (Recommended)

```bash
git clone https://github.com/spack/spack
source ./spack/share/spack/setup-env.sh
spack install libpressio+sz3
```

Available variants are listed via `spack info libpressio`. Each compressor plugin is enabled by its corresponding `+<name>` Spack variant (e.g., `+zfp`, `+sz`, `+mgard`, `+blosc`).

### Manual (CMake)

Unconditional dependencies:
* `cmake`
* `pkg-config`
* [`std_compat`](https://github.com/robertu94/std_compat)
* GCC ≥ 4.8.5 or Clang ≥ 7.0.0

Optional compressor backends are enabled via `LIBPRESSIO_HAS_*` CMake variables. All are **disabled by default**.

## Relationship to NetCDF/HDF5

LibPressio sits **above** the I/O library layer — it is an application-level abstraction, not an HDF5 filter plugin. It does not directly integrate into the HDF5 filter pipeline. HDF5 integration requires the `+hdf5+json` build variant, which allows LibPressio to serialize compressor configurations to/from HDF5 metadata. A native HDF5 filter variant exists for some backends (e.g., `H5Z-ZFP`) but is separate from LibPressio.

For NetCDF use cases, the DOE/Unidata approach embeds compression directly as HDF5 filters within the NetCDF-C library (via the NetCDF Expansion Pack), which provides full Fortran 90 compatibility and legacy code transparency that LibPressio's application-layer model cannot deliver to standard NetCDF users.

## Metrics Plugins (Selected)

| Plugin | Measures |
|--------|---------|
| `size` | Compressed and uncompressed byte sizes, compression ratio |
| `time` | Compression and decompression wall-clock time |
| `error_stat` | Min/max/mean absolute and relative error, PSNR, SSIM, NRMSE |
| `composite` | Combines multiple metrics modules |
| `kl_divergence` | KL divergence between original and decompressed value distributions |
| `spatial_error` | Spatial distribution of error |

## Related Repositories

* [`pressio-tools`](https://github.com/robertu94/pressio-tools) — CLI tools for LibPressio
* [`libpressio-tutorial` (FTHPC)](https://github.com/FTHPC/libpressio_tutorial) — canonical tutorial with 8 exercises in C, C++, Python; covers basics, comparing compressors, external metrics, auto-tuning, writing plugins, GPU compression
* [`libpressio-interesting-scripts`](https://github.com/robertu94/libpressio-interesting-scripts) — higher-level use-case examples
* [`libpressio-predict`](https://github.com/robertu94/libpressio-predict) — high-fidelity proxy models for compression parameter prediction
* [`libpressio-r`](https://github.com/robertu94/libpressio-r) — R bindings
* [`LibPressio.jl`](https://github.com/robertu94/LibPressio.jl) — Julia bindings
* [`libpressio-rs`](https://github.com/robertu94/libpressio-rs) — Rust bindings

## References

Underwood, R., Di, S., Calhoun, J. C., & Cappello, F. (2021). LibPressio: A Generic Abstraction Layer for Rationalized Scientific Data Compression. *IEEE Transactions on Parallel and Distributed Systems*, 32(11), 2626–2640. https://doi.org/10.1109/TPDS.2021.3054507 (Open access: https://par.nsf.gov/servlets/purl/10319848)

Underwood, R., Malvoso, V., Calhoun, J. C., Di, S., & Cappello, F. (2021). Productive and Performant Generic Lossy Data Compression with LibPressio. *2021 7th International Workshop on Data Analysis and Reduction for Big Scientific Data (DRBSD-7)*, pp. 1–10. https://doi.org/10.1109/DRBSD754563.2021.00005

Underwood, R. (2026). *LibPressio* (Version 0.99.0) [Computer software]. GitHub. https://github.com/robertu94/libpressio

FTHPC Tutorial: https://github.com/FTHPC/libpressio_tutorial

Documentation: https://robertu94.github.io/libpressio/
