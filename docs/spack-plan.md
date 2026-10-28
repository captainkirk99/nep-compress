# Getting nep-compress into spack-packages

The recipe's source of truth is
`spack/repos/spack_repo/nepcompress/packages/nep_compress/package.py`
in this repository. The upstream copy in `spack/spack-packages` must
stay byte-identical to it, and `.github/workflows/spack.yml` tests it
on every push.

## Prerequisites

- The fork `Intelligent-Data-Design-Inc/spack-packages` checked out at
  `~/spack-packages`, with an `upstream` remote pointing at
  `spack/spack-packages`.
- Spack at `~/spack`.
- The GitHub maintainer handle `edhartnett`.
- A green `spack` CI run on nep-compress `main`; note its URL for the
  PR.

## 1. Check the recipe locally

```sh
~/spack/bin/spack repo add spack/repos/spack_repo/nepcompress
cd ~/spack-packages-nep-compress && ~/spack/bin/spack style \
  repos/spack_repo/builtin/packages/nep_compress/package.py
cd -
~/spack/bin/spack audit packages nep-compress
~/spack/bin/spack spec nep-compress@main
~/spack/bin/spack spec nep-compress@main~bzip2~fortran ^netcdf-c~mpi
~/spack/bin/spack repo remove nepcompress
```

A clean result: `style` and `audit` report nothing, and both `spec`
calls concretize with `netcdf-c@4.10.0` and `hdf5+hl`. `spack spec
nep-compress~lz4~bzip2` must fail with "Enable at least one
compression filter".

## 2. Create the branch

Work in a worktree so the main `~/spack-packages` checkout is
untouched:

```sh
git -C ~/spack-packages fetch upstream
git -C ~/spack-packages worktree add ~/spack-packages-nep-compress \
  -b nep-compress-new-package upstream/develop
mkdir -p ~/spack-packages-nep-compress/repos/spack_repo/builtin/packages/nep_compress
cp spack/repos/spack_repo/nepcompress/packages/nep_compress/package.py \
  ~/spack-packages-nep-compress/repos/spack_repo/builtin/packages/nep_compress/package.py
diff spack/repos/spack_repo/nepcompress/packages/nep_compress/package.py \
  ~/spack-packages-nep-compress/repos/spack_repo/builtin/packages/nep_compress/package.py
```

The `diff` must be empty.

## 3. Commit and push

Make one commit that adds only
`repos/spack_repo/builtin/packages/nep_compress/package.py`, with the
message `nep-compress: new package`. Push the branch to `origin`
(`Intelligent-Data-Design-Inc/spack-packages`):

```sh
cd ~/spack-packages-nep-compress
git add repos/spack_repo/builtin/packages/nep_compress/package.py
git commit -m "nep-compress: new package"
git push -u origin nep-compress-new-package
```

## 4. Open the pull request

```sh
gh pr create --repo spack/spack-packages --base develop \
  --head Intelligent-Data-Design-Inc:nep-compress-new-package \
  --title "nep-compress: new package"
```

Follow the repo's PR template if one exists. The body states:

- What the package is, its variants (`lz4`, `bzip2`, `fortran`), and
  that it was tested by `spack install --test=root` and `spack test
  run` in nep-compress CI on `netcdf-c@4.10.0+mpi ^mpich`, with a link
  to the green `spack` CI run.
- That `1.1.0` needs `netcdf-c@4.10.1:` and resolves to
  `netcdf-c@main` until upstream PR #6569 (netcdf-c 4.10.1) merges,
  while `main` (and the coming 1.2.0) works with 4.10.0.

## 5. Review and merge

Upstream CI on the PR runs style and audit checks, and possibly a
GitLab build pipeline started by spackbot. Check these against the
first real PR and correct this text if it differs.

To make review changes: edit the overlay recipe
`spack/repos/spack_repo/nepcompress/packages/nep_compress/package.py`
in nep-compress first, copy it to the worktree, amend or add commits,
and push again. Ask spackbot for a rebuild with `@spackbot run
pipeline` if needed. The PR merges once a maintainer approves it.

## 6. After the merge

- Add the upstream PR link to the README "Install with Spack" section
  and change it to say `spack install nep-compress` works without the
  overlay.
- Remove the worktree and the branch:

  ```sh
  git -C ~/spack-packages worktree remove ~/spack-packages-nep-compress
  git -C ~/spack-packages branch -d nep-compress-new-package
  ```

## 7. Adding a new release (for example 1.2.0)

- After the tag is pushed, compute the sha256:

  ```sh
  curl -sL https://github.com/captainkirk99/nep-compress/archive/refs/tags/vX.Y.Z.tar.gz | sha256sum
  ```

- Add the `version(...)` line to the overlay recipe and land it in
  nep-compress.
- Copy the recipe to a new worktree branch `nep-compress-X.Y.Z` from
  `upstream/develop` (same steps as sections 2-4).
- Open a PR titled `nep-compress: add vX.Y.Z`.
- 1.2.0 needs nothing beyond the version line, because
  `depends_on("netcdf-c@4.10.0:", when="@1.2:")` already covers it.

## 8. When netcdf-c 4.10.1 reaches spack-packages

- Once upstream PR #6569 merges, bump `packages_ref` in
  `.github/workflows/spack.yml` to a spack-packages tag that includes
  it.
- Change the `skipping perf_compare_filters` zgrep in that workflow to
  a check that `perf_compare_filters` ran.
- No recipe change is needed.

## Status

| Step | Date | Link |
| --- | --- | --- |
| Recipe in the overlay | 2026-10-28 | `spack/repos/spack_repo/nepcompress` |
| First green `spack` CI run | pending | |
| Upstream PR opened | pending | |
| Upstream PR merged | pending | |
| 1.2.0 added to the recipe | pending | |

Author: Edward Hartnett, Intelligent Data Design, Inc.
