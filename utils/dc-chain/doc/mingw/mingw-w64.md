# Sega Dreamcast Toolchains Maker (`dc-chain`) with MinGW-w64/MSYS2 #

This document contains all the instructions to create a fully working
toolchains targeting the **Sega Dreamcast** system under **MinGW-w64/MSYS2**.

This document applies only on the newer **MinGW-w64/MSYS2** environment provided
by [MinGW-w64.org](https://mingw-w64.org/). For the legacy **MinGW/MSYS**
environment, check the `mingw.md` file.

## Introduction ##

On **MinGW-w64/MSYS2** system, the package manager is the `pacman` tool.
All preliminary operations will be done through this tool.

The **MinGW-w64/MSYS2** environment exists in two flavors:

- **32-bit**: `i686`
- **64-bit**: `x86_64`

This document was written when using the `i686` version, so if you are using
the `x86_64` version, you should replace all `i686` keywords in the packages
name with `x86_64`.

We don't know if the `x86_64` version is stable in this context. For your 
information, in the past some problems happened with the `x86_64` **Cygwin** 
version. Feel free to try it out if you want.

Please note also, the **Microsoft Windows XP** support was dropped on this
environment. If you need support for this old OS, you need to use the 
**MinGW/MSYS** environment instead.

## Installation of MinGW-w64/MSYS2 ##

1. Open your browser on [**MinGW-w64.org**](https://mingw-w64.org/) and choose 
   the [download **MSYS2** distribution](http://www.msys2.org/). Download the 
   installer file `msys2-${arch}-${date}.exe` (e.g.`msys2-i686-20180531.exe`)
   from the [**MSYS2** repository](http://www.msys2.org/).

2. Run `msys2-${arch}-${date}.exe` on **Administrator mode** (starting from
   **Microsoft Windows Vista**) then click on the `Next` button. In the
   `Installation Directory` text box, input `C:\dcsdk\` or something else. The
   `Installation Directory` will be called `${MINGW_ROOT}` later in the document.
   Click on the `Next` button to continue.

3. Click on the `Next` button again, then click on the `Finish` button to start
   the **MSYS2 Shell**.

The **MinGW-w64/MSYS2** base environment is now ready. It's time to setup the 
whole environment to build the toolchains.

### Update of your local installation ###

The first thing to do after installing is to update your local installation:

	pacman -Syuu

At the end of the process, a similar message to this one should be appear:

	warning: terminate MSYS2 without returning to shell and check for updates again
	warning: for example close your terminal window instead of calling exit

It means only the `pacman` runtime has been updated. Close the terminal as 
requested. Restart the **MSYS2 Shell** and run the same command again:

	pacman -Syuu

This should update all the packages of the **MinGW-w64/MSYS2** environment.

### Installation of required packages ###

The packages below need to be installed to build the toolchains, so open the
**MSYS2 Shell** and input:

	pacman -Sy --needed base-devel mingw-w64-i686-toolchains mingw-w64-i686-libpng mingw-w64-i686-libjpeg mingw-w64-i686-libelf

### Installation of additional packages ###

Additional packages are needed:

	pacman -Sy git subversion python2

**Git** is needed right now, as **Subversion Client** and **Python 2** will be
needed only when building `kos-ports`. But it's better to install these now.

By the way you can check the installation success by entering something like
`git --version`. This should returns something like `git version X.Y.Z`.

## Preparing the environment installation ##

1. Open the **MSYS2 Shell** by double-clicking the shortcut on your start menu 
   (or alternatively, double-click on the `${MINGW_ROOT}\mingw${arch}.exe` file,
   e.g. `${MINGW_ROOT}\mingw32.exe`).
   
2. Enter the following to prepare **KallistiOS**:

		mkdir -p /opt/toolchains/dc/
		cd /opt/toolchains/dc/
		git clone git://git.code.sf.net/p/cadcdev/kallistios kos
		git clone git://git.code.sf.net/p/cadcdev/kos-ports

Everything is ready, now it's time to make the toolchains.

## Compilation ##

**KallistiOS** provides a complete system that make and install all required
toolchains from source codes: **dc-chain**.

The **dc-chain** system is mainly composed by a `Makefile` doing all the
necessary. In order to work, you'll need to provide a `config.mk` file. Read
the main `README.md` file at the root for more information.

### Making the toolchains ###

To make the toolchains, do the following:

1. Start the **MSYS2 Shell** if not already done.
2. Navigate to the `dc-chain` directory by entering:

		cd /opt/toolchains/dc/kos/utils/dc-chain/
	
3. Enter the following to start downloading and building toolchain:

		make

Now it's time to take a coffee as this process is really long: several hours
will be needed to make the full toolchains!

### Making the GNU Debugger (gdb) ###

If you want to install the **GNU Debugger** (`gdb`), just enter:

	make gdb

This will install `sh-elf-gdb` and can be used to debug programs through
`dc-load/dc-tool`.

### Removing all useless files ###

After everything is done, you can cleanup all temporary files by entering:

	make clean

## Fixing up Newlib for SH-4 ##

The `ln` command in the **MinGW-w64/MSYS2** environment is not effective, as
symbolic links are not well managed under this environment.
That's why you need to manually fix up **SH-4** `newlib` when updating your
toolchains (i.e. rebuilding it) and/or updating **KallistiOS**.

This is the purpose of the provided `fixup-sh4-newlib.sh` script.

Before executing it, just edit it to be sure if the `$toolchains_base` variable
is correctly set. Then execute it by just entering:

	./packages/fixup-sh4-newlib.sh

## Next steps ##

After following this guide, the toolchains should be ready.

Now it's time to compile **KallistiOS**.

You may consult the `README` file from KallistiOS now.
