# Overview
KallistiOS[^1] is a development library and unikernel operating system for
the Sega Dreamcast[^1] licensed under a [permissive software license](LICENSE.md).

KallistiOS is a modular monolithic kernel, like Linux or FreeBSD. This
means that there is a kernel library which you link with your own code.
This library (`libkallisti.a`) contains the whole core OS functionality. You
may also enable dynamically loaded modules which will link into your
program at runtime and add extra functionality like new VFS modules,
new peripheral device support, image loader plugins, etc. The possibilities
are limited only by your patience with adding enough exports to support
the modules. :wink:

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
- Linux/BSD (those are their own ports!) :smiley:
- Memory protection; programs can overwrite each other at will
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

# Licensing
Please read [LICENSE.md](LICENSE.md) for more information about this subject.
The [FAQ](FAQ.md) also contains an informative "licensing" section.

If in doubt, please contact us. If you want to use KOS in a project, we'd
love to hear from you anyway!

# Prerequisites
Building KallistiOS from source entirely requires two things:
- GNU Binutils / GCC, to cross-compile to your target platform; for
  DC this is a compiler targeted for `sh-elf`. An `arm-eabi` toolchain
  can optionally be used. 
- A working host compiler toolchain that can be used to compile this
  target cross-compiler toolchain (GCC or Clang) and GNU Make

A toolchain-building script is included to build the cross-compiler. 
See the [dc-chain documentation](../utils/dc-chain/README.md) for more
information on installing the toolchain.

If building KallistiOS to target NAOMI, additionally you will need to
install libelf (and the development package, if applicable) for your
host environment (or modify the utils/naomibintool Makefile to not
use libelf and to define NO_LIBELF for building that tool).

