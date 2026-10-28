# Sega Dreamcast Binary Checker (bincheck)

`bincheck` is a simple command-line tool that checks whether a Sega Dreamcast homebrew `.BIN` file is **scrambled** or **unscrambled**.

## What does "scrambled" mean?

A `.BIN` file is a compiled Dreamcast program (often found in homebrew releases under "plain files"). For homebrew programs, these files come in two formats:

- **Scrambled**: Needed when running a program directly from a CD-ROM via the Dreamcast bootloader (usually `IP.BIN`). These files are typically named `1ST_READ.BIN`.
- **Unscrambled**: Used in all other situations, such as when starting a program with a loader (like `dcload`) or another menu program.

This distinction only applies to homebrew programs built with tools like KallistiOS.

> [!NOTE]
> `bincheck` is designed for raw `.BIN` files. It will warn you if you provide an ELF executable or a non-Dreamcast file, because checking those files won't produce a useful result.

This project is a modern C rewrite of the original 2005 `bincheck.dll` ([which was written in Delphi](https://sizious.com/download/dreamcast/#binary-checker)). While the original version relied on a hardcoded list of compiler signatures, this version analyzes the file's contents directly, so it does not rely on a list of known toolchains.

## How to use it

```text
Usage: bincheck [options] <file.bin>

Options:
  -b <bytes>     Block size for the entropy variance (default: 256)
  -m <margin>    Decision threshold, relative increase (default: 0.15)
  -i             Invert the exit code (scrambled: 1, unscrambled: 0)
  -v             Verbose mode (show details)
  -h             Show this help
  --version      Show the version

Exit status:
  0 if the file is scrambled, 1 if it is unscrambled (swapped with -i),
  2 if an error occurred.
```

### Examples

Check a file:
```text
$ ./bincheck 1ST_READ.BIN
1ST_READ.BIN: Scrambled
```

Check a file with detailed info:
```text
$ ./bincheck -v 1ST_READ.BIN
File                         : 1ST_READ.BIN
Entropy variance, as-is      : 1.1451
Entropy variance, unscrambled: 2.6444
Threshold (relative increase): +15%
SH-4 code score              : 109.63 (minimum: 2.00)
---
1ST_READ.BIN: Scrambled
```

Handling non-Dreamcast files:
```text
$ ./bincheck notepad.exe
Warning: "notepad.exe" does not look like SH-4 code (Dreamcast program) so this result is probably meaningless
notepad.exe: Unscrambled
```

Warnings are sent to `stderr`, so any automated scripts reading the result from `stdout` continue to work smoothly.

### Use in scripts

The exit code gives the verdict, so scripts don't need to parse the output:

| Exit code | Default | With `-i` |
|-----------|---------|-----------|
| `0` | Scrambled | Unscrambled |
| `1` | Unscrambled | Scrambled |
| `2` | Error (file not found, invalid option...) | Error |

For example, to scramble a binary only if it isn't scrambled yet:
```bash
bincheck program.bin > /dev/null
case $? in
    0) cp program.bin 1ST_READ.BIN ;;          # already scrambled
    1) scramble program.bin 1ST_READ.BIN ;;    # unscrambled
    *) echo "bincheck failed" >&2; exit 1 ;;
esac
```

Use `-i` when the unscrambled case is the "success" you test for, since an
`if` only succeeds on exit code `0` (an error, `2`, counts as a failure):
```bash
if bincheck -i program.bin > /dev/null; then
    echo "program.bin is unscrambled"
fi
```

Or in a Windows batch file:
```bat
bincheck 1ST_READ.BIN > NUL
if errorlevel 2 goto error
if errorlevel 1 echo Unscrambled
```

The warnings shown for ELF or non-SH-4 files do not change the exit code.

> [!TIP]
> `bincheck` only detects whether a file is scrambled or not. To scramble or unscramble a binary, use the `scramble` utility included with KallistiOS (`utils/scramble`).

## How it works (in simple terms)

In short: `bincheck` creates an unscrambled copy of the file in memory, and then checks which version looks like a real, organized program.

Scrambling does not encrypt or change individual bytes. Instead, it chops the file into 32-byte chunks and shuffles them around. A normal program has organized, distinct sections (code, data tables, and blocks of empty zero bytes). Scrambling mixes all these sections together into an even blend.

`bincheck` measures how varied the different sections of the file are (using a statistical concept called **entropy variance** across 256-byte blocks).
- If unscrambling the file makes its internal sections look **much more distinct and organized** (an entropy variance increase of over 15%), the file was scrambled.
- If the file becomes less organized or stays roughly the same, it was already unscrambled.

`bincheck` also performs two quick sanity checks to see whether the file looks like an actual Dreamcast program: it checks whether the file is an ELF binary, and it looks for standard SH-4 CPU instructions that GCC produces. If either check fails, it prints a warning, but the verdict is still given.

For a full, in-depth explanation and test numbers, see [HOW_IT_WORKS.md](HOW_IT_WORKS.md).

## Accuracy

The default decision threshold (+15% increase) was tested against a wide range of real-world binaries:
- 152 KallistiOS examples tested in both scrambled and unscrambled form.
- Real-world utilities (`dcload-ip` and `dcload-serial`).
- Classic homebrew releases from 2004 to 2011 (`snes4all`, `SuperFamicast`, `dcplaya`).
- A NetBSD 10.1 Dreamcast kernel built with the NetBSD toolchain.

Every test was 100% accurate. The lowest variance increase seen on a scrambled file was +31% (`dcload-serial`), and every unscrambled file experienced a decrease of at least −21% when unscrambled again.

You can tune the threshold (`-m`) or the block size (`-b`) if you ever encounter an unusual binary.

## How to build

To compile the project, run:

```bash
make
```

This compiles all source files in `src/` and places the `bincheck` executable in this directory (`utils/bincheck`). To clean up build files, run `make clean`.

The version number is set by `VERSION` in `src/Makefile` and shown by `bincheck --version` (and in `bincheck -h`). See [CHANGELOG.md](CHANGELOG.md) for the changes in each version.

`bincheck` only requires standard C library functions and the math library (`libm`). It builds cleanly with GCC on Linux and on Windows (via MinGW / DreamSDK), and should build as-is on macOS.

## Project structure

```text
bincheck/
├── src/
│   ├── scramble.h/.c        # Scramble/unscramble logic
│   ├── check.h/.c           # Entropy variance analysis and checks
│   ├── main.c               # CLI interface and output formatting
│   └── Makefile             # Builds bincheck (sets VERSION)
├── CHANGELOG.md             # Version history
├── HOW_IT_WORKS.md          # Technical deep dive into the detection algorithm
├── LICENSE                  # BSD 2-Clause License
├── Makefile                 # Top-level makefile
└── README.md
```

## License

- `bincheck` is open source under the **BSD 2-Clause License** (see [LICENSE](LICENSE)).
- `src/scramble.c` is based on Marcus Comstedt's `scramble.c` (from KallistiOS `utils/scramble`), licensed under the BSD-compatible KOS License.
