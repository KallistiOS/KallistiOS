KallistiOS ##version##
Copyright (C) 2002, 2003 Megan Potter
Copyright (C) 2012-2019 Lawrence Sebald
Copyright (C) 2024 Donald Haase
Copyright (C) 2025 Eric Fradella

RELEASE NOTES for 2.2.0
-----------------------
A significant change has been made regarding the default floating-point ABI used
by KallistiOS. In previous KOS releases, and even in commercial games released
during the Dreamcast's lifetime, the `m4-single-only` floating-point ABI was
used. With the `m4-single-only` ABI, the SH4 CPU is always in single-precision
mode and all uses of 64-bit `double` values are truncated to 32-bit `float`
values by the compiler, allowing the compiler to use twice the number of
floating-point registers at the expense of precision. Going forward, KOS now
uses the `m4-single` floating-point ABI by default. With the `m4-single` ABI,
the SH4 CPU is in single-precision mode upon function entry, but the compiler
can change the mode and use true 64-bit `double` values within functions. For
most projects, there are no implications from this change other than gaining
the ability to use 64-bit `double` values. There are, however, two possibilities
for negative effects:

- In older projects using `double` values (which were actually being truncated
  to 32-bit `float` values upon compilation anyway), the code should be changed
  to explicitly use `float` values instead. If not changed, fewer floating-point
  registers may be available to the compiler, in exchange for a needless bonus
  doubling of floating-point precision.

- The order of floating-point register names is changed. When using
  `m4-single-only`, register names are ordered fr4, fr5, fr6, fr7, etc., but in
  `m4-single`, register names are ordered fr5, fr4, fr7, fr6, etc. In order to
  account for this difference and still have inline assembly functions using
  floating-point registers work properly regardless of the ABI used, the
  KOS_FPARG(n) macro has been provided in `arch/args.h`.

Despite the change to `m4-single` by default, KOS is still committed to full
support for `m4-single-only` in addition to offering new support for
`m4-single`. This is selectable using the `KOS_SH4_PRECISION` environment
variable within environ.sh. It is highly recommended to compile KOS, all
kos-ports, and all libraries with the same uniform setting as your projects.
Other ABIs, such as `m4` or `m4-nofpu`, are not supported at this time.


RELEASE NOTES for 2.1.1
-----------------------

This minor patch version is primarily aimed at fixing the versioning system
which simply didn't work as implemented in v2.1.0. Alongside that another few
dozen PRs were included that containing minor bugfixes and documentation updates.

Also included is a new host-side util pvrtex which converts standard images
to formats used directly by the Dreamcast's PowerVR (utils/pvrtex), a significant
rewrite of wav2adpcm which converts standard sound data into the smaller ADPCM
format used by the Dreamcast's AICA (utils/wav2adpcm), an example that
demonstrates how to draw lines with quads via the pvr (pvr/pvrline), one for
testing network speed (network/speedtest) and another on how to use libADX
from kos-ports for audio playback (sound/libADX).

RELEASE NOTES for 2.1.0
-----------------------

# What's New in Version 2.1.0

KOS v2.1.0 has been a long time in the making. As such, it seemed prudent to
provide an overview of the new functionality since v2.0.0 in 2013. We intend
to have more frequent versioned releases moving forward, so this kind of
information should be easily seen in the changelog.

# Core Functionality
* Cooperative Threading mode is no longer supported.
* Static Thread Local Storage (TLS).
* C11 threads and worker threads.
* /dev/ vfs supporting null, random, and urandom.
* VFS Expanded with readlink, rewinddir, and more compliant readdir and stat.
* Expanded C language support including C11, C17, and C23.
* Expanded C++ language support including C++11, C++14, C++17, C++20, C++23, and C++26.
* Expanded POSIX support: clock_gettime/settime/getres, getaddrinfo/freeaddrinfo, libgen.h, and more.
* GCC 9-15 supported. Support for GCC 2-3 removed, and 4 deprecated.
* Default language spec of the codebase is now gnu17/gnu++17.

# Dreamcast Hardware Support
* NAOMI/NAOMI2 including net-dimm uploading.
* New and enhanced driver for SH4 User Break Controller (UBC).
* SH4 Watch Dog Timer (WDT) device.
* Hardware Performance Counters.
* Support for m4, and m4-single modes alongside m4-single-only.
* Store Queue access is now managed by KOS and direct access may break.
* PVR YUV converter DMA.
* PVR 'cheap' shadows via volume modifiers.
* PVR Two-pass render-to-texture option.
* CD-ROM DMA, subcode, and alternative data type reading.
* 4/8-bit wav support for sfx and streaming audio.

