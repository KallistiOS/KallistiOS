KallistiOS ##version##
README
Copyright (C) 2000-2003 Megan Potter
Copyright (C) 2012-2014 Lawrence Sebald

OVERVIEW
--------
KallistiOS is a pseudo-real-time operating system for gaming consoles,
licensed under the terms of the *new* BSD license (the one without the
advertising clause). It is currently available for the Dreamcast*, but
ports previously existed for the Gameboy Advance*, PlayStation 2*, and
Intel* ia32 platforms, although none were particularly complete.

Depending on whether you wish to see the fnords, you can pronounce it
"kallisti-o's" (like a cereal) or "kallisti o s" (like an operating
system). We like the former since it's silly to be so serious most of the
time =). "Kallisti" means "to the fairest" in Greek. This was the word (so
the story goes) that was inscribed on the golden apple that Eris threw
into the banquet of the gods to start the Trojan war. This somehow landed
her the title of matriarch of a silly religion called Discordianism, which
is what the name pays homage to. If you want a short abbreviation, you can
also refer to it as "KOS", which can be pronounced "k-os" (chaos) =).

Note that this name is _not_ to be confused or associated with either the
professional development company Kalisto Software* or the cracking group
"Kalisto".

Now that that is cleared up... =)

KallistiOS is a modular monolithic kernel, like Linux or FreeBSD. This
means that there is a kernel library which you link with your own code.
This library (libkallisti.a) contains the whole core OS functionality. You
may also enable dynamically loaded modules which will link into your
program at runtime and add extra functionality like new VFS modules,
new peripheral device support, image loader plugins, etc. The possibilities
are limited only by your patience with adding enough exports to support
the modules. ;)

Note that this is different from previous versions of KOS which had an
actual "OS mode". The new dynamic loader system is architected somewhat
like the AmigaOS's library system but it makes no pretenses whatsoever
to being an actual process management system.

What KallistiOS is primarily:
- Processor manager (threads, MMU, store queues, DMA, exceptions, etc)
- Pseudo-POSIX/ANSI layer (printf, files, threads, etc)
- HAL (hardware abstraction layer)

What KallistiOS provides optionally:
- Dynamic module loading
- Ported addon modules, through kos-ports

What KallistiOS is NOT:
- UNIX (or even compatible)
- Linux/BSD (those are their own ports! =)
- Memory protection; programs can overwrite each other at will#
- Pretty much anything else you can think of that's not in the above list

(# There is an MMU module for the DC port, but nothing really uses it at
   this point.)

If you're looking for all these features in a solid proven kernel, then
look no further than Linux or NetBSD. On the other hand, if you want a
simple kernel that you can grok in a few hours and use simply, use KallistiOS!

As an aside, if you are interested in a skeleton version of KOS that uses
the MMU and memory protection, a real process model, etc, check out
KOS-MMU. It's not being worked on any more but it has some interesting
code in it and is a clean example of a simple OS. Most of the interesting
code from KOS-MMU was merged into KOS 1.1.x, 1.2.x, and 2.0.0.


LICENSING
---------
Please read the file "LICENSE" for more information about this subject.
This program is now distributed under a similar set of terms as earlier
ones (libdream), and has changed since KallistiOS 0.7 (GPL). Even so, some
of the software distributed with KOS (newlib math routines, Xing MP3
engine, modplug, etc) are under different licenses. Please read "LICENSE"
if in doubt!

This software is optionally based in part on the work of the Independent JPEG
Group (see addons/libjpeg/README).

I have also added a somewhat informative "licensing" section to the FAQ.

If in doubt, please contact us. If you want to use KOS in a project, we'd
love to hear from you anyway!


PREREQUISITES
-------------
Building KallistiOS from source entirely requires two things:
- GNU Binutils / GCC, cross-compile to the platform of your choice; for
  DC this is a compiler targeted for "sh-elf", as well as (optionally)
  a compiler targeted for "arm-eabi". See below about version notes.
- A working compiler toolchain that can be used to cross-compile the
  requisite target toolchain (so GCC or Clang based, basically) and
  GNU Make for your host platform

If building KallistiOS to target NAOMI, additionally you will need to
install libelf (and the development package, if applicable) for your
host environment (or modify the utils/naomibintool Makefile to not
use libelf and to define NO_LIBELF for building that tool).

For the Dreamcast version, a Makefile is provided to build the requisite
GCC, Binutils, and Newlib versions in the utils/dc-chain directory.

If you don't have any of the above, please see one of the following sites
for some more instructions (and perhaps a build script that may help you):

  http://gamedev.allusion.net/softprj/kos/setup.php
  http://dcemulation.org/

On the Dreamcast, the 4.x series of GCC works pretty well. Look at the patches
provided in utils/dc-chain/patches to see what versions have been tested and
are known to work. In addition, some people still use earlier versions of GCC,
such as 3.4.6 and 3.0.4. There are often strange bugs with GCC for SuperH, so
if you deviate from these suggested versions, you're on your own (for instance,
pretty much the entire 3.1.x, 3.2.x, and 3.3.x branches of GCC were broken for
SuperH). Basically, if you use a newer version of GCC and find that it works
perfectly, great -- let us know and give us the patches needed to make it work.
However, if you find that it doesn't work, you're on your own to try to figure
out why.

