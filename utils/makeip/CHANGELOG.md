# IP creator (makeip): Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.1] - 2026-01-20
### Added
- `" MIL CD"` is now a valid input for **Device Info**.

## [2.0.0] - 2020-06-24
### Added
- All the fields may now be filled directly from the command-line. Use the `-u`
  switch to get the list of all the available fields.
- All the fields are now validated when necessary (previously, only the **Area
  Symbols** field was validated).
- **PNG** images are now supported for logo insertion (extracted from `logotools`).
  **IP creator** may be used to convert a PNG image into a MR image by using
  the `-l` and `-s` switches together.

### Changed
- Complete redesign/refactoring of the whole code. Splitting code into many
  small source files instead of a single `makeip.c` file.
- Command-line options are now better handled.  
- The possibility to use an external `IP.TMPL` was restored by using the `-t`
  switch.
- The embedded `IP.TMPL` file is now the **LiENUS** version instead of the
  original file (which was using the copyrighted `AIP` library). 
- The `ip.txt` template file was changed a bit: the **Hardware ID** and 
  **Maker ID** fields aren't necessary anymore. The **Device Info** field
  doesn't need to contains the fake `0000` **CRC**, only the `CD-ROM1/1` value
  is needed now.  
  
## [1.5.0] - 2019-12-10
### Added
- Support of command-line arguments to edit the fields of the `IP.BIN`.
- Support of **MR image** insertion into the `IP.BIN`.

### Changed
- The `IP.TMPL` file is now embedded, it no longer depends of the external 
  `IP.TMPL` file.
- Program is now licensed under the **BSD 2-Clause License** (previously it
  wasn't licensed).

## [1.0.0] - 2000-09-05
### Added
- Initial (unversioned) release.