## Peripherals and Accessory Support
* French AZERTY, German, Spanish, and UK keyboards.
* Basic Lightgun support based on libronin's implementation.
* VMU buttons, date/time, BIOS color, and using the 'extra 41 blocks'.
* Enhanced support for testing the capabilities of connected controllers.

## Hardware Modification Support
* Additional G1 ATA device (IDE hard drive mod).
* 32MB RAM upgrade.
* Custom BIOSes.
* Navi modified Dreamcast subarch has been moved to addons.

Below are more verbose notes for some of the changes
-----------------------
There are a lot less major changes in this release than in the previous one,
that is for sure. Of course, this isn't to say that there hasn't been some
interesting changes along the way.

The first change is that all targets deprecated in 2.0.0 were removed entirely
from the tree. That is to say, there are no remnants of the GBA, PS2, or ia32
ports of KOS in the tree anymore. If someone REALLY wants them back, please let
me know at some point and we can work that out. I doubt this will come up at
all, however. In addition, the navi subarch was moved out of the main tree and
into the libnavi addon library.

Further standards compliance issues were worked out for this release. KOS' core
should now compile cleanly with a relatively new GCC with the -std=c99 flag
(as well as -Wall and -Wextra). Older (prior to 4.7.x) versions of GCC might
complain about one or two things here and there, but that is not of any
particular concern.

The fs_stat() function has been completely reworked to actually map cleanly onto
the normal C stat() function. This means you must use a struct stat when calling
the function, rather than the removed stat_t (you'll get an error if you try to
use the old struct, since it is completely gone now). If I recall correctly, the
only filesystem to actually have any direct support for fs_stat() before was
fs_vmu (which only supported it in a strange manner to get the free space left
on the VMU in question). The fs_vmu behavior has been retained so that if you do
something like fs_stat("/vmu/a1", &buf, 0), you will still get the number of
free blocks in buf.st_size (the standard doesn't say what to do with the st_size
value on a stat() call about a directory, so this is actually compliant with the
standard, oddly enough). More filesystems support fs_stat() directly now, such
as fs_dcload/fs_dclsocket. Also added is an fs_fstat() call (which, of course,
maps onto the normal C fstat() function). If a filesystem doesn't support
fs_fstat(), it will get a simple mapped version of it, much like with fs_stat().

Hardware-wise, a new driver was added for accessing a hard drive that might be
hooked directly up to the GD-ROM port. The GD-ROM itself is actually a bit of a
strange ATA device, and it is entirely possible to chain a slave device off the
connector with a bit of a hardware modification. The new driver is in g1ata.c
(in the kernel/arch/dreamcast/hardware directory), and should work relatively
well with devices that comply with the ATA standard. The driver supports both
PIO and Multi-word DMA based access to the hard drive device, and can vastly
surpass the potential speed of the GD-ROM itself (with DMA, I get around 12.5
megabytes per second reading sequential sectors off the drive in testing). There
have been a few small modifications to the cdrom.c file to accommodate the
possibility that a device other than the GD-ROM drive might be selected on the
bus (the BIOS syscalls do not check what device is selected). GD-ROM access is
still done through the BIOS syscalls for various reasons (including dcload ISO
redirection, which relies on the syscalls being used). The hard drive access
layer exports a kos_blockdev_t interface to interact with the drive, so you
should be able to use libkosext2fs with hard drives without any difficulties.

The microphone driver (for the Seaman mic) has been changed around a bit. The
internal buffer has been removed in favor of a callback-based sampling approach.
The callback will be called each frame (in an IRQ handler context) while
sampling with the samples collected that frame. The idea is that you'd copy the
samples into some buffer in your program and basically return immediately from
the callback (it is called in an IRQ handler context, so you really shouldn't be
doing a lot of work in the callback).

The kos-ports tree has been changed quite a bit from the last release. No longer
does the kos-ports tree itself host all the source code of the various libraries
it contains, but rather just (generally) a Makefile with some metadata and any
patched or additional files needed for building the library for KOS. You build a
library simply by going into its directory in the kos-ports tree and running
"make install clean", much like the FreeBSD ports collection. Check out the
README in the kos-ports tree for more information. Some libraries have been
updated in the switchover, so keep that in mind. Anything that uses Lua or
liboggvorbisplay will probably have a bit of fixing up needed. Lua has been
updated to 5.3.0. liboggvorbisplay has been split into three libraries: libogg,
libvorbis, and liboggvorbisplay (the first two are the official libraries from
Xiph.org and liboggvorbisplay is the KOS wrapper for them).