If you wish to work with the sound processor on the Dreamcast, you'll need an
ARM toolchain as well. ARM support is maintained pretty well in GCC, so most
versions should "just work". The ARM processor in the sound unit is fairly old
and doesn't support Thumb mode at all, so keep that in mind when building GCC.
You do not necessarily need an ARM version of GCC to build KOS for the DC, as
precompiled binaries are provided of the necessary code. As long as you don't
intend to modify the sound driver, you don't need the ARM tools.


BUILDING
--------
Building KOS itself is actually quite simple, especially if you don't want
to add new sections for yourself. Make a copy of doc/environ.sh.sample and
call it environ.sh in your main KOS dir. Edit this file to match your
compiler setup and any other preferences you may have. Also make sure
you set the architecture that you want to build for here.

Note that unlike previous versions, there is only one environ.sh file no
matter what platform you use. The common pieces and most of the platform
specific pieces have been pulled out so that you don't have to deal with
them anymore.

After you have a working environ script, run it with 'source environ.sh'
and run your make program. For BSD people this may be 'gmake', otherwise
it will probably be just 'make'. It should be done after a few minutes. If
something goes wrong, please check the FAQ; if that fails, email one of us
and we'll see what we can do. I recommend putting the 'source' line above
into your shell's .rc/.login.

This process should work (and has been tested) under Linux, BSD, Cygwin and
MinGW/MSYS. It is very doubtful that it will work in any non-*nix environment.


VERSION CODES
-------------

All KOS versions are composed of three sections: major, minor, micro. Major
revisions are generally something that changes the OS fundamentally, or
when we wait several years between releases =). The minor version number
is used to denote a development series. This gets incremented basically
whenever we feel that it is relevant to do so. Finally, the micro version
denotes a sequence in the development series. 1.1.0 would be the first
snapshot of the 1.1 development series.

And as always, if you want to be on the bleeding edge, use the Git
repository hosted at SourceForge.


HACKING
-------
If you are planning on doing your own hacking and potentially submitting
patches for me to include, then I'd very much appreciate it if you would do
this against the Git repository rather than a release tree. The general
workflow to do this is to add/commit the changes to your local git repo
and then send a patch for the revisions in question like so (where n is the
number of commits you have made):

git format-patch -n

This will write a number of .patch files to the current directory for the n
most recent commits you have made. You can then upload those patches to the
SourceForge patch tracker to get them to us. You should ensure that your git
setup is relatively sane and that you have your name/email address filled in
properly, or I will probably reject your patches. You should also provide
useful commit messages, with a summary on the first line to make it easier
on me later.

In addition, for those used to working with pull requests on GitHub, I accept
those as well. I assume you know how to make things work that way if you wish
to go that route. The mirror of the repository at GitHub is currently at the
following url:
        https://github.com/KallistiOS/KallistiOS

I don't guarantee that any patch will get included (especially if your patch
contains a lot of reformatting, sloppy coding, things at cross-purposes with
the primary thrust of KOS, etc), but I will make an effort to at least
examine and comment on any submitted patches.

Code should be relatively well formatted, especially in the core library/kernel
portions. There are tools out there to help you reformat your code so that it
will match the rest of KOS a lot more nicely. One such tool is Artistic Style
(http://astyle.sourceforge.net/). Here's a nice set of flags to feed to the
astyle program to make your code match up nicely with the rest of KOS:
  astyle --style=attach -s4 -c -S -w -y -f -U -p -m0 -xn -xc -xl -xk -C -N -Y \
    -k3 -xL -xM -xQ -xP0 [your files go here]
I don't guarantee that this produces the exact same formatting as the rest of
the code, but it should be pretty much correct (and I won't reject your patch
for styling if you use this set of flags to astyle on it). You might also want
to take a look at the coding_style.txt file (in the same directory as this
README in the KOS tree) for some rationale for most of that formatting stuff.
Note that some of that has changed over time (mainly when the maintainer of KOS
changes), but it is still mostly right. ;-)


OS MODE IS GONE FOR GOOD
------------------------

All the interesting and maintainable pieces of the KOS 1.0.x "os mode" have
been fully integrated back into the mainline KOS 2.0.0 tree. It is now used
for dynamic module loading and does not include process support, etc. This
is unlikely to be revived again in the future.

If you want this kind of functionality but still want an MMU-less
environment for speed, I recommend porting uCLinux to your platform.


PLATFORM NOTES
--------------
Dreamcast*

- MPGLIB (LGPL MP3 library, which is also faster) is included, but does
  not work. If you want to screw with it yourself, check the libmp3 Makefile
  and look at sndmp3_mpglib.c in that dir.
- libs3m is present but doesn't really work. It needs serious porting work.

There are no other supported platforms after KOS 2.0.0. They were all pretty
much broken and unmaintained anyway, so they were removed shortly after the
release of KOS 2.0.0.


END
---
That's where we leave off here. Now that you have a build and ready copy,
you're ready to get to the more fun things. Please read the other documents
in the "doc" directory for more information. Also check the FAQ if there
is something you were wondering.

Also, please take note of our new web URL (below). We have moved hosting
to SourceForge for all console-related things.

					Cryptic Allusion
					http://gamedev.allusion.net/


* "Sega", "Dreamcast", and NAOMI are registered trademarks of Sega Corporation.
  "Nintendo" and "Gameboy Advance" are registered trademarks of Nintendo of America
  Kalisto Software is a registered trademark of Kalisto Software, Inc.
  "PlayStation" is a registered trademark of Sony Computer Entertainment America
  "Intel" is a registered trademark of Intel, Inc.
  Any other trademarks are trademarks of their respective owners.
