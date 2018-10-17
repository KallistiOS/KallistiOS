# Sega Dreamcast toolchain with MinGW/MSYS #

This document contains all the instructions to create a fully working
toolchain targeting the **Sega Dreamcast** system under **MinGW/MSYS**.

This document applies only on the original **MinGW/MSYS** environment provided
by [MinGW.org](http://www.mingw.org). For **MinGW-w64/MSYS2** environment, check
the `../mingw-w64` directory.

## Introduction ##

On the **MinGW/MSYS** system, the package manager is the `mingw-get` tool.

In this document, it will be used in graphical mode (GUI).

## Prerequisites ##

Before doing anything, you'll have to install some prerequisites in order to
build the whole toolchain:

- [Git](https://git-scm.com/)
- [Subversion Client](https://sliksvn.com/download/)
- [Python 2](https://www.python.org/downloads/) - (**Python 3** is untested)

**Git** is needed right now, as **Subversion Client** and **Python 2** will be
needed only when building `kos-ports`. But it's better to install these now.

Install all these prerequisites and add them to the `PATH` environment variable.
It should be already done automatically if you use the **Windows** installers.

Check if you can run the tools from the **Windows Command Interpreter** (`cmd`):

- `git --version`
- `svn --version`
- `python --version`

All these commands should produces an output containing the version of each
component.

## Installation of MinGW/MSYS ##

1. Open your browser on [**MinGW.org**](http://www.mingw.org) and download
`mingw-get-setup.exe` from the
[**MinGW** repository](https://osdn.net/projects/mingw/releases/).

2. Run `mingw-get-setup.exe` on **Administrator mode** (starting from
**Microsoft Windows Vista**) then click on the `Install` button. In the
`Installation Directory` text box, input `C:\dcsdk\` or something else. The
`Installation Directory` will be called `${MINGW_ROOT}` later in the document.

3. Leave the other options to its defaults then click on `Continue`. 
The **MinGW/MSYS** installation begins. When the progress bar is full, click on
the `Continue` button.

4. When the **MinGW Installation Manager** shows up, select the following
packages:
 - `mingw32-base`
 - `mingw32-gcc-g++`
 - `msys-base`
 - `msys-patch`
 - `msys-wget`
 - `msys-coreutils-ext`

5. Now we can commit the changes by selecting `Installation` > `Apply Changes`,
   confirm the opening window by hitting the `Apply` button.

The **MinGW/MSYS** base environment is ready, but a patch should be installed
before doing anything. It's the purpose of the section located below.

## Patching the MSYS installation ##

The latest provided **MSYS** package, which is the `1.0.19` in date, contains
**a severe bug in the heap memory management system**. This can stop the `gcc`
compilation in progress with the following unresolvable error: `Couldn't commit
memory for cygwin heap, Win32 error`. 

In order to resolve this bug, you must install the `msys-1.0.dll` from the
[C::B Advanced package](https://sourceforge.net/projects/cbadvanced/files/)
which has been patched to increase the heap internal memory size at its maximum
value (i.e. from `256 MB` to more than `1 GB`). The issue is that package was
removed from the 
[C::B Advanced](https://sourceforge.net/projects/cbadvanced/files/) repository,
as they are now using the modern **MinGW-w64/MSYS2** environment.
Fortunately, the required package was cached in this directory, under the
following name: `msysCORE-1.0.18-1-heap-patch-20140117.7z`.

This patch is just necessary to build the `gcc` cross-compiler. After installing
all the toolchains, you can revert back the replaced `msys-1.0.dll` with its
original version.

To install the **MSYS** heap patch:

1. Fire up at least one time the **MSYS** Shell (it's needed to create some
   necessary file, e.g. the `/etc/fstab` file). You can do that by
   double-clicking the shortcut on your desktop (or alternatively,
   double-clicking on the `${MINGW_ROOT}\msys\1.0\msys.bat` batch file).
2. Close the bash by entering the `exit` command.
3. Move the original `/bin/msys-1.0.dll`
  (i.e. `${MINGW_ROOT}\msys\1.0\bin\msys-1.0.dll`) outside its folder (please 
  don't just rename the file in the `/bin` folder!).
4. Extract the patched `msys-1.0.dll` from 
   `msysCORE-1.0.18-1-heap-patch-20140117.7z` and place it in the `/bin`
   directory (i.e. `${MINGW_ROOT}\msys\1.0\bin\`).

## Checking the `/mingw` mount point ##

This step should be automatic, but in the past we had problems with the `/mingw`
mount point.

Before doing anything, just check if you can access the `/mingw` mount point
with the `cd /mingw` command. If this isn't the case, please check the content
of the `/etc/fstab` file (i.e. `${MINGW_ROOT}\msys\1.0\etc\fstab`).

## Preparing the environment installation ##

1. Open the **MSYS** Shell by double-clicking the shortcut on your desktop (or
   alternatively, double-click on the `${MINGW_ROOT}\msys\1.0\msys.bat` batch 
   file).
   
2. Enter the following to prepare **KallistiOS**:

		mkdir -p /opt/toolchains/dc/
		cd /opt/toolchains/dc/
		git clone https://github.com/KallistiOS/KallistiOS.git kos
		git clone https://github.com/KallistiOS/kos-ports.git

3. Enter the following to prepare **dcload**/**dc-tool** (part of 
   **KallistiOS**):
 
		mkdir -p /opt/toolchains/dc/dcload/
		cd /opt/toolchains/dc/dcload/
		git clone https://github.com/KallistiOS/dcload-serial.git
		git clone https://github.com/KallistiOS/dcload-ip.git

Everything is ready, now it's time to use the make the toolchain.

## Compilation ##

**KallistiOS** provides a complete system that make and install all required
toolchains from source codes: **dc-chain**.

The **dc-chain** system is mainly composed by a `Makefile` doing all the
necessary. Open that file with a text editor and locate the `User configuration`
section. You can tweak some parameters, but usually everything is ready to
work out-of-the-box. For example, it isn't recommended to change the toolchains
program versions. The highest versions confirmed to work with the
**Sega Dreamcast** are always already set in that `Makefile`.

### Making the toolchain ###

To make the toolchains, do the following:

1. Start the **MSYS** Shell if not already done.
2. Navigate to the `dc-chain` directory by entering:

		cd /opt/toolchains/dc/kos/utils/dc-chain/
	
3. Enter the following to download all source packages for all components:

		./download.sh

4. Enter the following to unpack all source packages.

		./unpack.sh

5. Enter the following to launch the process:

		make

Now it's time to take a coffee as this process is really long: several hours
will be needed to make the full toolchain!

### Making the GNU Debugger (gdb) ###

If you want to install the **GNU Debugger** (`gdb`), just enter:

	make gdb

This will install `sh-elf-gdb` and can be used to debug programs through
`dc-load/dc-tool`.

### Removing all useless files ###

After everything is done, you can cleanup all temporary files by entering:

	./cleanup.sh

## Removing the MSYS heap patch ##

After your toolchain is ready, please don't forget to replace the patched
`msys-1.0.dll` with its original version (i.e. the patched file `SHA-1`
is `4f7c8eb2d061cdf4d256df624f260d0e58043072`).

But before replacing the file, close the running **MSYS** Shell by entering
the `exit` command!

## Fixing up Newlib for SH-4 ##

The `ln` command in the **MinGW/MSYS** environment is not effective, as
symbolic links are not well managed under this environment.
That's why you need to manually fix up **SH-4** `newlib` when updating your
toolchain (i.e. rebuilding it) and/or updating **KallistiOS**.

This is the purpose of the provided `./packages/fixup-sh4-newlib.sh` script.

Before executing it, just edit it to be sure if the `$toolchains_base` variable
is correctly set. Then execute it by just entering `./fixup-sh4-newlib.sh`.

## About static binaries ##

If you are making the whole toolchains on **MinGW/MSYS**, if your host is
**Microsoft Windows XP**, all the produced binaries in the toolchain will
be statically linked, i.e. they can be run **outside** the **MSYS** environment:
For example, if you just double-click on any `sh-elf` binary (e.g. `sh-elf-gcc`)
the program should run.

If you are compiling the toolchains on a modern host like 
**Microsoft Windows 7**, the binaries will be dynamically linked, so if you try 
to double-click on any `sh-elf` binary, you will have the following message:

	The file libintl-8.dll is missing from your computer.

Of course, this is not relevant if you are working directly from the 
**MinGW/MSYS** environment, but this point can be notable if you want to use
these toolchains from an IDE (like **Code::Blocks**), i.e. **outside** the
**MinGW/MSYS** environment.

**Note:** This apply only on **MinGW/MSYS** environment with the heap patch
applied!

## Next steps ##

After following this guide, the toolchain should be ready.

Now it's time to compile **KallistiOS**.

Please read the `/opt/toolchains/dc/kos/doc/README` file to learn the next
steps.
