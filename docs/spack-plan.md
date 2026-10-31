# Getting nep-compress into spack-packages

The recipe's source of truth is
`spack/repos/spack_repo/nepcompress/packages/nep_compress/package.py`
in this repository. The upstream copy in `spack/spack-packages` must
stay byte-identical to it, and `.github/workflows/spack.yml` tests it
on every push.

## Prerequisites

- The clone of the fork `captainkirk99/spack-packages` at
  `/fast/ed/spack-packages`, with `origin` pointing at the fork
  (`git@github.com:captainkirk99/spack-packages.git`). It also needs an
  `upstream` remote pointing at `spack/spack-packages`; add it once:

  ```sh
  git -C /fast/ed/spack-packages remote add upstream \
    https://github.com/spack/spack-packages.git
  ```
- Spack at `~/spack`. Its `builtin` repo is whatever `spack repo list`
  shows (currently `~/spack-packages`, a separate checkout), so local
  concretization uses that checkout's branch, not `upstream/develop`.
  CI pins Spack `v1.2.2` and
  spack-packages `v2026.10.0`; a local Spack or `builtin` branch that is
  much older or newer can concretize differently.
- The GitHub maintainer handle `edhartnett`.
- A green `spack` CI run on nep-compress `main`; note its URL for the
  PR.

## 1. Check the recipe locally

```sh
~/spack/bin/spack repo add spack/repos/spack_repo/nepcompress
~/spack/bin/spack audit packages nep-compress
~/spack/bin/spack spec nep-compress@main
~/spack/bin/spack spec nep-compress@main~bzip2~fortran ^netcdf-c~mpi
~/spack/bin/spack repo remove nepcompress
```

A clean result: `audit` reports nothing, and both `spec` calls
concretize with `netcdf-c@4.10.0` and `hdf5+hl`. `spack spec
nep-compress~lz4~bzip2` must fail with "Enable at least one
compression filter". If Spack's `builtin` repo is not on a recent
`develop`, treat the concretized versions as indicative only.

## 2. Create the branch

Branch from a fresh `upstream/develop` in the fork clone:

```sh
git -C /fast/ed/spack-packages fetch upstream
git -C /fast/ed/spack-packages switch \
  -c nep-compress-new-package --no-track upstream/develop
mkdir -p /fast/ed/spack-packages/repos/spack_repo/builtin/packages/nep_compress
cp spack/repos/spack_repo/nepcompress/packages/nep_compress/package.py \
  /fast/ed/spack-packages/repos/spack_repo/builtin/packages/nep_compress/package.py
diff spack/repos/spack_repo/nepcompress/packages/nep_compress/package.py \
  /fast/ed/spack-packages/repos/spack_repo/builtin/packages/nep_compress/package.py
```

The `diff` must be empty.

Then run upstream's style checks. spack-packages does not use `spack
style`; its CI runs `.ci/style_check.sh` (`ruff format` and `ruff
check`, configured by its `pyproject.toml`) and `vermin`, at the
versions pinned in
`.github/workflows/requirements/style/requirements.txt`. `spack style`
reports false F405 star-import errors on package files, so do not use
it. Install the pinned tools in a throwaway venv:

```sh
python3 -m venv /tmp/sp-style-venv
/tmp/sp-style-venv/bin/pip install ruff==0.15.11 vermin==1.8.0
cd /fast/ed/spack-packages
f=repos/spack_repo/builtin/packages/nep_compress/package.py
/tmp/sp-style-venv/bin/ruff format --diff "$f"
/tmp/sp-style-venv/bin/ruff check --no-fix "$f"
/tmp/sp-style-venv/bin/vermin --backport importlib --backport argparse \
  --violations --backport typing --eval-annotations -t=3.6- -vvv "$f"
cd -
```

All three must exit 0 (`1 file already formatted`, `All checks
passed!`, no vermin violations). Check the pins in
`requirements.txt` first; they change over time.

## 3. Commit and push

Make one commit that adds only
`repos/spack_repo/builtin/packages/nep_compress/package.py`, with the
message `nep-compress: new package`:

```sh
cd /fast/ed/spack-packages
git add repos/spack_repo/builtin/packages/nep_compress/package.py
git commit -m "nep-compress: new package"
```

Before pushing, rebase onto a fresh `upstream/develop` (the commit may
have been made days earlier), then push the branch to `origin`
(`captainkirk99/spack-packages`):

```sh
git fetch upstream
git rebase upstream/develop
git push -u origin nep-compress-new-package
```

## 4. Open the pull request

```sh
gh pr create --repo spack/spack-packages --base develop \
  --head captainkirk99:nep-compress-new-package \
  --title "nep-compress: new package"
