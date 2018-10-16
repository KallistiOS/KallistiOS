# Sega Dreamcast toolchain with Cygwin #

This document contains all the instructions to create a fully working
toolchain targeting the **Sega Dreamcast** system under **Cygwin**.

## Introduction ##

On the **Cygwin** system, the package manager is the `setup-${arch}.exe`
file. It's designed to be run on graphical user-interface (GUI) mode.

The **Cygwin** environment exists in two flavors:

- **32-bit**: `i686`
- **64-bit**: `x86_64`

This document was written when using the `i686` version, so if you are using
the `x86_64` version, you should replace all `i686` keywords in the packages
name with `x86_64`.

There is no particular improvement by running a specific flavors, so it's
should be nice to stick with your **Windows** host flavor, i.e. if you are
using **Windows x64**, then use the `x86_64` flavor.

## Installation of Cygwin ##

1. Open your browser on [**Cygwin.com**](https://www.cygwin.com/) and download
   `setup-${arch}.exe` (e.g. `setup-x86_64.exe`) from the 
   [**Cygwin** website](https://cygwin.com/install.html).

2. Run `setup-${arch}.exe` on **Administrator mode** (starting from
   **Microsoft Windows Vista**) then click on the `Install` button. In the
`Installation Directory` text box, input `C:\dcsdk\` or something else. The
`Installation Directory` will be called `${MINGW_ROOT}` later in the document.

3. Leave the other options to its defaults then click on `Continue`. 
The **Cygwin** installation begins. When the progress bar is full, click on
the `Continue` button.

4. When the **MinGW Installation Manager** shows up, select the following
packages:

	- `autoconf`
	- `automake`
	- `binutils`
	- `curl`
	- `gcc-core`
	- `gcc-g++`
	- `git`
	- `libelf0-devel`
	- `libjpeg-devel`
	- `libpng-devel`
	- `make`
	- `patch`
	- `python2`
	- `subversion`
	- `texinfo`

5. Now we can commit the changes by selecting `Installation` > `Apply Changes`,
   confirm the opening window by hitting the `Apply` button.

The **Cygwin** base environment is ready, but a patch should be installed
before doing anything. It's the purpose of the section located below.

## Preparing the environment installation ##

1. Open the **Cygwin** Terminal by double-clicking the shortcut on your desktop 
   (or alternatively, double-click on the `${MINGW_ROOT}\msys\1.0\msys.bat` batch 
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

## Toolchain compilation ##

**KallistiOS** provides a complete system that make and install all required
toolchains from source codes: **dc-chain**.

The **dc-chain** system is mainly composed by a `Makefile` doing all the
necessary. Open that file with a text editor and locate the `User configuration`
section. You can tweak some parameters, but usually everything is ready to
work out-of-the-box. For example, it isn't recommended to change the toolchains
program versions. The highest versions confirmed to work with the
**Sega Dreamcast** are always already set in that `Makefile`.

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

If you want to install the **GNU Debugger** (`gdb`), just enter:

	make gdb

This will install `sh-elf-gdb` and can be used to debug programs through
`dc-load/dc-tool`.

After everything is done, you can cleanup all temporary files by entering:

	./cleanup.sh

## Next steps ##

After following this guide, the toolchain should be ready.

Now it's time to compile **KallistiOS**.

Please read the `/opt/toolchains/dc/kos/doc/README` file to learn the next
steps.
