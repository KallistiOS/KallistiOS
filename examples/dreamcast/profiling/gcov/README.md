# Profiling with gcov on KallistiOS

This example shows how to **profile** a KOS program with gcov — measure what your
code actually does while it runs — and what you can do with that information.

## What is gcov profiling?

When you build with `--coverage`, the compiler adds tiny counters throughout your
program ("instrumentation"). As the program runs, those counters record how many
times each line and branch executed. On exit the data is written to a `.gcda`
file, which you feed to a tool to either *read* it (coverage) or *reuse* it (PGO).

Two files are involved:

* **`.gcno`** — written at **compile** time. A map from the counters back to your
  source lines. (You don't read it directly.)
* **`.gcda`** — written at **run** time. The actual counts collected during the run.

On a KOS target the program writes the `.gcda` to `/pc` — the folder on your
computer that your loader shares with the console. `/pc` maps to your filesystem
root by default, so each `.gcda` lands at its real build-time path, right next to
its source and `.gcno` — exactly where the report and PGO tools look for it.

## Two things you can do with a profile

1. **Code coverage** — see which lines and branches ran (and which never did).
   Great for spotting untested or dead code. → *Route 1.*
2. **Profile-guided optimization (PGO)** — hand the profile back to the compiler
   so it builds a **faster** binary, using the real run to drive inlining, hot vs.
   cold code layout, and branch prediction. → *Route 2.*

Both start the same way: build with `--coverage` and run to collect a `.gcda`.

> **Coverage + arc-based PGO only.** GCC's freestanding libgcov on these targets
> ships the arc (edge) counter runtime but no value profilers, so
> `-fprofile-generate` / `-fprofile-values` won't link. You get coverage and
> edge-frequency PGO; value-profiling PGO (speculative devirtualization, value
> specialization) isn't available.

## Tools you'll need

* **`$KOS_GCOV`** — the gcov tool for your KOS toolchain. It comes with the toolchain
  and the Makefile finds it automatically, so there's nothing to install for that.
* **`lcov` + `genhtml`** — turn the raw coverage data into a browsable HTML
  report. `genhtml` ships *inside* the `lcov` package, so installing `lcov` gives
  you both:

  | System | Command |
  |---|---|
  | macOS (Homebrew) | `brew install lcov` |
  | Debian / Ubuntu | `sudo apt install lcov` |
  | Fedora / RHEL | `sudo dnf install lcov` |
  | Arch | `sudo pacman -S lcov` |

  (Only needed for Route 1's HTML report. Route 2 doesn't use lcov.)

> First time only: build the addon so the linker can find it —
> `cd $KOS_BASE/addons/libkosgcov && make` (a normal `make` in `$KOS_BASE/addons`
> covers it).

## Route 1 — measure code coverage

```sh
make          # build the instrumented program (CFLAGS = --coverage)
make run      # run it on the console; writes the .gcda files to /pc
make report   # build the HTML report
```

Open `html/index.html` in a browser. Lines that ran are green, lines that never
ran are red, and you get a percentage per file.

Each `make run` **overwrites** the `.gcda` with that run's counts — this runtime
doesn't merge across runs. A single run already accumulates everything, so its
`.gcda` is complete; to combine *separate* runs, merge host-side with
`lcov -a run1.info -a run2.info -o all.info`.

## Route 2 — build a faster program (PGO)

The same `.gcda` can also drive optimization: recompiled with `-fprofile-use`,
the compiler reads it and optimizes from the recorded edge counts. The example's
Makefile is coverage-focused, so build the optimized version by hand, keeping the
`.gcda` from Route 1:

```sh
make ; make run                  # 1-2. build --coverage, run, collect the .gcda
rm -f *.o */*.o */*/*.o          # 3.   drop the objects (keep the .gcda!)
make CFLAGS="-fprofile-use -fprofile-correction -fprofile-partial-training"
make run                         # 4-5. run the optimized build
```

**Don't edit the source between collecting and using the profile** — gcov matches
it to the code by a checksum, and any change makes the compiler discard the
profile (you'll see a `-Wmissing-profile` warning).

> On a tiny example like this you won't *feel* the speedup — PGO pays off on
> large, branch-heavy programs. The point here is that the workflow works.

## What this example actually does

The program is split across several directories — `gcov.c`, `math/ops.c`,
`util/str.c`, `util/text/format.c` — so you can see that **each translation unit
gets its own `.gcda`** at a path mirroring where it was built.

`main()` exercises the helpers (clamp, sign, parity, string length/count) over a
range of inputs, deliberately leaving some branch arms cold (e.g. `op_clamp`'s
lower-bound path for non-negative inputs, `fmt_parity`'s "zero" arm). So the
report shows a realistic **mix of covered and uncovered** lines and branches
across all four files and three directories.

> Returning from `main()` flushes the data automatically. A program with an
> infinite main loop that never returns can call `kos_gcov_dump()` to write it
> explicitly.

## A note on the build flags

The Makefile sets these for you; here's what they mean:

* `--coverage` — instrument for coverage and emit the `.gcno` map (shorthand for
  `-fprofile-arcs -ftest-coverage`).
* `-fprofile-use` — the use side (Route 2): optimize from a collected `.gcda`.
* `-fprofile-correction` — KOS is threaded, so counter updates can be slightly
  inconsistent; this tolerates that instead of erroring out.

`-fprofile-generate` / `-fprofile-values` are **not** usable here — they need
value profilers GCC's freestanding libgcov doesn't ship, so they fail to link.

## Larger projects with nested folders

`make report` runs `lcov --directory .`, which searches this whole tree
recursively — so it already handles sources in subfolders, as this example (with
`math/`, `util/`, `util/text/`) shows. For a project rooted elsewhere, point
`--directory` at it. One rule: leave `GCOV_PREFIX_STRIP` at its default so each
`.gcda` is written at its build path, next to its `.gcno` — that's how lcov pairs
them.

## A note on lcov 2.x

`make report` passes a couple of flags worth keeping if you copy this Makefile,
because lcov 2.x (current Homebrew/distro versions) is far stricter than 1.x and
otherwise aborts on data quirks that are normal for optimized and inlined/header
code:

* `--rc geninfo_unexecuted_blocks=1` — when a line compiles to both executed and
  unexecuted basic blocks, score it as not-covered (0) instead of taking the max.
  This clears most "unexecuted block … with non-zero hit count" mismatches.
* `--ignore-errors inconsistent,source,…` — tolerate the remaining line-vs-
  function hit-count inconsistencies (common for `inline`/header functions) and
  missing-source lookups, instead of letting `genhtml` fail outright.

Without them, a real codebase typically stops with
`genhtml: ERROR: (inconsistent) …`.
