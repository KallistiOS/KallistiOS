# Filesystem lifetime regression

Build with this checkout's KOS environment and `make`. Set `KOS_GENROMFS` to
the built `utils/genromfs/genromfs` executable; the fixture contains only a
small authored text file. Run `fs-lifetime.elf` on a console or emulator.

Expected serial output: `FS-LIFETIME: PASS checks=1078`.
Every failed check emits a line with the source location and errno.

The test uses real VFS/ROMFS and cooperative barriers between preemptive KOS
threads. It deliberately closes all descriptors, including stdout, so reporting
uses the debug console directly. No disc, networking, fiber runtime, SH4ZAM,
or proprietary firmware/assets are required. Flycast was tested using REIOS
and 16 MiB RAM in interpreter and dynarec modes; physical hardware is untested.

See `doc/fs-object-lifetime.md` for ownership and validation limits.
