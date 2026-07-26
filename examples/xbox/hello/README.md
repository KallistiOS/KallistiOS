# Xbox "hello world"

The first KOS example for the original Microsoft Xbox: a runnable console
program that prints `Hello, Xbox!` over the network via kos-tool's
`xbox-load-ip`.

## How it works

There is no KOS Xbox arch yet (no libc, no video/console driver), so this is a
freestanding **guest** that talks straight to the loader, using the same ABI as
kos-tool's own `console-test` / `xbox-video-test` examples:

- `xbox-load-ip` publishes a header at `XBOX_KOSLOAD_BASE` (`0x00011000`):
  the magic `0xdeadbeef` at `+0` and a syscall trampoline pointer at `+4`.
- `hello.c`'s `start()` verifies the magic, then calls
  `syscall(WRITE, 1, "Hello, Xbox!\n", ...)` to print to the host console and
  `syscall(EXIT, 0, ...)` to hand control back.

## Build

Uses the `i686-pc-xbox` cross toolchain directly (the KOS build env doesn't
cover Xbox yet). It links a PE image at the guest load address
(`0x00400000`) and `objcopy`s it to `elf32-i386`, which is what the loader
consumes:

```sh
make                                                   # -> hello.elf
make XBOX_TOOLCHAIN=/opt/toolchains/xbox/i686-pc-xbox  # explicit toolchain
```

## Run

With `xbox-load-ip` running on the Xbox, upload and execute from the host:

```sh
make run          # = kos-tool -x hello.elf
```

`Hello, Xbox!` should appear on the host console.

## Relationship to the KOS port

This is the **loader-guest** path, which works today. The eventual
**KOS-native** path — a program linked with `kernel/arch/xbox`'s `startup.S`
and `utils/ldscripts/xbox.ld`, using a shared kosload console driver instead of
poking the loader header directly — depends on the arch bring-up still in
progress (`arch_main`, BSS clear, video/console drivers).