Also, speaking of kos-ports, SDL has been updated a bit to version 1.2.15. If
anyone is interested in updating SDL further to the 2.x versions, feel free to
contact me. As of now, nobody's maintaining the KOS port of SDL, so it could use
a maintainer too. Continuing on with kos-ports news, a port of the Opus audio
codec (the successor to Vorbis and Speex) has been added. Like the Vorbis port,
this one is split into multiple libraries, mirroring how it is distributed. Opus
and Opusfile are direct from Xiph.org. The libopusplay library links against
those two libraries and adds in the KOS-specific interface, which should look
very familiar to anyone who's used liboggvorbisplay.

A new VFS driver for accessing FAT filesystems was added in libkosfat in the
addons tree. This driver supports FAT12, FAT16, and FAT32, including proper
long filename support. FAT isn't quite as robust of a filesystem as ext2 is,
but it is probably a lot easier to work with on SD cards, considering how
widely supported FAT is on pretty much every OS ever.

Basic support has been added for getting things up and running on a NAOMI or
NAOMI 2 arcade board (since both are variants of the Dreamcast hardware). In
order to do this, build KOS with the KOS_SUBARCH set to "naomi". Support is
fairly basic at the moment, but will hopefully improve over time (if I can get
a hold of working hardware again). Two utilities have been added to the utils/
tree for network bootable NAOMI binaries and for actually network booting them,
assuming you have the requisite hardware.

Support was added for various mods that have been made for the Dreamcast
hardware in recent years. Support for ATA devices on G1 was already mentioned
up above, but also added was support for modified BIOSes (necessitating a
change to the GD-ROM initialization code), and more interestingly for consoles
with 32MiB of RAM.

Support has been removed for using toolchains with GCC 3.x and older. Going
forward, at least GCC 4.7.4 is required for building KOS. The GCC patches for
4.x improved/cleaned up building with KOS a lot, and I doubt there's many good
reasons to keep around support for the old patches with GCC 3.x.

POSIX threading (pthreads) support has been moved out of the kernel and into
its own addon library (libpthread). This support has been vastly improved and is
much more complete and standard-compliant than it was before. It still isn't
100% POSIX-compliant by any means, but it's a lot closer than it was. There is
no guarantee that this will work with GCC's `--enable-threads=posix`, as that
configuration is not tested/supported any longer in dc-chain.


RELEASE NOTES for 2.0.0
-----------------------
This release has been a long time coming, to say the least. Pretty much every
part of KOS has been modified in some way since 1.2.0 and many things have
undergone a complete overhaul. After almost a decade of living exclusively in
the source repository things have finally settled to a point where a release is
possible and a good idea.

All of the various platform targets for KOS other than the Dreamcast target are
considered deprecated unless someone else steps up to maintain them. If nobody
steps up, these will be removed at a later date. I somewhat doubt that any of
the other platforms can be built successfully anymore at this point.

Several libc standards compliance things were fixed, so stdlib.h no longer
includes assert.h for you. This will break some code that assumes that
assert() is available when stdlib.h (or kos.h) is included.

Speaking of libc-related standards compliance stuff, the built-in libc has been
removed entirely in favor of using Newlib directly. You must build a patched
Newlib for use with KOS. The patches needed for various versions of Newlib are
included in the utils directory of the KOS source.

The build system (including environ.sh) has seen some overhauling. You'll
need to build a new environ.sh from a sample again. Additionally, your
Makefile may need to change. See the examples.

