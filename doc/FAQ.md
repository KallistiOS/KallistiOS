# Frequently Asked Questions

## Licensing

### Disclaimer

This section is NOT legal advice. It's just a help to people who are
having trouble with the myriad free software / "open source" licenses.
If you need legal advice, we suggest contacting a professional.

### What is the new BSD/KOS license? What exactly do its terms mean? Can I make a proprietary / "closed source" program using KOS? Can I distribute "forked" versions of KOS?

The BSD license is pretty well understood among the free software
community, but I'll go ahead and give a layman's description of it here.
Basically what it says is that you can use the code in any of your
programs (proprietary, free/"open source", etc) as long as you don't try
to remove or change the copyright notices, and you don't try to
advertise your program as if it's endorsed by us. You may also
distribute changed versions as long as they are clearly marked as such
(often called a "fork"). This is a layman's description, it is not a
replacement for reading and understanding the license itself. If you
don't understand it, please find a lawyer friend to help you with it.

Sorry for being so terse about this, I'm just sick of "intellectual
property" battles. That and people somehow equate the BSD or X11 license
with "I can rip and use this code with no credit". This is absolutely
false. You may also not include the KOS code (or any other copyrighted
code) in your program verbatim without credit and a copyright notice.
Very small pieces are probably alright, but anything substantial must
have something to the effect of "derived from blagh.c in KOS 2.1.x".
It's really a very small price to pay for this nice code library you
can pull from freely :)

### What about kos-ports licenses?

The kos-ports repository provides ports of libraries with a variety of
licenses. For each, the applicable license is noted in its Makefile as
the LICENSE define. This can also be verified with the original authors.
Please review and be aware of these licenses if you wish to use kos-ports.

### Attribution

To reiterate: THE KOS LICENSE DOES NOT ALLOW USE WITHOUT ATTRIBUTION
In order to help facilitate this, we provide [the license text](./license/LICENSE.KOS),
[a listing of authors](../AUTHORS), and functions `kos_get_authors` and 
`kos_get_license` to easily access this text within your code.


## Operating System Basics

### Why do you call this an "operating system" if it's just a library?

This seems to be a point of common confusion. Most of the
misunderstanding stems from the fact that both the kernel and you can
link the kernel itself into your program. Many people who are writing
software have not really had much exposure to a true embedded
environment before and so this confusion is understandable.

The most common definition for an OS I've heard is a program or API
layer which translates something undesirable to work with (for example,
raw hardware) to something desirable to work with (for example, fopen(),
fread(), fclose(), etc). You could group a lot of things under this
definition, like a JVM -- you are taking an unruly and unportable
OS-level API and translating it to something that is the same for all
platforms, even down to the binary code. To be more precise about the
common meaning though , the "undesirable" state is actually raw hardware
and the "desirable" state includes some way to run multiple tasks at
once, a virtual file system, hardware drivers, perhaps a TCP/IP stack
and other such services.

In a standard OS like you might run on your development workstation, the
OS "kernel" is what performs these tasks, and it provides an interface
for normal programs ("userland") to talk to the OS and other programs in
a very controlled manner. It is easy to see how this userland layer is
just another abstraction which isn't really even necessary if you don't
want its features -- and on a console where you must take advantage of
being as close to the hardware as possible, it will just get in the way.

Thus what ends up happening is that you link your "userland program"
directly into the kernel itself, and this is loaded on the target system
all at once instead of in two pieces. Your program also gets full,
unfettered access to everything the kernel can do; so if you don't like
the way it does something, you can change it.

### What's a virtual file system, and why do I want one? Why don't I just talk directly to the device I want to use?

A virtual file system is basically an object oriented multiplexer for
a generic file interface. What that means in human-readable terms is that
it translates a full path name to a device handle plus a device name. For
example, if you access "/cd/foobar.txt", and "/cd" is mapped to the
CD-Rom driver, then you will actually access "foobar.txt" on the root of
the CD-Rom device.

