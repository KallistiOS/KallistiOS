# Profiling with gprof on KallistiOS

This example shows how to **profile** a KOS program with gprof — measure *where
your program spends its time* and *which functions call which* — and turn that
into a readable report.

## What is gprof profiling?

When you build with `-pg`, the compiler adds a tiny hook at the start of every
function ("instrumentation"). While the program runs, the libgprof runtime
collects two things:

* a **flat profile** — a statistical histogram of the program counter, sampled
  on a timer, that estimates how much *time* each function used; and
* a **call graph** — who called whom, and how many times, recorded at each
  function entry.

On exit the runtime writes a single `gmon.out` file. You feed that to the
host-side `gprof` tool to read it.

The program writes `gmon.out` to `/pc` — a host folder shared with the console
by your loader. You have to **map** `/pc` to a writable directory for this to work:
`make run` does it with `-m .`, mapping `/pc` to this example's directory, which is
exactly where `make report` looks. (If `/pc` isn't mapped, the run still finishes —
you just see `[GPROF] /pc/gmon.out not opened.` and get no file.

## What you don't have to do

You **never link `-lgprof` yourself.** Building/linking with `-pg` is enough: the
`kos-cc` wrapper sees `-pg` on the link and pulls the runtime in automatically —
the same way `gcc` auto-links `-lgcov` for `--coverage`. You also don't call
anything to start/stop profiling: linking with `-pg` auto-starts it before
`main()` and flushes `gmon.out` on exit.

## Tools you'll need

* **`sh-elf-gprof`** — the gprof tool for the Dreamcast toolchain. It already
  comes with your KOS toolchain; the Makefile finds it automatically (via
  `$KOS_GPROF`), so there's nothing to install.
* **`gprof2dot` + Graphviz** *(optional)* — to render the call graph as an
  image. Only needed for `make graph`. Install Graphviz from your package
  manager:

  | System | Graphviz |
  |---|---|
  | macOS (Homebrew) | `brew install graphviz` |
  | Debian / Ubuntu | `sudo apt install graphviz` |
  | Fedora / RHEL | `sudo dnf install graphviz` |
  | Arch | `sudo pacman -S graphviz` |

  For gprof2dot: on macOS it's a Homebrew formula too, so just
  `brew install graphviz gprof2dot`. On Linux it's usually not in the distro
  repos (it's in the [AUR](https://aur.archlinux.org/packages/gprof2dot) on
  Arch), so install it from PyPI: `pipx install gprof2dot` (or
  `pip install gprof2dot`).

## Quick start

```sh
make            # build the instrumented program (-pg)
make run        # run it on the Dreamcast; writes gmon.out to /pc
make report     # turn gmon.out into gprof_output.txt
```

Open `gprof_output.txt`. The **flat profile** at the top lists functions by the
share of time spent in each; the **call graph** below shows the caller/callee
relationships and call counts.

Optionally, render the call graph as an image:

```sh
make graph      # writes graph.svg  (needs gprof2dot + graphviz)
```

## What this example actually does

`gprof.c` runs a small workload several times. It's shaped so the report has
something to show:

* **`spin()`** is a pure CPU burner — it does most of the program's work. It's
  reached through **`hot_path()`** (many calls) and **`warm_path()`** (far
  fewer), so `spin()` tops the flat profile, and the call graph splits its time
  across the two callers by how often each reaches it (mostly `hot_path`).
* **`fibonacci()`** is naive recursion, kept shallow on purpose. It does little
  real work, but it's a great call-graph subject: a large call count and a
  **recursive self-edge**. Watch its slice of *time*, though — most of it is
  per-call instrumentation overhead (every call takes a trap + mcount), which is
  exactly why it's kept shallow.
* **`main` → `workload` → {`hot_path`, `warm_path`, `fibonacci`} → `spin`** gives
  the graph a clear shape.

So the flat profile answers *"where did time go?"* (mostly `spin`, via
`hot_path`) while the call graph answers *"how did we get there, and how often?"*

> A note on accuracy: because each function call goes through a trap, a function
> called a *huge* number of times accrues real per-call overhead in the profile.
> That's a property of this kind of instrumenting profiler, not a bug — keep very
> hot, tiny functions in mind when reading the times.

## A note on the build flags

The Makefile sets these for you; here's what they mean:

* `-pg` — instrument every function for profiling. Must be on **both** the
  compile and the link command (the link `-pg` is what auto-links the runtime).
* `-fno-omit-frame-pointer` — REQUIRED because GCC rejects `-pg` together with
  `-fomit-frame-pointer` (which KOS sets by default), so this overrides it.
* `-fno-inline` — don't inline, so each function stays a distinct symbol you can
  see in the report.
* `-fno-ipa-icf` — don't fold identical functions. `hot_path()` and `warm_path()`
  have identical bodies; without this GCC would merge them into one symbol and the
  call graph would show only one caller of `spin()` instead of two.

## Choosing where gmon.out is written

By default the file is `/pc/gmon.out`. To send it somewhere else (say an SD
card), set `GMON_OUT_PREFIX` in the program's environment — matching glibc's
gprof, the file is then named `$GMON_OUT_PREFIX.$pid` (e.g.
`GMON_OUT_PREFIX=/sd/gmon` → `/sd/gmon.1`). Point `gprof` at whatever name it
ends up with:

```sh
sh-elf-gprof gprof.elf /path/to/gmon.out > gprof_output.txt
```

> Statistical profiles need a bit of runtime to collect enough samples — this
> example runs for a few seconds on purpose. If your own program is short, loop
> the interesting part, or raise the workload sizes at the top of `gprof.c`.

## Programs that never exit

`gmon.out` is normally flushed automatically when `main()` returns. If your
program never returns — a game that loops forever, or one that reboots — call
`_mcleanup()` (from `<gprof/gmon.h>`) at a checkpoint or right before you stop,
to write the profile on demand. It's the gprof counterpart of the gcov example's
`__gcov_dump()`, and it's safe to call even though the exit-time flush also runs.
