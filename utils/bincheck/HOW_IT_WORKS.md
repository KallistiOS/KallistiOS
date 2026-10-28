# How bincheck works

This guide explains how `bincheck` determines whether a Sega Dreamcast `.BIN` program is **scrambled** (the format needed when booting from a CD, usually `1ST_READ.BIN`) or **unscrambled** (a standard, flat binary image).

For build and usage instructions, see [README.md](README.md).

> **In a nutshell:** `bincheck` creates an unscrambled copy of the file in memory and compares both versions. The version with the structured characteristics of a real program tells us which state the file was originally in.

---

## Overview

The tool is organized into three main source files:

| File | Purpose |
|------|---------|
| [`src/main.c`](src/main.c) | Command-line interface: handles options and prints results or warnings. |
| [`src/scramble.c`](src/scramble.c) | The scrambling and unscrambling algorithms (adapted from the KallistiOS `scramble` tool). |
| [`src/check.c`](src/check.c) | The core analysis and detection logic (`bincheck_analyze_buffer()`). |

---

## Step 1: Unscrambling

The unscrambling code in [`src/scramble.c`](src/scramble.c) is ported directly from Marcus Comstedt's `scramble` utility in KallistiOS (`utils/scramble`).

Dreamcast scrambling is **not encryption**. It does not alter byte values or password-protect anything. Instead:
1. The file is split into **32-byte slices**.
2. These slices are shuffled around according to a deterministic pseudorandom pattern based on the total file size.

Because the shuffle depends purely on the file size, it is completely reversible. The function `dc_descramble_buffer()` performs this exact mathematical reversal. When tested on real software like `dcload-ip`, unscrambling its `1st_read.bin` produces the exact byte-for-byte image of `loader.bin`, and scrambling `loader.bin` produces `1st_read.bin` again.

`bincheck` keeps the unscrambled version in memory only for analysis; it does not modify your file or write a new one to disk.

---

## Step 2: Detection

The core detection function, `bincheck_analyze_buffer()` in [`src/check.c`](src/check.c), follows these steps:

1. **Always unscramble:** It unscrambles the binary into a separate memory buffer, regardless of whether the input file was scrambled or unscrambled.
2. **Measure entropy variance:** It calculates the **entropy variance** (explained below) for both the original file and the unscrambled copy.
3. **Compare results:** It checks how much the variance changed:

   ```text
   relative_increase = (variance_unscrambled - variance_as_is) / variance_as_is
   is_scrambled      = relative_increase > 0.15  (+15% margin)
   ```

If unscrambling increases the entropy variance by more than **15%**, the original file was **scrambled**.