To fully appreciate this design decision, you must port a load of
software to KOS. :) But the quick answer is, it provides a (mostly)
uniform interface to any sort of storage medium. This way, for example,
the PNG library doesn't need to know that it is loading files from a
VMU, from inside an internal ROM image, from over the network, etc...
all it knows is that it has a file descriptor it can use to access that
data. With the concept of a working directory, this allows you to very
easily switch targets during development. For example, you can use all
relative paths to identify your files (no leading slashes) and then put a
fs_chdir at the top of your program to switch to the file source, whether
that's over the ethernet using a loader or from a CD.

### What are threads? Why do I want to use them? Why are we wasting resources with such a silly thing on a console? Why don't you ninnies code like real men and do your own task multiplexing?!

Threads are basically very light-weight processes or tasks. It is a
simple way to get the system to do more than one thing at once without
explicitly taking control over that switching, such as making a load of
"per frame" calls to do things like sound mixing. A very simple example
of something that is very difficult or impossible to do without threads
is a 3D "loading" display during a large batch load of data from a CD or
other VFS device.

Yes, the second question has actually come up a number of times. :)
Especially from people disenfranchised with the removal of non-threaded
mode from KOS earlier. This was a design decision that was made to
prevent the continuing spread of code that looked like this:

```
if (thd_enabled) {
	do one thing;
} else {
	do something else;
}
```

The better, and somewhat counterintuitive answer, is that threading can
actually _improve_ your program's performance. Most people who dislike
threading point to context switch overhead (saving and restoring the CPU
state when switching threads), and this is a valid concern (and is
certainly a problem on a purely computational set of tasks). However,
with a properly written set of drivers, OS core, and user program, the
OS can get a much better picture of what is going on and optimize task
scheduling. Remember, most hardware usage involves a lot of busy
waiting which you could spend doing something useful with the CPU. For
example, let's say you are doing a 3D display and playing back an MP3
while loading from the CD. The system state might look like this:

- One thread handling 3D display at higher priority, so that it will
  always get scheduled when the 3D card is ready for more data; when it's
  waiting for the next frame, there's no need to even schedule it.
- One thread that is reading MP3 data from some source (perhaps a ramdisk
  on the VFS) and decoding it for output buffer. Again, it will spend most
  of its time waiting for sound to finish playing back.
- One thread handling data loading from the CD, which spends most of its
  time waiting for data to come back (thus it has no need to even be
  scheduled until something is ready). This thread could be lower than
  normal priority so it would not cause skipping or frame drops with the
  other two.

Assuming properly written drivers for all of the above, the OS could see
what was going on at any given moment and schedule accordingly. And I
think we can all appreciate the considerably reduced complexity from
decoupling all three tasks.

On the other hand, the drivers in KOS aren't very efficient at thread
usage still. This is mostly a left-over vestige from the days when
threads were optional (and when we had less info about hardware IRQ
support), and is slowly being worked out.

### What VFS modules are available?

Presently there are six in-kernel filesystems that you might interact with
on a semi-regular basis (and two in addon libraries):

- ISO9660 -- mounts on /cd, and is a decent implementation of the
ISO9660 file system standard used on CDs. Note that because of the
simple nature of KOS's block devices right now, ISO9660 is bound
directly to the CD-Rom device. In the future this may change. This is
DC-only.

- DC-Load Console -- mounts on /pc, connects to a PC-hosted program,
called 'dc-load'. DC-Load is Andrew Kieschnick's BBA/Serial port loader
program. If you use the DC-Load console, then you can read files
directly from your in the /pc directory. The other major advantage (if
you are using a BBA) is that DC-Load will redirect console messages over
the ethernet for you.

- VMU -- mounts on /vmu, what it sounds like. This dynamic file system
shows all detected VMU or memory cards on the maple bus and lets you
browse and read/write them. This is DC-only. Note that in order for files
to show up in the BIOS menu, they must have a specific format, which is
handled by the vmu_pkg code.

