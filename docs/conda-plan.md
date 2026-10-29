# Getting nep-compress into conda-forge

The source of truth for the recipe is `conda/recipe/recipe.yaml` and
`conda/recipe/build.sh` in this repository; it builds the checkout
directly. `conda/make_staged_recipe.py` generates the staged-recipes
copy (release tarball source and version), so the two cannot drift.
`.github/workflows/conda.yml` builds and tests the recipe and lints
the generated staged copy on every push.

## Prerequisites

- The GitHub accounts `edhartnett` and `captainkirk99` (both are
  listed as recipe maintainers; conda-forge requires each named
  maintainer to confirm in the PR).
- `rattler-build` and `conda-smithy` installed locally.
- A green `conda` CI run on nep-compress `main`; note its URL for the
  PR.

## 1. Build and lint locally

```sh
conda create -y -p /fast/ed/tmp-conda/env -c conda-forge rattler-build conda-smithy
curl -sSfL -o /fast/ed/tmp-conda/conda_build_config.yaml \
  https://raw.githubusercontent.com/conda-forge/conda-forge-pinning-feedstock/main/recipe/conda_build_config.yaml
/fast/ed/tmp-conda/env/bin/rattler-build build --recipe conda/recipe/recipe.yaml \
  -m /fast/ed/tmp-conda/conda_build_config.yaml \
  --output-dir /fast/ed/tmp-conda/out
```

A clean result: the build log's ctest summary is 100% passed
(including `perf_compare_filters`, `tst_lz4_filters` and
`ftst_nep_bzip2`), the package tests pass, and the `.conda` contains
`lib/hdf5/plugin/libh5lz4_nepcompress.so` and no `lib/plugin/`.

## 2. Generate the staged recipe

```sh
python3 conda/make_staged_recipe.py 1.1.0 \
  1f55bf23d6fb6552c14d868416f71c194843f5d7c011d7571fedfd64a2a45e1e \
  /fast/ed/tmp-conda/staged
rattler-build build --recipe /fast/ed/tmp-conda/staged/recipes/nep-compress/recipe.yaml \
  -m /fast/ed/tmp-conda/conda_build_config.yaml \
  --output-dir /fast/ed/tmp-conda/out
conda-smithy recipe-lint --conda-forge /fast/ed/tmp-conda/staged/recipes/nep-compress
```

The build must pass and `recipe-lint` must report no errors.

## 3. Fork, branch, commit, push

As `captainkirk99`:

```sh
gh repo fork conda-forge/staged-recipes --clone=false
git clone https://github.com/captainkirk99/staged-recipes /fast/ed/staged-recipes
git -C /fast/ed/staged-recipes checkout -b nep-compress main
mkdir -p /fast/ed/staged-recipes/recipes/nep-compress
cp /fast/ed/tmp-conda/staged/recipes/nep-compress/{recipe.yaml,build.sh} \
  /fast/ed/staged-recipes/recipes/nep-compress/
cd /fast/ed/staged-recipes
git add recipes/nep-compress
git commit -m "Add nep-compress"
git push -u origin nep-compress
```

## 4. Open the PR

```sh
gh pr create --repo conda-forge/staged-recipes --base main \
  --head captainkirk99:nep-compress --title "Add nep-compress"
```

Fill in the PR template checklist. The body:

- Names both maintainers and asks `@edhartnett` to comment that he is
  willing to maintain the feedstock (conda-forge requires every named
  maintainer to confirm in the PR).
- Explains that the LZ4 plugin is installed in HDF5's default plugin
  directory as `lib/hdf5/plugin/libh5lz4_nepcompress.so`, and that the
  `run_constraints` on `hdf5-external-filter-plugins-lz4` prevents two
  32004 providers in one environment.
- Links a green `conda` CI run.

## 5. Review and feedstock creation

- Reviewers comment on the PR; if it stalls, comment
  `@conda-forge-admin, please ping team`.
- After merge, `conda-forge/nep-compress-feedstock` is created with
  both maintainers.
- From then on, recipe changes go to the feedstock first and are
  mirrored back to `conda/recipe/` so the checkout recipe stays in
  sync.
- Remove the `Lint the staged-recipes copy` step and the micromamba
  step from `.github/workflows/conda.yml` once the feedstock exists.
  Keep `Fetch conda-forge pinnings`, because the build step uses it.

## 6. New releases

The conda-forge autotick bot opens the version PR on the feedstock
after each nep-compress tag. Check that `conda/recipe/build.sh` still
applies to the new release; for 1.2.0 nothing else changes (the
minimum `libnetcdf >=4.10.1` is already in the host requirements).

## 7. Adding macOS

Add `osx-64`/`osx-arm64` in the feedstock (`conda-forge.yml`, and drop
the `skip: not linux` selector) once nep-compress builds on macOS.
Windows is not planned; nep-compress is Unix-only.

## Status

| Step | Date | Link |
| --- | --- | --- |
| Recipe in `conda/recipe/` | 2026-10-29 | `conda/recipe/` |
| First green `conda` CI run | pending | |
| staged-recipes PR opened | pending | |
| staged-recipes PR merged | pending | |
| Feedstock created | pending | |
| 1.2.0 published on conda-forge | pending | |

Author: Edward Hartnett, Intelligent Data Design, Inc.
