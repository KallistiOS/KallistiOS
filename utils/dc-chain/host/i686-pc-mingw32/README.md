# KallistiOS under MinGW/MSYS #

This document contains all the instructions to create a fully working environment for **Sega Dreamcast** programming under **MinGW/MSYS**. This document was created because making a working environment under **MinGW/MSYS** is really difficult. If you are using another environment, including **MinGW-w64/MSYS2**, please [visit the official wiki](http://dcemulation.org/?title=Development) to get more information.

This document applies only on the original **MinGW/MSYS** environment provided by [MinGW.org](http://www.mingw.org)

## Installation of MinGW/MSYS ##

1. Open your browser on [MinGW.org](http://www.mingw.org) and download `mingw-get-setup.exe` from the [MinGW repository](https://osdn.net/projects/mingw/releases/).

2. Run `mingw-get-setup.exe` on **Administrator mode** (starting from **Microsoft Windows Vista**) then click on the `Install` button. In the `Installation Directory` text box, input `C:\dcsdk\`. Leave the other options to its defaults then click on `Continue`. The **MinGW/MSYS** installation begins. When the progress bar is full, click on the `Continue` button.

3. When the **MinGW Installation Manager** shows up, select the following packages:
 - `mingw32-base`
 - `mingw32-gcc-g++`
 - `msys-base`
 - `msys-patch`
 - `msys-wget`
 - `msys-coreutils-ext`

Now we can commit the changes by selecting `Installation` > `Apply Changes`, confirm the opening window by hitting the `Apply` button.

## Patching the MSYS installation ##

The latest provided **MSYS** package, which is the `1.0.19` in date, contains **a severe bug in the heap memory management system**. This can stop the `gcc` compilation in progress with the following unresolvable error: `Couldn't commit memory for cygwin heap, Win32 error`. 

In order to resolve this bug, you must install the `msys-1.0.dll` from the [C::B Advanced package](https://sourceforge.net/projects/cbadvanced/files/) which has been patched to increase the heap internal memory size at its maximum value (i.e. from `256 MB` to more than `1 GB`). The problem is that package was removed as the [C::B Advanced](https://sourceforge.net/projects/cbadvanced/files/) project is now using the modern **MinGW-w64/MSYS2** environment. Fortunately, the required package was cached in this directory, under the following name: `msysCORE-1.0.18-1-heap-patch-20140117.7z`.

This patch is just necessary to build the `gcc` cross-compiler. After installing all the toolchains, you can revert back the replaced `msys-1.0.dll` with its original version.

To install the **MSYS** heap patch:

1. Fire up at least one time the **MSYS** bash (it's needed to create some necessary file, e.g. the `/etc/fstab` file).
2. Close the bash by entering the `exit` command.
3. Move the original `/bin/msys-1.0.dll` (i.e. `C:\dcsdk\msys\1.0\bin\msys-1.0.dll`) outside its folder (please don't just rename the file in the `/bin` folder!).
4. Extract the patched `msys-1.0.dll` from `msysCORE-1.0.18-1-heap-patch-20140117.7z` and place it in the `/bin` directory (i.e. `C:\dcsdk\msys\1.0\bin\`).

## Checking the `/mingw` mount point ##

This step should be automatic, but in the past we had problems with the `/mingw` mount point.

Before doing anything, just check if you can access the `/mingw` mount point with the `cd /mingw` command.

## Preparing the environment ##

Go to the [KallistiOS official repository](https://github.com/KallistiOS/KallistiOS) then download the [KallistiOS package](https://github.com/KallistiOS/KallistiOS) from **Git** (or you can just clone the repository). Install it in `/opt/toolchains/dc/kos/` (i.e. `C:\dcsdk\msys\1.0\opt\toolchains\dc\kos\`).

Download the [KOS-Ports](https://github.com/KallistiOS/kos-ports) and install it on `/opt/toolchains/dc/kos-ports/` (i.e. `C:\dcsdk\msys\1.0\opt\toolchains\dc\kos-ports\`).

## Toolchain compilation ##

**KallistiOS** provides a complete system that make and install all required toolchains from source codes: `dc-chain`.

The `dc-chain` system is mainly composed by a `Makefile` doing all the necessary. Open that file with a text editor and locate the `User configuration` section. You can tweak some parameters, but usually everything is ready to work out-of-the-box. For example, it isn't recommended to change the toolchains program versions. The highest versions confirmed to work with the **Sega Dreamcast** are always already set in that `Makefile` (e.g. `gcc 4.8.x` branch is unstable, so it's really better to stick with the `4.7.3` version).

To make the toolchains, do the following:

1. Start the **MSYS** shell.
2. Navigate to the `dc-chain` directory by entering `cd /opt/toolchains/dc/kos/utils/dc-chain/`.
3. Enter `./download.sh`. This will download all the source packages for all components.
4. Enter `./unpack.sh`. This will unpack all packages.
5. Enter `make`.

Now it's time to take a coffee as this process is really long: several hours will be needed to make the full toolchain!

If you want to install the **GNU Debugger** (`gdb`), just enter `make gdb`. This will install `sh-elf-gdb` and can be used to debug programs throught `dc-load/dc-tool`.

After everything is done, you can cleanup all temporary files by just entering `./cleanup.sh`.

Don't forget to replace the patched `msys-1.0.dll` with its original version (i.e. the patched file `SHA-1` is `4f7c8eb2d061cdf4d256df624f260d0e58043072`).

## Fixing up Newlib for SH-4 ##

The `ln` command in the **MinGW/MSYS** environment is not effective, as symbolic links are not well managed under this environment.
That's why you need to manually fix up **SH-4** `newlib` when updating your toolchain (i.e. rebuilding it) and/or updating **KallistiOS**.

This is the purpose of the provided `fixup-sh4-newlib.sh` shell script.

Before executing it, just edit it to be sure if the `$toolchains_base` variable is correctly set. Then execute it by just entering `./fixup-sh4-newlib.sh`.

## Final note ##

After following this guide, the toolchain should be ready.

You can just compile **KallistiOS** by following the same guide as the others platforms, that's why it isn't described here.
Please read the `/opt/toolchains/dc/kos/doc/README` file to learn the next steps.