# kos-ports
[kos-ports](https://github.com/KallistiOS/kos-ports) is a package manager and repository of various useful libraries
that have been ported to KOS.

# Addons
[Addons](../addons/readme.md) are additional libraries that are kept as optional but
were written specifically for and maintained alongside KOS unlike kos-ports.

# Utilities
[Utilities](../utils/readme.md) contains various PC-side tools used to assist development.

# Examples
[Examples](../examples/dreamcast/readme.md) on using KOS, it's addons, and various kos-ports libs.

# Building
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
something goes wrong, please check the [FAQ](FAQ.md); if that fails, you may
seek support though [several options](../README.md#resources). I recommend
putting the 'source' line above into your shell's .rc/.login.

This process should work (and has been tested) under Linux, BSD, Cygwin and
MinGW/MSYS. It is very doubtful that it will work in any non-*nix environment.

# Version Codes
All KOS versions are composed of three sections: major, minor, micro. Major
revisions are generally something that changes the OS fundamentally, or
when we wait several years between releases :smiley:. The minor version number
is used to denote a development series. This gets incremented basically
whenever we feel that it is relevant to do so. Finally, the micro version
denotes a sequence in the development series. 1.1.0 would be the first
snapshot of the 1.1 development series.

And as always, if you want to be on the bleeding edge, use the Git
repository hosted at SourceForge.

# Hacking
If you are planning on doing your own hacking and potentially submitting
patches for me to include, then I'd very much appreciate it if you would do
this against the Git repository rather than a release tree. The general
workflow to do this is to add/commit the changes to your local git repo
and then send a patch for the revisions in question like so (where n is the
number of commits you have made):

`git format-patch -n`

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
`https://github.com/KallistiOS/KallistiOS`

I don't guarantee that any patch will get included (especially if your patch
contains a lot of reformatting, sloppy coding, things at cross-purposes with
the primary thrust of KOS, etc), but I will make an effort to at least
examine and comment on any submitted patches.

Code should be relatively well formatted, especially in the core library/kernel
portions. There are tools out there to help you reformat your code so that it
will match the rest of KOS a lot more nicely. One such tool is Artistic Style
(http://astyle.sourceforge.net/). Here's a nice set of flags to feed to the
astyle program to make your code match up nicely with the rest of KOS:
```
astyle --style=attach -s4 -c -S -w -y -f -U -p -m0 -xn -xc -xl -xk -C -N -Y \
    -k3 -xL -xM -xQ -xP0 [your files go here]
```

I don't guarantee that this produces the exact same formatting as the rest of
the code, but it should be pretty much correct (and I won't reject your patch
for styling if you use this set of flags to astyle on it). You might also want
to take a look at the coding_style.txt file (in the same directory as this
README in the KOS tree) for some rationale for most of that formatting stuff.
Note that some of that has changed over time (mainly when the maintainer of KOS
changes), but it is still mostly right. :wink:

## History and Lore
KallistiOS began its life as a hobby operating system written by Megan Potter 
in the late 90s, not for the Sega Dreamcast, but for i386 PCs. Megan later
became interested in hacking her new Sega Dreamcast console, and along with
Jordan DeLong and Mike "Tursi" Brent, began work on the **libdream** library
by adapting the work pioneered by Marcus Comstedt. _Stars_ and _Ghetto Pong_
then become the first publicly released Dreamcast homebrew demo and game. 

Later, they began discussing the idea of writing a real embedded operating
system for Dreamcast, and KallistiOS was reborn. None of the original code
survives in the Dreamcast version, but the concepts and name still do. 

Depending on whether you wish to see the [fnords](https://en.wikipedia.org/wiki/Fnord),
you can pronounce the name _Kallisti-O's_ (like a cereal) or _Kallisti O. S._ (like an operating
system). We like the former since it's silly to be so serious most of the
time :smiley:. _Kallisti_ means "to the fairest" in Greek. In Greek mythology, this
word was inscribed upon a golden apple and thrown into the banquet of the gods by
Eris, the goddess of discord and strife, in retaliation for being disinvited.

The goddesses Hera, Athena, and Aphrodite then fought over who was deserving
of the title _fairest_ and thus possession of the apple. Zeus, wishing to stay
out of the matter, asked Paris, prince of Troy, to arbitrate the disagreement.
Each goddess attempted to bribe Paris with a reward for choosing her. Paris chose
Aphrodite, which won him the love of the most beautiful woman on Earth, who was
Helen, Queen of Sparta. The Greeks sailed to Troy to retrieve her, starting the
10 year Trojan War.

This landed Eris the title of matriarch of a silly religion called _Discordianism_,
to which the KallistiOS name pays homage. If you want a short abbreviation, you
can also refer to it as _KOS_, which can be pronounced _k-oss_ (like chaos) :smiley:.

If you think that is silly, well, the original suggestion for the name of
the operating system was _HuevOS_, and the mascot was an egg wearing a sombrero
and riding a surfboard. 😃 Eventually, though, _KallistiOS_ was chosen due to the
authors' interest in Discordianism.

The development of KallistiOS, credited to Megan Potter's _Cryptic Allusion_
organization, accelerated through the end of 2000 and into 2001, and by May 2001
Dreamcast dev pioneers _Cryptic Allusion_, _Ganksoft_, _Moving Target Software
Design_, and _AndrewK/Napalm_ collaborated to design, produce, and distribute
a collection of homebrew games and demos called _DC Tonic_ at the E3 Electronics
Entertainment Exposition. The demo disc was a hit and both the demos and the
KallistiOS software caught the attention and praise of Sega themselves. 

Ports of KallistiOS once existed for the existed for the Gameboy Advance[^2],
PlayStation 2[^2], and Intel[^2] ia32 platforms, although none were particularly
complete. At this time, only the Sega Dreamcast portion remains, but the project
architecture still exists to add additional consoles at later time.


# Platform Notes

There are no other supported platforms after KOS 2.0.0. They were all pretty
much broken and unmaintained anyway, so they were removed shortly after the
release of KOS 2.0.0.

# End
Please read the other documents in the "doc" directory for more information.
Also check the FAQ if there is something you were wondering.

Also, please take note of our new web URL (below). We have moved hosting
to SourceForge for all console-related things.

`https://sourceforge.net/projects/cadcdev/`

[^1]: Note that this name is _not_ to be confused or associated with either the
  professional development company Kalisto Software* or the cracking group
  "Kalisto".

[^2]: "Sega", "Dreamcast", and NAOMI are registered trademarks of Sega Corporation.
  "Nintendo" and "Gameboy Advance" are registered trademarks of Nintendo of America
  Kalisto Software is a registered trademark of Kalisto Software, Inc.
  "PlayStation" is a registered trademark of Sony Computer Entertainment America
  "Intel" is a registered trademark of Intel, Inc.
  Any other trademarks are trademarks of their respective owners.
