# Optional compression benchmarks

These LZ4/BZIP2 programs, CSV snapshots, plot scripts and caption
metadata were carried together from NEP's `examples/performance/`.
They use a 500 x 180 x 360 float dataset and are **not** CI tests.
The CSV files are historical measurements, not predictions for another
machine. General NetCDF compression benchmarks remain in NEP.

Configure with `-DNEPCOMPRESS_BUILD_BENCHMARKS=ON`, enable the relevant
filter(s), then build. Before running, set `HDF5_PLUGIN_PATH` to
`build/plugin` for LZ4 and to your external BZIP2 directory for BZIP2.
Run from a writable directory; the benchmarks create and remove their
own temporary NetCDF file. Keep new measurements in `build` so the
historical CSVs are not overwritten.

```sh
cd build
./examples/performance/benchmark_lz4 > run_lz4.csv
./examples/performance/benchmark_bzip2 > run_bzip2.csv
cd ../examples/performance
python3 plot_lz4.py
python3 plot_bzip2.py
```

The plotters read the tracked historical CSVs in this directory;
copy new CSVs into a scratch directory alongside the plotters if
plotting new measurements. Plotting requires `matplotlib`.
