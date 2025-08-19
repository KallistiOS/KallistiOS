# Sega Dreamcast Toolchains Maker (`dc-chain`) with MinGW/MSYS #

This document contains all the instructions to create a fully working
toolchain targeting the **Sega Dreamcast** system under **MinGW/MSYS**.

This document applies only on the legacy **MinGW/MSYS** environment provided
by [MinGW - Minimalist GNU for Windows](https://sourceforge.net/projects/mingw/).
For **MinGW-w64/MSYS2** environment, read the `mingw-w64.md` file.

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

All these commands should produce an output containing the version of each
component.

## Installation of MinGW/MSYS ##

1. Open your browser to [**MinGW - Minimalist GNU for Windows**](https://sourceforge.net/projects/mingw/)
and download `mingw-get-setup.exe` from the
[**MinGW** repository](https://osdn.net/projects/mingw/releases/).

2. Run `mingw-get-setup.exe` in **Administrator mode** (if using **Microsoft
Windows Vista** or later) then click on the `Install` button. In the
`Installation Directory` text box, input `C:\dcsdk\`. The `Installation
Directory` will be called `${MINGW_ROOT}` later in the document.

3. Leave the other options to its defaults, then click on `Continue`. 
The **MinGW/MSYS** installation will begin. When the progress bar is full, click
on the `Continue` button.

4. When the **MinGW Installation Manager** appears, select the following
packages:
 - `mingw32-base`
 - `mingw32-gcc-g++`
 - `msys-base`
 - `msys-patch`
 - `msys-wget`
 - `msys-coreutils-ext`

5. Now we can commit the changes by selecting `Installation` > `Apply Changes`.
   Confirm the opening window by hitting the `Apply` button.

The **MinGW/MSYS** base environment is ready, but a patch needs to be installed
before proceeding further.

## Patching the MSYS installation ##

The latest provided **MSYS** package, which is the `1.0.19` in date, contains
**a severe bug in the heap memory management system**. This can stop the `gcc`
compilation in progress with the following unresolvable error:
`Couldn't commit memory for cygwin heap, Win32 error`.

In order to resolve this bug, you must install the `msys-1.0.dll` from the
[C::B Advanced package](https://sourceforge.net/projects/cbadvanced/files/),
which has been patched to increase the maximum heap internal memory size from
`256 MB` to more than `1 GB`). Unfortunately, that package was removed from the
[C::B Advanced](https://sourceforge.net/projects/cbadvanced/files/) repository,
as they are now using the modern **MinGW-w64/MSYS2** environment.
Fortunately, the required package was cached in this directory, under the
following name: `msysCORE-1.0.18-1-heap-patch-20140117.7z`.

This patch is just necessary to build the `gcc` cross-compiler. After building
all the toolchains, you can revert back the replaced `msys-1.0.dll` with its
original version.

To install the **MSYS** heap patch:

1. Fire up the **MSYS Shell** one more time (it's needed to create some
   necessary files, e.g. the `/etc/fstab` file). You can do that by
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
dynamically linked, and that's the way that meant to be. The drawback is
if you want to use the toolchains outside the **MinGW/MSYS** environment and
the binaries are dynamically linked, you'll have some error messages like:
```
The file libintl-8.dll is missing from your computer.
```
This happens if you just double-click on any `sh-elf` binaries (e.g.
`sh-elf-gcc`), including with `arm-eabi` binaries.

In the **MinGW/MSYS** environment, you have the option to make the
toolchain binaries statically linked; i.e. they can be run **outside** the
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
**MSYS Shell**, you may just leave the `STANDALONE_BINARY` flag undefined.

**Note:** This flag exists to build [DreamSDK](https://dreamsdk.org) as well.

## Compilation ##

The **dc-chain** system may be customized by setting up a
[`Makefile.cfg`] file in the root of the `dc-chain` directory tree. If this is
desired, read the main [`README`](../../README.md) for more information on
setting up custom options for the toolchain; however, in most circumstances,
the stable defaults already present in
[`Makefile.dreamcast.cfg`](../../Makefile.dreamcast.cfg) will be fine.

### Building the toolchain ###

To build the toolchain, do the following:

1. Start the **MSYS Shell** if not already done.

2. Navigate to the `dc-chain` directory by entering:
   ```
   cd /opt/toolchains/dc/kos/utils/dc-chain/
   ```

3. (Optional) Copy and alter the `Makefile.cfg` file options to your liking.

4. Enter the following to start downloading and building toolchain:
   ```
	make
   ```

Now it's time to have a coffee as this process can be long: several minutes to
hours will be needed to build the full toolchain, depending on your system.

### Removing all useless files ###

After everything is done, you can cleanup all temporary files by entering:
```
make distclean
```
## Removing the MSYS heap patch ##

After your toolchains is ready, please don't forget to replace the patched
`msys-1.0.dll` with its original version (the patched file's `SHA-1` hash
is `4f7c8eb2d061cdf4d256df624f260d0e58043072`).

But before replacing the file, close the running **MSYS Shell** by entering
the `exit` command!

## Fixing up Newlib for SH4 ##

The `ln` command in the **MinGW/MSYS** environment is not effective, as
symbolic links are not well managed under this environment.
That's why you need to manually fix up **SH4** `Newlib` when updating your
toolchains (i.e. rebuilding it) and/or updating **KallistiOS**.

This is the purpose of the provided `fixup-sh4-newlib.sh` script.

Before executing it, edit the file to be sure the `$toolchains_base` variable
is correctly set, then run it by entering:
```
./packages/fixup-sh4-newlib.sh
```
## Next steps ##

After following this guide, the toolchains should be ready.

Now it's time to compile **KallistiOS**.

You may consult the [`README`](../../../../doc/README) file from KallistiOS now.