Before calculating entropy, `bincheck` also runs sanity checks to see whether the file looks like a Dreamcast program. They only print warnings and never change the verdict (see [Sanity checks](#checking-that-the-file-is-a-dreamcast-program)).

---

## What is entropy variance?

### Shannon entropy (randomness of a block)

Shannon entropy measures how unpredictable or random data is on a scale from **0 to 8 bits per byte**:

| Data type | Typical entropy |
|-----------|-----------------|
| Blocks of zeros (padding, empty space) | 0 (completely predictable) |
| Text, repetitive lookup tables | Low to medium |
| Compiled SH-4 CPU code | High (typically 5 to 7) |
| Compressed files or encrypted data | Very high (close to 8) |

`shannon_entropy()` calculates this value based on how frequently each byte value (0–255) appears in a given block.

### Block entropy variance (variation across the whole file)

Rather than calculating a single entropy number for the whole file, `block_entropy_variance()` looks at how entropy changes across different parts of the file:

1. It divides the file into consecutive **256-byte blocks** (configurable with `-b`).
2. It calculates the Shannon entropy for every 256-byte block.
3. It computes the **variance** of these entropy values—measuring how much the blocks differ from each other.

### Why this detects scrambling

Because scrambling only shuffles existing bytes, the total file has the exact same byte counts before and after. Measuring the entropy of the entire file would produce identical numbers.

However, the **local structure** changes drastically:

- **An unscrambled program is organized into distinct zones:** It has dense machine code sections (high entropy), data tables (medium entropy), and blocks of zero padding (near 0 entropy). Because different blocks look very different, **the variance between blocks is high**.
- **A scrambled program is thoroughly blended:** The 32-byte slices of code, data, and zero blocks are scattered all over the file. Each 256-byte block ends up containing a random mix of everything, making every block look similar. Because there is little contrast from one block to the next, **the variance between blocks is low**.

This creates a reliable, two-way test:

- **If the file was scrambled:** Unscrambling it restores the distinct sections. The entropy variance **jumps noticeably higher**.
- **If the file was already unscrambled:** Running the unscrambler on it actually shuffles and scrambles the data further. The distinct sections disappear, and the variance **drops or stays low**.

---

## Real-world example

Below is the output of `bincheck -v` run on a real 1.2 MB homebrew program in both formats:
- `test-s.bin`: Scrambled version
- `test-u.bin`: Unscrambled version

```text
$ ./bincheck -v test-s.bin
File                         : test-s.bin
Entropy variance, as-is      : 1.1451
Entropy variance, unscrambled: 2.6444
Threshold (relative increase): +15%
SH-4 code score              : 109.63 (minimum: 2.00)
---
test-s.bin: Scrambled

$ ./bincheck -v test-u.bin
File                         : test-u.bin
Entropy variance, as-is      : 2.6444
Entropy variance, unscrambled: 1.1965
Threshold (relative increase): +15%
SH-4 code score              : 109.63 (minimum: 2.00)
---
test-u.bin: Unscrambled
```

### Breakdown:

| File | Variance as-is | Variance after unscrambling | Change | Verdict |
|------|----------------|-----------------------------|--------|---------|
| `test-s.bin` | 1.1451 | 2.6444 | **+131%** | **Scrambled** |
| `test-u.bin` | 2.6444 | 1.1965 | **−55%** | **Unscrambled** |

Notice that the variance of the scrambled file more than doubles (+131%) once unscrambled, well above the 15% threshold. Conversely, unscrambling the already-unscrambled file cuts its variance in half (−55%).

Also note that the unscrambled variance of `test-s.bin` (2.6444) matches the original as-is variance of `test-u.bin` (2.6444), as expected since unscrambling restores the original file byte for byte (see [Step 1](#step-1-unscrambling)).

### Test results across different programs

| Program | Scrambled file (after unscramble) | Unscrambled file (after unscramble) |
|---------|-----------------------------------|-------------------------------------|
| `dcload-ip` (`1st_read.bin` / `loader.bin`) | +51% | −33% |
| `dcload-serial` (`1st_read.bin` / `loader.bin`) | +31% | −21% |
| KOS example `nehe05` (`1st_read.bin` / `nehe05.bin`) | +134% | −53% |
| `snes4all` beta 1 (KOS from 2011) | +172% | −61% |
| `snes4all` beta 2 (KOS from 2011) | +46% | −25% |
| `snes4all` beta 2 "secure" (KOS from 2011) | +55% | −32% |
| `dcplaya` 2.0 beta 2 (KOS from 2004) | +125% | −56% |
| NetBSD 10.1 kernel (NetBSD toolchain) | +91% | −50% |

In every case, the distinction is clear. Across a broader test set of 152 KallistiOS programs (304 files total), the results remained consistent:
- Scrambled binaries saw variance increases between **+97%** and **+696%** (median: **+204%**).
- Unscrambled binaries saw variance drops between **−47%** and **−89%** (median: **−65%**).

---

## Why not just disassemble the file?

A common question is: *Why not disassemble the file and check if the CPU instructions make sense?*

Earlier iterations tried this with the `dcdis` disassembler, but disassembling proved unreliable for two reasons:

1. **SH-4 instructions are very dense:** In the SH-4 architecture, almost any random 16-bit number happens to map to a technically valid instruction. Even random white noise decodes to roughly 90% "valid" instructions. A scrambled file still looks like valid code to a disassembler.
2. **Ambiguous differences:** On test programs, the percentage of valid instructions slightly increased after unscrambling in *both* cases (for example, rising from 86% to 90% for a scrambled file, and 90% to 93% for an unscrambled file). A disassembler cannot clearly tell which file is scrambled.

Removing the disassembler also keeps `bincheck` entirely under permissive licenses (BSD 2-Clause, and the KOS License for `src/scramble.c`), since `dcdis` is GPLv3.

---

## Checking that the file is a Dreamcast program

Because the entropy calculation is purely statistical, it produces numbers for any file—even a Windows `.exe` or a JPEG image.

To prevent confusion, `bincheck` performs two sanity checks. Each one only prints a warning (on `stderr`) when it fails; the verdict is still printed.

### 1. Detecting ELF files
A compiled executable right out of GCC is typically in ELF format. If the file starts with the ELF magic header bytes (`\x7fELF`), `bincheck` prints a warning, since it is designed for raw `.BIN` files (the output of `objcopy -O binary`) and the result is meaningless for an ELF file.

### 2. Identifying SH-4 machine code
Rather than disassembling everything, `bincheck` counts how often two specific 16-bit instructions appear:
- `sts.l pr,@-r15` (`0x4f22`): Saves the return address onto the stack (function start).
- `lds.l @r15+,pr` (`0x4f26`): Restores the return address from the stack (function exit).

GCC includes this pair in virtually every C/C++ function that calls another function. While generic instructions are common everywhere, this exact instruction pair is extremely rare in non-SH4 files.

`bincheck` calculates an **SH-4 code score** (occurrences per 10,000 words). The file passes if both instructions are found and the score is at least **2.0**:

| File type | SH-4 code score |
|-----------|-----------------|
| Raw Dreamcast binaries | **10.7 to 145** (most are above 45) |
| Dreamcast ELF files (code + debug symbols) | ~12 |
| Non-Dreamcast files (x86 binaries, images, text) | **0 to 0.85** |

Because scrambling moves whole 32-byte slices, 16-bit instructions are never broken apart. The SH-4 score is identical whether the file is scrambled or unscrambled, so this check can run immediately without needing to unscramble anything first.

---

## Limitations

- **Calibration & Toolchains:** The 15% threshold has been verified on about 160 programs (over 300 files): KallistiOS binaries from current, 2004 and 2011 versions, and a NetBSD kernel built with the NetBSD toolchain. Binaries from other toolchains have not been tested yet. You can adjust the threshold using `-m` or block size with `-b`.
- **Heavy Asset Binaries:** A binary with very little code and massive embedded assets (such as uncompressed music or textures) might have fewer function calls, which could result in a lower SH-4 score warning. The verdict is still calculated and displayed.
- **Very Small Files:** Binaries with very little code or data may lack enough contrast for entropy variance to show a clear gap.
