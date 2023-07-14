# Sega Dreamcast Toolchains Maker (`dc-chain`) with MinGW/MSYS #

This document contains all the instructions to create a fully working
toolchains targeting the **Sega Dreamcast** system under **MinGW/MSYS**.

This document applies only on the legacy **MinGW/MSYS** environment provided
by [MinGW.org](http://www.mingw.org). For **MinGW-w64/MSYS2** environment, read
the `mingw-w64.md` file.

## Introduction ##

On the **MinGW/MSYS** system, the package manager is the `mingw-get` tool.

In this document, it will be used in graphical mode (GUI).

## Prerequisites ##

Before doing anything, you'll have to install some prerequisites in order to
build the whole toolchains:

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
compilation in progress with the following unresolvable error:
`Couldn't commit memory for cygwin heap, Win32 error`.

In order to resolve this bug, you must install the `msys-1.0.dll` from the
[C::B Advanced package](https://sourceforge.net/projects/cbadvanced/files/)
which has been patched to increase the heap internal memory size at its maximum
value (i.e. from `256 MB` to more than `1 GB`). The issue is that package was
removed from the 
[C::B Advanced](https://sourceforge.net/projects/cbadvanced/files/) repository,
as they are now using the modern **MinGW-w64/MSYS2** environment.
Fortunately, the required package was cached in this directory, under the
following name: `msysCORE-1.0.18-1-heap-patch-20140117.7z`.

This patch is just necessary to build the `gcc` cross-compiler. After building
all the toolchains, you can revert back the replaced `msys-1.0.dll` with its
original version.

To install the **MSYS** heap patch:

1. Fire up at least one time the **MSYS Shell** (it's needed to create some
   necessary file, e.g. the `/etc/fstab` file). You can do that by
   double-clicking the shortcut on your desktop (or alternatively,
   double-clicking on the `${MINGW_ROOT}\msys\1.0\msys.bat` batch file).
   
2. Close `bash` by entering the `exit` command.

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

1. Open the **MSYS Shell** by double-clicking the shortcut on your desktop (or
   alternatively, double-click on the `${MINGW_ROOT}\msys\1.0\msys.bat` batch 
   file).
   
2. Enter the following to prepare **KallistiOS**:
   ```
	mkdir -p /opt/toolchains/dc/
	cd /opt/toolchains/dc/
	git clone git://git.code.sf.net/p/cadcdev/kallistios kos
	git clone git://git.code.sf.net/p/cadcdev/kos-ports
   ```
Everything is ready, now it's time to make the toolchains.

## About making toolchains static binaries ##

By default, all the binaries of the toolchains (e.g. `sh-elf-gcc`...) are
dynamically linked, and that's the way that meant to be. The drawback is,
if you want to use the toolchains outside the **MinGW/MSYS** environment and
the binaries are dynamically linked, you'll have some error messages like:
```
The file libintl-8.dll is missing from your computer.
```
This happens if you just double-click on any `sh-elf` binaries (e.g.
`sh-elf-gcc`), even with `arm-eabi` binaries.

In the **MinGW/MSYS** environment, you will have the possibility to make the
toolchains binaries statically linked, i.e. they can be run **outside** the
**MinGW/MSYS** environment:

1. Open the **dc-chain** `Makefile` with a text editor.

2. Locate the `STANDALONE_BINARY` flag and set it to `1`.

3. Build the toolchains as usual with `make`.

Now, if you just double-click on any `sh-elf` binary (e.g. `sh-elf-gcc`)
the program should run properly.

Of course, this is not relevant if you are working directly from the 
**MinGW/MSYS** environment (i.e. from the **MSYS Shell**), but this point can
be notable if you want to use these toolchains from an IDE (like
**Code::Blocks**, **CodeLite**...), i.e. **outside** the **MinGW/MSYS**
environment.

Basically, if you just plan to use the **MinGW/MSYS** environment through the
**MSYS Shell**, just let the `STANDALONE_BINARY` flag undefined.

**Note:** This flag is here to build [DreamSDK](https://dreamsdk.org) as well.

## Compilation ##

**KallistiOS** provides a complete system that make and install all required
toolchains from source codes: **dc-chain**.

The **dc-chain** system is mainly composed by a `Makefile` doing all the
necessary. In order to work, you'll need to provide a `config.mk` file. Read
the main `README.md` file at the root for more information.

### Making the toolchains ###

To make the toolchains, do the following:

1. Start the **MSYS Shell** if not already done.
2. Navigate to the `dc-chain` directory by entering:
   ```
   cd /opt/toolchains/dc/kos/utils/dc-chain/
   ```
3. Enter the following to start downloading and building toolchain:
   ```
	make
   ```
Now it's time to take a coffee as this process is really long: several hours
will be needed to make the full toolchains!

### Making the GNU Debugger (gdb) ###

If you want to install the **GNU Debugger** (`gdb`), just enter:
```
make gdb
```
This will install `sh-elf-gdb` and can be used to debug programs through
`dc-load/dc-tool`.

### Removing all useless files ###

After everything is done, you can cleanup all temporary files by entering:
```
make clean
```
## Removing the MSYS heap patch ##

After your toolchains is ready, please don't forget to replace the patched
`msys-1.0.dll` with its original version (i.e. the patched file `SHA-1`
is `4f7c8eb2d061cdf4d256df624f260d0e58043072`).

But before replacing the file, close the running **MSYS Shell** by entering
the `exit` command!

## Fixing up Newlib for SH-4 ##

The `ln` command in the **MinGW/MSYS** environment is not effective, as
symbolic links are not well managed under this environment.
That's why you need to manually fix up **SH-4** `newlib` when updating your
toolchains (i.e. rebuilding it) and/or updating **KallistiOS**.

This is the purpose of the provided `fixup-sh4-newlib.sh` script.

Before executing it, just edit it to be sure if the `$toolchains_base` variable
is correctly set. Then execute it by just entering:
```
./packages/fixup-sh4-newlib.sh
```
## Next steps ##

After following this guide, the toolchains should be ready.

Now it's time to compile **KallistiOS**.

You may consult the `README` file from KallistiOS now.
