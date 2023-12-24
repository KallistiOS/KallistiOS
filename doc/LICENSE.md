# Licensing

## KallistiOS
KallistiOS is licensed under the terms of the "new" BSD license. We are 
calling our version of this license with the KOS copyright notice the **KOS
License**. For all files that are copyrighted by the authors of KallistiOS
(Megan Potter, etc), or for any file which does not otherwise specify
(and does not come from a third-party package like newlib), you can assume
that this text is attached to them:

> This program is free software; you can redistribute it and/or modify
> it under the terms of the KOS License.
>
> This program is distributed in the hope that it will be useful,
> but WITHOUT ANY WARRANY; without even the implied warranty of
> MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
> KOS License (README.KOS) for more details.
>
> You should have received a copy of the KOS License along with this
> program; if not, please visit Cryptic Allusion Game Dev at:
>
>   http://gamedev.allusion.net/

Please read [README.KOS](README.KOS) for the full details.

## Included BSD Code

The above information applies to the portions of KallistiOS that are copyrighted
by the authors. BSD code that is included with KallistiOS is licensed under the
BSD license -- please reference README.BSD. This BSD license is nearly identical
to the KOS license, but ours has removed the "advertising clause." 

## Included lwIP Code

KOS also optionally contains code from the "lwIP" TCP/IP stack,
copyright 2001 Swedish Institute of Computer Science. This code is
licensed under a license basically identical to the KOS license, but
with their name. You can find the license in any of their files.

## Included JPEG Code

This software optionally is based in part on the work of the Independent
JPEG Group (see kos-ports/libjpeg/README).

## Included GPL/LGPL Code

Some of the software included with the KOS distribution is not copyrighted
by us, nor is it part of the codebase of KOS. Pretty much all of that
code is contained in the "utils" directory, and will shortly be contained
as well (or perhaps only) in the "addons"
directory. 
Some of the software included with KallistiOS is not copyrighted by us, nor
is it part of the codebase of KallistiOS. Most of that code will be found
within the "utils" directory and the "addons" directory. Licenses on these
pieces vary; please see the source of the program in question to be sure.
In particular, genromfs, dialog, and the XingMP3 library (in 
kos-ports/libmp3/xingmp3) are covered under the GPL, and MPGLIB
(in kos-ports/libmp3/mpglib) is covered under the LGPL. I don't yell
lightly, so pay attention to this:

**IF YOU USE THE XINGMP3 LIBRARY IN YOUR PROGRAM, YOUR PROGRAM
AUTOMATICALLY BECOMES RESTRICTED BY THE GPL LICENSE. IF YOU USE THE
XINGMP3 LIBRARY IN YOUR PROGRAM, YOU MAY NOT DISTRIBUTE IT WITHOUT
SOURCE CODE. END.**

Just in case you missed that or don't like caps, you may not include the
Xing MP3 library in your program (this currently means linking -lmp3
from the KOS kos-ports tree) without making your program GPL'd, or
something less restrictive (e.g., BSD license) but distributing it by
the terms of the GPL. If you replace the MP3 engine, however, you _can_
do this, as all of our libmp3 code is covered under the BSD license.

## More licensing information
If there is ever a question or you want to ask permission to do something
else with it, please contact the authors and we'll see how we can accommodate
you.

The Free Software Foundation has a very good source of information on
the various licenses. Please see this page if you have license compatabality
questions:

http://www.gnu.org/philosophy/license-list.html

