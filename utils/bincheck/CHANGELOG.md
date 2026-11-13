# Sega Dreamcast Binary Checker (bincheck): Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-10-27
### Added
- Initial release: a C rewrite of the original Binary Checker
  (`bincheck.dll`, 2005).
- Detection of scrambled/unscrambled raw `.BIN` files based on the content
  of the file (entropy variance before and after unscrambling), instead of
  a fixed list of toolchain signatures.
- Real scramble/unscramble algorithm, ported from Marcus Comstedt's
  `scramble` (`utils/scramble`).
- Warnings when the input is an ELF file or does not look like SH-4 code.
- Command-line options: block size (`-b`), decision threshold (`-m`),
  verbose mode (`-v`) and `--version`.