The sound stream system has changed to accommodate multiple streams. Please see
kernel/arch/dreamcast/include/dc/sound/* for the new info. In particular, you
will need to call snd_stream_init from your program before using any of the
libraries like OggVorbis. Also if you are a stream user, you need to alloc
and free channels, and pass a handle along with it. See the OggVorbis libs
for an example.

The dbgio functions have changed. It now implements a full debug-friendly
console system. See include/kos/dbgio.h for more info.

There is a new build system for addons/ports which is quite a bit more
automated than the old way, and is arch-centric. Now to build a new addon
you downloaded, just extract it into addons/ and it will be built for
your arch if possible.

Subsequently, most everything that was in addons/ in previous versions of
KOS is now located in its own source control tree (and will be distributed
separately).

Several incorrectly placed pieces were moved from kernel/ into their own
addon (libkosutils). If you use the bspline or kos_img_* functions, you'll
need to add -lkosutils to your link line somewhere.

KOS now has a built-in network stack in the kernel/net directory. This is only
usable at the moment with the Broadband Adapter or the Lan Adapter for the
Dreamcast. It supports UDP and TCP over both IPv4 and IPv6. You also have an
almost complete set of sockets functions so that you can use the networking
support just like you would on any other OS. Add INIT_NET to your KOS_INIT_FLAGS
to initialize the network support on startup.

If you are using the networking support on the Dreamcast, it is now possible to
use dcload debugging through KOS' network stack. This is automatically set up
for you if you initialize the network at startup time (through the
KOS_INIT_FLAGS).

The old Maple API (that was deprecated in 1.1.8) has been removed entirely from
the source tree. Please update any code that may still be using it to the "new"
API that has been around since 1.1.8.

There are a few new Maple drivers included with this release that were not
around previously. There is now support for the PuruPuru pack, the Dreameye
camera, and the Sound Input Peripheral. See the appropriate header files for
more information about how to use each of these.

All of the KOS header files have had Doxygen comments added to them. From now
on, anything to be added to KOS should be documented before it is included in
the main tree.

A lot of cobwebs have built up over the years in the KOS source, and its quite
possible that there are parts of the code that do not work properly anymore.
Please, if you find anything that doesn't work properly anymore, report an issue
on the Sourceforge bug tracker!

Many changes were made to the synchronization primitives and other threading-
related things. Make sure to take a look at the documentation on them.

Support for the homebrew serial port SD card reader has been added. There is a
block-level driver for SD cards, as well as a lower-level driver for using the
serial port as a generic SPI bus.

If you would like a ready-to-use filesystem to go with your SD card support,
look no farther than libkosext2fs (in the addons tree). This provides support
for reading and writing to ext2 filesystems. This library is still pretty raw
and could use a lot more testing, but it seems to work fairly well as long as
you respect the few limitations in it (no files >= 4GiB in size being the big
one, absolute paths in symlinks don't work either). As a result of adding this
filesystem into the tree, there were a bunch of other changes made in the VFS
code. Specifically, fs_link() and fs_symlink() were added.

The GBA, ia32, and PS2 ports of KOS are all considered abandoned and are likely
to be removed in the future. If you would like to step up to maintain/improve
them, please let us know!

There's probably a whole bunch of other stuff that should be mentioned in here,
but its been so long since anyone has updated this document...


RELEASE NOTES for 1.2.0
-----------------------
The PVR API's performance/statistics measuring facility has changed.
Rather than try to keep backwards compatibility, the new structs have
been changed so that the names are more accurate. The main change that
will be user-noticable is that "frame_count" has become "vbl_count",
counting the number of VBlanks, which is a much more useful measurement
(so you can do constant rate animations and such).


RELEASE NOTES for 1.1.9
-----------------------
The snd_sfx_* API has changed to allow for unlimited numbers of sound
effects. The main difference is that you now use sfxhnd_t instead of int
for addressing sound effects, and the invalid return value for snd_sfx_load
failure is 0/NULL/SFXHND_INVALID and not -1.

libdcutils has been removed at this point. Everything that was in it has
been moved elsewhere or just removed in general. This includes:
  - 3dutils -- moved into kernel/arch/dreamcast/math
  - bspline -- moved into kernel/misc
  - matrix -- moved into kernel/arch/dreamcast/math
  - pcx.c -- moved into addons/libpcx
  - pcx_texture.c -- ditto
  - precompiler.c -- removed
  - pvrutils.c -- redundant, removed
  - rand.c -- removed; a different randnum() is in libc now
  - sintab.h -- removed
  - tga.c -- moved into addons/libtga
  - tga_texture.c -- ditto
  - vmu.c -- merged into kernel/arch/dreacmast/hardware/vmu/vmu.c

The GBA code base should be functional again. I've sync'd in a bunch of
changed from Gil Megidish which brings everything relatively back up to
date:
  - math.h: GBA now supported, and include/newlib-libm-arm
  - lua and lwip not compiled for GBA
  - support for romdisk and initflags for GBA
  - mockups for threading/irq for now
  - pogo-keen example

The thread scheduling system has been changed up slightly, though this
shouldn't affect most users. If you call thd_schedule or thd_add_to_runnable,
then you should probably look at the notes in kernel/thread/thread.c above
thd_schedule.

The fake "thd_enabled" has been removed completely at this point. If you
had code which checked it (it used to resolve to "1") then you should
go ahead and remove those "if" statements. The closest thing at this point
would be thd_mode == THD_MODE_COOP.

A very early port to the PS2 RTE has been added to the source tree, but
will not be released as binaries (not mature enough yet). If anyone plays
with this or has fixes, I'd very much like to hear from you.

The SYSCALL macro was _very_ broken, as in "I'm surprised it works"
magnitude of broken. This may be responsible for some of the apparent
breakage with newer compilers.


RELEASE NOTES for 1.1.8
-----------------------
There is now a new public maple API. Please see the examples for how to
use this new API. It's pretty similar to the old API except that you call
different functions to get the info, and some of the data interpretation
has changed since the last version. Specifically, controller buttons are
no longer inverted and the joystick is centered at 0 like one would expect.

If you used to use vmu_icon_init in libdcutils, it has been replaced by
vmu_set_icon. The new one will target all attached VMUs.

The sound API (higher level one with streaming and such) now has a real
allocation system for SPU RAM. This means that, like the change earlier
in TA->PVR, you can no longer just assume that SPU RAM is free to trample
on, nor can you assume that resetting the stream driver will release your
samples. You have two options here: you can use snd_sfx_unload to unload
a single sample loaded with snd_sfx_load; or you can use snd_sfx_unload_all
to unload all loaded samples at a shot. Note that unlike previous versions,
this will not touch other samples you may have allocated (or streaming
buffers) so these must be done separately. Calling snd_init() will reset
all SPU allocation.

The sound stream API has changed quite a bit internally, but the main
external change is that the "more data" callback now returns not only a
block of data, but the amount of data.

The deprecated TA API has been removed entirely. You need to convert any
remaining code to the new PVR API or KGL. You can take a look at the
examples to see how this works, but here is a quick rundown:
   - poly_hdr_t becomes pvr_poly_hdr_t
   - ta_poly_hdr_txr becomes pvr_poly_cxt_txr
   - ta_poly_hdr_col becomes pvr_poly_cxt_col
   - pvr_poly_compile must be called to generate the actual pvr_poly_hdr_t
   - ta_commit_vertex and ta_commit_poly_hdr become pvr_prim
   - TA_VERTEX_NORMAL becomes PVR_CMD_VERTEX
   - TA_VERETX_EOL becomes PVR_CMD_VERTEX_EOL
   - TA_ARGB4444 (and others) become PVR_TXRFMT_ARGB4444 or whatnot
   - TA_NO_FILTER becomes PVR_FILTER_NONE
   - TA_BILINEAER_FILTER becomes PVR_FILTER_BILINEAR
   - TA_OPAQUE becomes PVR_LIST_OP_POLY
   - TA_TRANSLUCENT becomes PVR_LIST_TR_POLY
   - "uint32 texture" becomes "pvr_ptr_t texture"
   - ta_txr_allocate becomes pvr_mem_malloc
   - Textures must be freed with pvr_mem_free
...and so forth. Most of the API changes are cosmetic, but it's important
to pay attention and make sure you understand the shifts in paradigm where
appropriate as well (such as raw texture space to managed, allocated
texture space; commit_eol gives way to real lists; etc).

The "scene idiom" has also changed to the following:
  pvr_wait_ready();
  pvr_scene_begin();
  pvr_list_begin(PVR_LIST_OP_POLY)
    /* Do your opaque rendering */
  pvr_list_finish();
  pvr_list_begin(PVR_LIST_TR_POLY)
    /* Do your translucent rendering */
  pvr_list_finish();
  pvr_scene_finish();

