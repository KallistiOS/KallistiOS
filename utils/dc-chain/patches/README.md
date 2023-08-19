# Patches Information #

This file list patches information applied to the toolchains before building it
for usage in **KallistiOS**.

Patches are located in the `patches` directory. These are automatically applied
when using the **dc-chain** `Makefile`.

`gcc-3.4.6-diff`:

- Defines `SUPPORT_WEAK 0` in `config/sh/kos.h` so that pthreads will not create
  weak symbols (GNU ld does not extract weak symbols from archives).
- Do not build `m2/m2e/m4-single/m4` by default.  Should be unnecessary once
  we have a `sh-kos-elf` target.
- Adds `config/sh/kos.h` to `config.gcc`.  **Note:** this is under `sh-elf-unknown`.
  Once we have a `sh-kos-elf` target, that should be corrected.
- Add `sh4/atomicity.h` stub, not currently used.  Requires addition of spinlock
  or interrupt disable/enable code.  Currently we use the generic atomicity
  functions, which use pthreads (slow).
- Correct bug (?) in `config/generic/atomicity.h` (`_GLIBCXX_` macros undefined).
- Add `sh-unknown-elf` defines to `libstdc++/crossconfig.m4` -- this was done by
  default (accident?) prior to 3.4.

Generic `newlib` fixups (applied directly after `newlib` is installed):
```
cp $(kos_base)/include/pthread.h $(newlib_inc)                       # KOS pthread.h is modified
cp $(kos_base)/include/sys/_pthread.h $(newlib_inc)/sys              # to define _POSIX_THREADS
cp $(kos_base)/include/sys/sched.h $(newlib_inc)/sys                 # pthreads to kthreads mapping
ln -nsf $(kos_base)/include/kos $(newlib_inc)                        # so KOS includes are available as kos/file.h
ln -nsf $(kos_base)/kernel/arch/dreamcast/include/arch $(newlib_inc) # kos/thread.h requires arch/arch.h
ln -nsf $(kos_base)/kernel/arch/dreamcast/include/dc   $(newlib_inc) # arch/arch.h requires dc/video.h
```
**Note:** For the **MinGW/MSYS** environment, these `newlib` fixups should be
manually applied. See the `mingw` directory for details. For all the others
platforms, these fixups are applied automatically from the `Makefile`.
 