- Romdisk -- mounts on /rd. This is an implementation of the Linux ROMFS
file system. One may be built-in to your program to mount on /rd on boot.
In addition, you may load other images and mount them freely at runtime.

- Ramdisk -- mounts on /ram. Works exactly as you expect -- putting files
here eats up core RAM, deleting them recovers it. Otherwise it works
exactly like a normal file system.

- Sockets -- mounts on /sock. This one is not exactly a filesystem so to speak.
It is used to allow network sockets to use the normal VFS functions (write(),
read(), close()) for its operation. There are never any visible files on this
VFS, so you can't use open() to access it.

- ext2 -- mounts wherever you tell it to. This one is implemented in the
libkosext2fs library in the addons tree. It was primarily added to provide a
freely-usable (not patent-encumbered) filesystem that could be used on SD cards
and with g1ata. It is usable with more than just SD cards and hard drives
though, if you want to provide a block device for using it with other things.
There probably isn't much of a good reason to do so, however.

- fat -- much like ext2, can be mounted wherever you tell it to. This is
implemented in the libkosfat library in the addons tree.

### How can I make images for the romdisk file system? Where can I get this genromfs program to make ROMFS image for use with the romdisk file system?

This is a Linux utility (it used to be a Linux-specific file system
but it is well used among embedded developers these days) called
genromfs that does this for you. Here is a URL:

`ftp://sunsite.unc.edu/pub/Linux/system/recovery/genromfs-0.3.tar.gz`

Additionally, I have provided a copy of this program for your convenience.
You will need to edit your environ.sh/tcsh to change the paths if you want
it to work "out of the box". If you are using Cygwin you will also need to
make sure there's a /tmp path mount for bin2o's usage.


## Memory Allocations

### How can I determine how much RAM I have allocated / how much RAM my program is using?

This won't always be accurate, but in the kernel space you can call
malloc_stats(). The output is sent to the kernel console.

### My malloc()'s are failing. Why?  I tried malloc_stats() and it claims I allocated like four billion bytes of RAM!

You probably have a pointer overrun somewhere. Unfortunately these
are rather difficult to debug for now in KOS since it has no MMU
support. I recommend old school debugging (calling malloc_stats() every
so often to see when it bombs and narrowing it down, or perhaps
commenting out blocks until it works again).

You can also edit include/kos/opts.h and enable the memory debugging
defines (anything that starts with KM_ as well as MALLOC_DEBUG). These
will track all memory usage and look for buffer overruns/underruns,
reporting any issues as they happen. It will also show leaked buffers
remaining at the end of the program. There is a similar facility for the
PVR on the DC port (the PVR_KM_* and PVR_MALLOC_DEBUG macros in the same
file).

## Networking

### Does KOS have any kind of TCP/IP stack? Network support at all?

Yes. There is a working driver for the Sega broadband adapter (an
RTL8193C based 10/100 card) and the Lan Adapter on the DC. With regards
to TCP/IP and friends, there is a built-in network stack provided in
the kernel tree. There is also a PPP stack provided in the addons tree
for using the modem.

lwIP used to be provided as an alternate TCP/IP stack for the BBA and
Lan Adapters, however it was removed from the kos-ports tree as it has
been deprecated by the built-in network stack.

### What about the BSD stack?

That seems to be everyone's generic answer to wanting a TCP/IP stack.
Have you actually _looked_ at the BSD stack? Not just glanced through it
at the files and said "oh yep, it's all there!" but really studied what
it would take to do a port? If you're still interested then I welcome
you to attempt a port, but it's not a small task. The biggest problem
would seem to be ripping it out of its BSD home and putting it in a
foreign environment, but with the additions of semaphores and other
thread primitives now in KOS this might not be as hard.


## Building

### I'm having an issue building KOS; it gives me errors about not finding Makefiles or not finding include files.

Make sure you check the paths in environ.sh, and make sure you are
running the script with the 'source' command (see the README). If in
doubt, use 'set' or 'setenv' to find the environment variables. You
should have a KOS_BASE variable. If you don't, then you haven't set up
the and/or sourced environment script correctly in your shell.