Deprecated kos_init_all and kos_shutdown_all have been removed.

Deprecated compat macros like ALL_ENABLE have been removed.


RELEASE NOTES for 1.1.7
-----------------------
KOS 1.1.7 is probably one of the biggest, nastiest upgrades KOS has seen
since the 1.0.x -> 1.1.x transition. Unlike that transition =) this one
brings many fixes and helpful features. Most things will continue working
just fine, but specific issues are listed below. Please check through
this if you have problems.

Initialization has changed somewhat. Now instead of calling kos_init_all(),
you will need to use one or more of the KOS_INIT_* macros in
arch/arch.h. These include KOS_INIT_FLAGS and KOS_INIT_ROMDISK. Note that
there are new names for the flags to OR together, also. Please check
kernel/arch/dreamcast/include/arch.h for more info, and/or see the
examples.

Threading is also different now. Threading is now always enabled. Now
before you groan and moan at me, there are now two modes of threading
instead of just enable/disable: cooperative and pre-emptive. In
cooperative threading mode, the thread module is active and it is
possible to do things like thd_pass(), use condition variables
between the main program and an IRQ, etc. However, the timer is not
hooked and no pre-emption will occur. If you enable pre-emptive
mode, then this is basically like the old threads-enabled mode.

Note that kos_init_defaults() is now a compatibility shim which will
correct any implicit defaults. However, if you want better control over
this situation, please change your program to use the macros. Also note
that this and other compatibility shims will be removed by the next
release version (i.e., removed in CVS after the tagging).