```

The repo's PR template (`.github/pull_request_template.md`) is only an
HTML comment listing spackbot commands; there are no sections to fill
in. The body states:

- What the package is and its variants (`lz4`, `bzip2`, `fortran`).
- That `@main` was tested by `spack install --test=root` and `spack
  test run` in nep-compress CI on `netcdf-c@4.10.0+mpi ^mpich`, with a
  link to the green `spack` CI run. CI builds the checkout through
  `spack develop`, so it does not build the `1.1.0` tarball; the
  `1.1.0` sha256 was checked by hand against the release tarball.
- That `1.1.0` needs `netcdf-c@4.10.1:` and resolves to
  `netcdf-c@main` until upstream PR #6569 (netcdf-c 4.10.1) merges,
  while `main` (and the coming 1.2.0) works with 4.10.0.

## 5. Review and merge

Upstream GitHub Actions on the PR run `prechecks.yml` (vermin, style
through `.ci/style_check.sh`, audit, and checksum verification),
`audit.yaml` and `ci.yaml`. Spackbot
may also start a GitLab build pipeline. Check this list against the
first real PR and correct this text if it differs.

To make review changes: edit the overlay recipe
`spack/repos/spack_repo/nepcompress/packages/nep_compress/package.py`
in nep-compress first, copy it to `/fast/ed/spack-packages`, amend or add commits,
and push again. `@spackbot fix style` pushes a style-fix commit;
`@spackbot re-run pipeline` reruns the GitLab pipeline (write access
required). After a maintainer approves, the PR goes through the merge
queue (`ci.yaml` runs again on `merge_group`) before it merges.

## 6. After the merge

- Add the upstream PR link to the README "Install with Spack" section
  and change it to say `spack install nep-compress` works without the
  overlay.
- Delete the branch locally and on the fork:

  ```sh
  git -C /fast/ed/spack-packages switch develop
  git -C /fast/ed/spack-packages branch -D nep-compress-new-package
  git -C /fast/ed/spack-packages push origin --delete nep-compress-new-package
  ```

  `-D` is needed because upstream squash-merges, so the branch is not
  an ancestor of `develop`.

## 7. Adding a new release (for example 1.2.0)

- After the tag is pushed, compute the sha256:

  ```sh
  curl -sL https://github.com/captainkirk99/nep-compress/archive/refs/tags/vX.Y.Z.tar.gz | sha256sum
  ```

- Add the `version(...)` line to the overlay recipe and land it in
  nep-compress.
- Copy the recipe to a new branch `nep-compress-X.Y.Z` in
  `/fast/ed/spack-packages`, created from
  `upstream/develop` (same steps as sections 2-4).
- Open a PR titled `nep-compress: add vX.Y.Z`.
- 1.2.0 needs nothing beyond the version line, because
  `depends_on("netcdf-c@4.10.0:", when="@1.2:")` already covers it and
  JPEG-LS (`NEPCOMPRESS_BUILD_JPEGLS`) is off by default.

## 8. When netcdf-c 4.10.1 reaches spack-packages

- Once upstream PR #6569 merges, bump `packages_ref` in
  `.github/workflows/spack.yml` to a spack-packages tag that includes
  it.
- Change the `skipping perf_compare_filters` grep in that workflow to
  a check that `perf_compare_filters` ran.
- No recipe change is needed.

## Status

| Step | Date | Link |
| --- | --- | --- |
| Recipe in the overlay | 2026-10-28 | `spack/repos/spack_repo/nepcompress` |
| First green `spack` CI run on `main` | 2026-10-29 | https://github.com/captainkirk99/nep-compress/actions/runs/40899834222 |
| Clone of `captainkirk99/spack-packages` | 2026-10-30 | `/fast/ed/spack-packages` |
| `upstream` remote added to the clone | 2026-10-30 | |
| Local checks (section 1) | 2026-10-30 | audit clean; `netcdf-c@4.10.0`, `hdf5@1.14.6+hl` |
| Branch created, recipe copied, style clean | 2026-10-30 | `nep-compress-new-package` on `upstream/develop` `294ff82c1a` |
| Local commit | 2026-10-30 | `e5f35745a1` |
| Branch pushed to fork | 2026-10-30 | https://github.com/captainkirk99/spack-packages/tree/nep-compress-new-package |
| Upstream PR opened | 2026-10-30 | https://github.com/spack/spack-packages/pull/7334 |
| Upstream PR merged | pending | |
| 1.2.0 added to the recipe | pending | |

Author: Edward Hartnett, Intelligent Data Design, Inc.
