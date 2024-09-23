# font2txf: Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
  
## [1.12.0] - 2024-08-xx

### Added

- Preview option (`-p`), used for displaying the generated TXF file at the
  end of the process. This requires [Freeglut](https://freeglut.sourceforge.net/),
  but it can be disabled if needed at compile time.
- Verbose option (`-v`), that display the original output from `ttf2txf`.

### Changed

- Complete redesign/refactoring of the whole code. Splitting code into many
  small source files instead of a single `ttf2txf.cpp` file.
- Command-line options were updated a bit (exclusion between `-c` and `-f`,
  `-q` is really quiet now, `-h` stands for help now but can be mapped to 
  height as originally if used with `-w`, etc.).
- Program is now licensed under the **GNU GPL 2 License** like the
  [Celestia](https://celestiaproject.space/) project (previously this tool
  wasn't clearly licensed).

### Fixed

- Some memory leaks while building the TXF font when an error occured.

## [1.11.0] - 2001-10-20

### Added
- Initial release used for the [Celestia](https://celestiaproject.space/) project.