### Where is the source to bin2o? All your other stuff has source...

Examine the file more closely. It _is_ the source code =). bin2o is
a bash shell script that runs from a real bash on a *nix, or under the
Cygwin bash. There were some issues with it running under Cygwin
previously but those should now be fixed.


## Debugging

### I work on a DC; I don't have a serial cable so how can I see the debug output?

Use dc-load-ip with your broadband adapter or Lan adapter on the
Dreamcast. If you don't have one of those, you might look into fb_console,
which you can redirect to either the whole frame buffer or a texture
somewhere in VRAM.

### I want to use the serial line but the pesky kernel debug output is getting in my way.

Use INIT_QUIET in your init flags or make sure the debugging output
is not sent to the serial port by using dbgio_dev_select().

### Can I trace through KOS using GDB or similar?

Yes, in theory. It will run just like any other program on your
target hardware, but you will have to be aware that there are interrupts
enabled and they are extremely relevant. They are used in things like
hardware support, threading, and will be used other places later on.
Additionally, you can have things like the console's boot ROM itself
being multitasked in a thread during things like CD access! So be
careful!

There is an effort to make a GDB stub that will work with KOS, but
nothing has fully materialized yet. There's a very simplistic one
for the DC port which lets you place manual breakpoints, but it
doesn't support any sort of tracing.


## kos-ports

### What is kos-ports?

kos-ports is a repository of libraries that have been ported to or
designed for KallistiOS over the years. Most (but not all) of the ported
libraries contain some KOS-specific functionality, such as interfacing
with the PVR code for texture loading or other things of the sort.

Note that kos-ports no longer contains the full source code of each
ported library. This has been done to make updating each port easier (and
thus to actually encourage us to do so). Instead, each port in kos-ports
contains a minimal amount of metadata that is used to build the library
in question from the upstream source (where available). In addition, the
kos-ports system keeps track of dependencies and other such things to
ensure that you don't try to do something like building libpng without
having zlib already built.

### How do I get kos-ports?

If you're using a release version of KOS, I'd recommend grabbing the
corresponding release version of the kos-ports tree from the sourceforge
file release system (or by using a version-specific tag to check it out
from the Git repository). That way, you'll know that each port should
actually work with the version of KOS you're using, in case there have
been updates done to it in the Git repository to support new versions
of KOS.

If you're tracking the Git repository for KOS, then I'd recommend doing
the same for kos-ports. You can grab the latest Git version of kos-ports
by doing the following in a shell:

    git clone git://git.code.sf.net/p/cadcdev/kos-ports

I'd recommend putting the kos-ports tree as a sibling of the main KOS
tree (putting the root kos-ports directory in the same parent directory
as your kos tree).

You can then update the kos-ports tree by doing a "git pull" in the
kos-ports directory (you will have to check each port for updates
yourself).

### How do I build a port? How do I update an installed version of a port? How do I build every port that's available, as I used to do?

To build one port (and any of its dependencies), simply cd to the port's
directory in the kos-ports tree and run the following command:

    make install clean

This command will also work for updating a port you already have
installed (but it will not update dependencies).

If you really want to install every port that is in the tree, there is
a script to do so. Simply cd to the kos-ports directory and run the
following command:

    utils/build-all.sh

I personally don't recommend building every port, but rather building
only those you use, but to each his own, as they say.

### When I try to build a port, I get an error complaining that `curl` is not found. What's up with that?

The curl utility is used by default to download the distribution files
for each port. If you'd rather use wget to do this (or any other similar
utility), edit the `kos-ports/config.mk` file to point to your favorite
utility. There is an example in there for using `wget` already -- just
comment out the `FETCH_CMD = curl --progress-bar -O` line by putting a
`#` in front of it, and uncomment the line below it to use wget instead.

As for why `curl` is the default, well, it's installed by default on OS X,
and is easily available on just about any *nix-like system.