The build process has changed slightly. The main change is that libc is
in its own tree, and thus has its own include path. If you are using the
KOS Makefile templates, then you should use $(KOS_LIBS) at the end of your
link line (use this in place of -lkallisti -lgcc). You must also add
-I$(KOS_BASE)/libc/include to your CC line if you're using your own
custom Makefiles.

A couple of things have changed in the environ file, though nothing
drastic. Your existing environ should still work, but I recommend at
least adding the KOS_STRIP variable, as well as adding the
-fno-optimize-sibling-calls parameter to KOS_CFLAGS if you haven't built
a fixed GCC 3.0.3.

Libc has been split out of 'kernel' and into its own tree. This is what
triggered the build process change. In the future this will make it very
easy to replace libc with another libc (such as Newlib). Note that libc
is ported to KOS, not the other way around. This is why the libc objects
are still combined into libkallisti.a (easier linking until we have the
installation mode available...)

Libm from Newlib has been integrated into the source tree so that you
no longer have to pull in a separate Newlib binary. This also ensures that
it's compiled with the same compiler flags as the rest of KOS.

The new "PVR" API has completely replaced the old "TA" API. For the near
future, code based on the "TA" API will continue working, through an
adaptation layer. The one thing which really can't be emulated properly
with the adaptation layer is custom memory management (i.e., allocating
your own textures starting at texture address 0). "PVR" texture pointers
are now real SH-4 pointers, so you must allocate them through the 3D
subsystem or you'll get garbage for textures.

The streaming and basic sound effects portions of the MP3 and OGG libraries
has been split out and placed into the kernel now, as an architecture
independent interface. The DC implementation uses a generic AICA driver
which has been improved upon greatly since the last version. This has three
implications for anyone using sound stuff:

1) If you used sfx_* functions, you must now use snd_sfx_*
2) It is now possible to use basic sound effects without loading the MP3
   or OGG libraries
3) You no longer need to include stream.drv in a romdisk; it's built into
   the library itself now

The entire maple system has been replaced. Most things will still work
as before, but one of the most notable changes is that you will no longer
need to pause between polls on the maple bus. This is all handled
automatically in the background. Enumeration is done by using the
maple_enum_* functions (see dc/maple.h) and the way to access the
keyboard matrix and shift states is different also (see dc/maple/keyboard.h).
VMU saving should be considered somewhat "beta" as is the hotswap
capability. We're still working on finding and fixing issues there,
especially with third-party peripherals.

One consequence of this change which you should pay attention to for your
own programs is that maple_first_controller() and friends might conceivably
fail at one point during your program, yet succeed later. So you'll want
to poll for the devices you want before each condition check rather than
when the program starts. For the most part, the examples have been
updated to do this.

KGL has become a lot more OpenGL(tm) compliant. This means, for example,
that the usage of radians has been deprecated in favor of degrees,
images are expected to be loaded inverted, etc. If you program which
previously worked under KGL is having some issues, you should probably
check to see what changed there. Paul has helpfully created a KGL manual
as well, if you are looking for docs.

Image loaders now use the kos_img_t system so that they can be platform
independent and still pass around the data in a convenient format. This
also makes it easier to flip the data when loading it into the PVR RAM
for KGL usage. The loaders for PCX and TGA have additionally been split
out into their own libraries (libpcx, libtga). So you will need to
use -ltga or -lpcx for these in your link line.

Finally, if you do not have a working G++ compiler for your target, then
please comment out the line in environ for KOS_CCPLUS. This will disable
building any C++ targets. Conversely, if you have a working G++, make sure
you have a KOS_CCPLUS line so that all of the libraries and examples will
get built.
