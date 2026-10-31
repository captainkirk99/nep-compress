# Getting nep-compress into conda-forge

The source of truth for the recipe is `conda/recipe/recipe.yaml` and
`conda/recipe/build.sh` in this repository; it builds the checkout
directly. `conda/make_staged_recipe.py` generates the staged-recipes
copy (release tarball source and version), so the two cannot drift.
The checkout recipe carries the next development version
(`1.3.0.dev0`); the script substitutes the release version.
`.github/workflows/conda.yml` builds and tests the recipe and lints
the generated staged copy on every push.

## Prerequisites

- The GitHub accounts `captainkirk99` (Ed Hartnett) and `czender`
  (Charlie Zender, zender@uci.edu), both listed as recipe maintainers;
  conda-forge requires each named maintainer to confirm in the PR.
- `rattler-build` and `conda-smithy` in the conda env
  `/fast/ed/tmp-conda/env` (created by the first command in section 1;
  neither is packaged for apt). They are not on `PATH`, so call them as
  `/fast/ed/tmp-conda/env/bin/rattler-build` and
  `/fast/ed/tmp-conda/env/bin/conda-smithy`.
- The fork `captainkirk99/staged-recipes`. It already exists but is
  not kept in sync, so always branch from `upstream/main`, never from
  the fork's `main`.
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
python3 conda/make_staged_recipe.py 1.2.0 \
  de1d83d9fee6d3dddce1798d9811626b576d75a6d75367dc268e88fbe7631349 \
  /fast/ed/tmp-conda/staged
/fast/ed/tmp-conda/env/bin/rattler-build build \
  --recipe /fast/ed/tmp-conda/staged/recipes/nep-compress/recipe.yaml \
  -m /fast/ed/tmp-conda/conda_build_config.yaml \
  --output-dir /fast/ed/tmp-conda/out
/fast/ed/tmp-conda/env/bin/conda-smithy recipe-lint --conda-forge /fast/ed/tmp-conda/staged/recipes/nep-compress
```

The build must pass and `recipe-lint` must report no errors.

## 3. Clone, branch, commit, push

As `captainkirk99`. Clone the fork once and add the `upstream` remote:

```sh
git clone git@github.com:captainkirk99/staged-recipes.git /fast/ed/staged-recipes
git -C /fast/ed/staged-recipes remote add upstream \
  https://github.com/conda-forge/staged-recipes.git
```

Then branch from a fresh `upstream/main` (the fork's own `main` is
thousands of commits behind), copy the generated recipe, commit and
push:

```sh
git -C /fast/ed/staged-recipes fetch upstream
git -C /fast/ed/staged-recipes switch -c nep-compress --no-track upstream/main
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

The PR template is a checklist. Tick every item; all apply and all
hold for this recipe:

- meaningful title; v1 `recipe.yaml` format; `license_file: LICENSE`
  is packaged; source is the official release tarball (`url`, not a
  git repo); build number is 0;
- nothing vendored, and no static libraries shipped (the built
  `.conda` contains no `.a` files);
- every listed maintainer has commented to confirm (see below).

The body:

- Names both maintainers and asks `@czender` to comment that he is
  willing to maintain the feedstock (conda-forge requires every named
  maintainer to confirm in the PR).
- Explains that the LZ4 plugin is installed in HDF5's default plugin
  directory as `lib/hdf5/plugin/libh5lz4_nepcompress.so`, and that the
  package can coexist with `hdf5-external-filter-plugins-lz4`: both
  provide filter 32004 with the same encoding, and their file names
  differ, so HDF5 loads whichever it finds first.
- Links a green `conda` CI run.

## 5. Review and feedstock creation

- When the staged-recipes CI is green and `@czender` has
  confirmed, ask for review. The template asks for a comment
  `@conda-forge/help-c-cpp, ready for review!`, but first-time
  contributors cannot ping teams; in that case comment
  `@conda-forge-admin, please ping team` instead. If review stalls,
  ask on the conda-forge Zulip.
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
applies to the new release; for 1.3.0 nothing else changes unless a
dependency or CMake option changed (the minimum `libnetcdf >=4.10.1`
is already in the host requirements).

## 7. Adding macOS

Add `osx-64`/`osx-arm64` in the feedstock (`conda-forge.yml`, and drop
the `skip: not linux` selector) once nep-compress builds on macOS.
Windows is not planned; nep-compress is Unix-only.

## Status

| Step | Date | Link |
| --- | --- | --- |
| Recipe in `conda/recipe/` | 2026-10-29 | `conda/recipe/` |
| First green `conda` CI run on `main` | 2026-10-29 | https://github.com/captainkirk99/nep-compress/actions/runs/40899834364 |
| Local build of the checkout recipe (section 1) | 2026-10-30 | ctest 14/14, package tests pass; `libnetcdf 4.10.1`, `hdf5 1.14.6` |
| Staged 1.1.0 recipe built and linted (section 2) | 2026-10-30 | ctest 14/14, package tests pass, `recipe-lint`: "in fine form" |
| Clone of `captainkirk99/staged-recipes` with `upstream` remote | 2026-10-30 | `/fast/ed/staged-recipes` |
| Branch `nep-compress` committed and pushed to fork | 2026-10-30 | `42eb042105` on `upstream/main` `65da889783` |
| staged-recipes PR opened | 2026-10-30 | https://github.com/conda-forge/staged-recipes/pull/35426 |
| Staged 1.2.0 recipe built and linted | 2026-10-31 | ctest 15/15, package tests pass, `recipe-lint`: "in fine form" |
| PR #35426 updated to 1.2.0 | 2026-10-31 | `d54a9cb0a8 nep-compress: update to 1.2.0` |
| staged-recipes PR merged | pending | |
| Feedstock created | pending | |
| 1.2.0 published on conda-forge | pending | |

Author: Edward Hartnett, Intelligent Data Design, Inc.
