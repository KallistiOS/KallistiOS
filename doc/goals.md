# Goals for KallistiOS

## Primary Goals
- Thread support (within a single "process")
- Synchronization support
- Dreamcast hardware control with some (compile and runtime?) configuration,
  e.g., printf() goes to the video screen or to the serial port, etc.
  More on this below.
- Low-level interfaces to work with the Newlib C library so that things
  like printf() work as expected. These interfaces include file functions
  (that access the GD-ROM or other filesystems), malloc() and free() with
  a basic bucket algorithm, and other standard functions that are not
  provided with Newlib or where we provide more optimized/useful versions.
- Compile-time controls for many of the above, so that you can turn off
  what you don't need. This is one area where we are currently severely
  lacking.
- Support for loading elf binaries from a file system, relocating them,
  and running them in "immidate mode" or in a new thread.
- Realtime features (thread priorities, event-based rescheduling, etc)
- Lightweight Process support. This does not imply full memory protection
  or anything else of the sort.
- Maybe eventually, MMU support. As a general rule we don't care much about
  address relocation and protection, but during debugging this could be a
  big life saver.


## Non-Goals

- A lot of things that UNIX would provide -- like pipes, real tty's,
  etc.
- Full memory protection support through the MMU.


## Hardware Control Layer

- Provides synchronization for high-level primitives.
- Basic initialization -- initializes all known system components for you
  to reasonable defaults. Some init parameters can be determined at
  compile time (like default video mode), and potentially init can be
  disabled if you want to do it manually for some reason.
* G2 bus module
  - Provide interrupt multiplexing for any module that wants to hook an
    IRQ9 event.
  - DMA handling
* Video
  - Init to any of the three modes (RGB555/565/888) and handle being hooked
    up to a VGA box, an S-Video or a Composite output, or RGB cable.
  - BIOS font interface
  - Hardware control functions for moving start address, changing border
    color, and getting a pointer to the frame buffer.
  - More advanced 3D functions that do things like allocate memory for
    the PowerVR textures, load those textures, and do 3D display lists;
    additionally, functions for creating and using stacked windows (using
    3D accel).
  - Page flipping / 3D pipeline assistance if you want it.
  - G2 interrupt and DMA support to TA and/or VRAM
* Sound
  - Init the sound chip with a sound control program that implements a
    basic MIDI-style system. In reality, this is a MikMod-type abstracted
    Protracker player. (MAYBE, on this one and the next few)
  - Provide wavetable primitives -- loading patches, play, stop, vol/freq/pan
    controls.
  - Provide players for various "tracker" formats -- S3M, MOD, XM, all
    of which interface to the MIDI-style interface.
  - Provide a more advanced direct output mechanism that lets you output
    buffers of raw sound data at a given sample rate, blocking your program
    until it's ready for a new buffer. Should also provide double buffering
    support.
  - Allow for uploading custom sound programs, and provide some abstraction
    mechanisms for getting the address of sound RAM.
  - G2 interrupt and DMA support
* Maple Bus
  - Init that enumerates devices
  - Polling primitives for the known device types, mainly keyboard and
    controllers.
  - Unified enumeration system to let you figure out what's on the
    bus and potentially use it.
  - G2 interrupt support for device activity and addition/removal.
  - Modules for using each of the available device types.
* VMU
  - Functions to load/save VMU files
  - Display VMU graphics (on the VMU display)
  - VMU sound
* Serial port
  - kernel stdin and stdout/stderr map to the serial port or debug
    stub output
  - printf() and similar functions point to that stdio
  - getchar() and similar read from that stdin
* Virtualized file system
  - Like Neutrino -- delegated path spaces
  - Default drivers will include:
    + ROMFS (linked into the program or perhaps tacked onto
      the end of the BIN)
    + CDFS (reads from a CDR)
    + PC Debug (reads over a debug cable [ethernet, etc])
    + RAM Disk (for temporary files)
* GD-ROM
  - Hardware driver that accesses the GD-ROM directly, uses
    G2 interrupts, etc.
  - Access to the ISO file system on a CDR/CDROM
  - Playing audio tracks on music CDs
* Debug support (optional)
  - Interfaces to GDB over the serial port or ethernet
  - stdout gets redirected to debug stub output functions
* Expansion port
  - Ethernet
  - Modem
* TCP/IP stack
  - PPP support for the modem
