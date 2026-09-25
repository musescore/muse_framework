# 00105 Uploading native debug files to Sentry

Date: 2026-09-25  
Tags: crashdumps, sentry, ci, symbols   
Maintainers: Elnur Ismailzada   

## Status: Accepted

## Context

Crashes are captured by Crashpad and sent to Sentry as minidumps. A minidump on its own is a list of addresses. To turn it into a readable stack, Sentry needs two kinds of information about that exact build: debug info (functions, files, lines) and unwind info (walking the stack).   

Until now we gave Sentry Breakpad symbol files. CI ran the `dump_syms` utility over the built binary (over the PDB on Windows), converted the result into `.sym` files and uploaded those (see `buildscripts/ci/crashdumps`). This had several drawbacks:   

* `dump_syms` is a prebuilt third-party binary that we keep in the repository — one per platform and architecture, a few megabytes each. It has to be rebuilt and replaced by hand whenever a toolchain or a debug format moves on.
* The conversion adds a format in the middle. A `.sym` file that came out without unwind information uploads just as well as a complete one, and we find out only when a report turns out to be unreadable.
* Release builds were configured as `RELEASE`, so the binaries carried almost no debug information. The symbol files gave function names at best, without files and lines.

Sentry reads the native debug formats directly — dSYM, PDB, ELF with DWARF — so the conversion is not needed at all.   

## Decision

We upload the native debug files as the build produces them, and drop Breakpad from the pipeline.   

* Release builds are configured as `RelWithDebInfo` and installed with `install/strip`: the debug information stays in the build tree, the shipped binary is stripped.
* CI uploads, per platform, the debug file together with the shipped binary, because the two halves live in different files:
  * macOS — the DWARF file from the dSYM bundle, and the app binary
  * Windows — the `.pdb`, and the `.exe`
  * Linux — the unstripped binary, which carries both halves
* The upload is done by `buildscripts/ci/tools/sentry/upload_debug_files.cmake`. It takes `sentry-cli` from extdeps, checks the files with `debug-files check`, and fails the step unless the set as a whole provides both the `debug` and the `unwind` features; only then it runs `debug-files upload`.
* On macOS the linker leaves the DWARF in the object files and puts only a debug map into the executable, so `buildscripts/ci/macos/generate_dsym.cmake` collects it into a dSYM with `dsymutil`. It has to run before packaging: `macdeployqt` strips the binary and packaging removes the dSYM bundles from the app.
* Only stable builds upload debug files — they are large, and sending them every night is costly. Nightly builds still report crashes, those reports just stay unsymbolicated.

## Consequences

* The prebuilt `dump_syms` binaries and the Breakpad scripts are no longer needed. They are already removed from the MuseScore repository; `buildscripts/ci/crashdumps` here is still to be dropped.
* What Sentry gets is what the compiler and the linker produced, without a conversion step that can quietly drop information.
* Reports now have files and lines rather than function names alone, because the build actually carries debug information.
* Debug files are matched to a dump by debug id — `LC_UUID` on macOS, the PDB GUID on Windows, the build id on Linux. The id survives stripping, `install_name_tool` and code signing, so the files collected during the build stay valid for what ships.
* A dump can be read locally with the platform's own tools (LLDB on macOS, WinDbg on Windows) instead of `minidump_stackwalk`, taking the debug files for that build from Sentry, **Project Settings → Debug Files**.
* The release build takes longer and needs more disk space on CI: `RelWithDebInfo` generates debug information for the whole project.
* `sentry-cli` becomes an extdeps tool like `crashpad_handler` and the packaging tools, so its version and checksum are pinned by the recipe in `muse_deps` and it is fetched from our own mirror rather than from the vendor's install script.

## Alternatives

Keeping Breakpad and only changing the build type was considered — that alone would have put files and lines into the `.sym` files too. We abandoned it because it keeps both the prebuilt `dump_syms` binaries and the conversion step, and buys nothing: Sentry treats the native files as the primary format and reads them without our help.

## Implementation

`buildscripts/ci/macos/generate_dsym.cmake` and `buildscripts/ci/tools/sentry/upload_debug_files.cmake`, with the details described in `buildscripts/ci/tools/sentry/README.md`.

The CI steps, macOS as the most involved case:

```yaml
    # Must run before Package: macdeployqt strips the binary and package.sh
    # deletes the dSYM bundles
    - name: Generate dSYM
      if: env.DO_UPLOAD_SYMBOLS == 'true'
      run: |
        cmake -DBIN=applebuild/mscore.app/Contents/MacOS/mscore \
              -DDSYM=applebuild/mscore.dSYM \
              -P buildscripts/ci/macos/generate_dsym.cmake

    - name: Upload debug files
      if: env.DO_UPLOAD_SYMBOLS == 'true'
      env:
        SENTRY_URL: https://sentry.musescore.com
        SENTRY_AUTH_TOKEN: ${{ secrets.SENTRY_MUSE_AUTH_TOKEN }}
        SENTRY_ORG: sentry
      run: |
        cmake -DFILES="applebuild/mscore.dSYM/Contents/Resources/DWARF/mscore;applebuild/mscore.app/Contents/MacOS/mscore" \
              -P buildscripts/ci/tools/sentry/upload_debug_files.cmake
```
