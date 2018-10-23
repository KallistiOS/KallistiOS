KallistiOS
===

**KallistiOS** (**KOS**) is an unofficial development environment for the
**Sega Dreamcast** (**DC**) game console.

It was developed from scratch over the internet by a group of free software
developers, and has no relation to the official **Sega Katana** or **Microsoft
Windows CE** software development kits (**SDK**).

It's the great way to develop homebrew softwares for the **Sega Dreamcast**,
including commercial indie games, like **Feet of Fury**, **Maqiupai**,
**Dynamic Dreams**, **Irides: Master of Blocks**, **Last Hope** and many more!

Introduction
-------------

For all documentation, please look in the `doc` directory.
Please read *at least* the main `README` file before trying to begin!

The documentation there will explain in details all the steps to perform to
setup **KallistiOS**.

Getting Started
---------------

The main setup steps are:

1. Read the `doc/README` file.

2. Build an **sh-elf** and **arm-eabi** cross-compilers. To do that, you can
   use the **dc-chain** build system, that you will find in the `utils/dc-chain`
   directory. You may even read the documentation in the `dc-chain` directory to
   learn how to build these cross-compilers for your environment. Supported and
   tested environments are:

	- **BSD** (through `FreeBSD`)
	- **Cygwin**
	- **GNU/Linux** (through `Lubuntu`)
	- **macOS**
	- **MinGW/MSYS**
	- **MinGW-w64/MSYS2** 

3. Build **KallistiOS** itself (`kos`). You will need to set up an environment
   variable file (`environ.sh`) before doing that. The `doc/README` file that
   you've already read contains all the information related to this step.

4. Build **KallistiOS** utilities:

		cd utils
		make

5. **Optional:** Build the **KallistiOS ports** `kos-ports`. You need to build
   **KallistiOS** before doing that. If you want to build all ports at once:

		cd kos-ports/utils
		./build-all.sh

6. **Optional:** If you own a **Coders Cable** and/or a **Broadband Adapter** 
   (`HIT-400`/`HIT-401`) or even a **LAN Adapter** (`HIT-300`), you may install
   `dcload-serial` (for the **Coders Cable**) or `dcload-ip` (for the
   **Broadband Adapter** or **LAN Adapter**). This will help you to debug your
   homebrew programs on the real hardware, with the help of the **GNU Debugger**
   (`gdb`)! In that case, you'll need to either clone `dcload-serial` or
   `dcload-ip` and enter:

		make
		make install

7. **Optional:** Build all the **KallistiOS** examples, in order to verify if
   your environment is fully working:

		cd examples
		make

8. **Optional:** Cleanup temporary files. At the `kos` root directory, enter:

		make clean

Getting Help
------------

A more complete installation script, guide as well as programming tutorials may
be found [in the wiki](http://dcemulation.org/?title=Development).

[Ask for help on the official DCEmulation forums](http://dcemulation.org/phpBB/viewforum.php?f=29).

Official **IRC** chat:

- `irc.freenode.net`
- `#dreamcastdev`

Useful Links
------------

- [KallistiOS Homepage](http://gamedev.allusion.net/softprj/kos/)
- [SourceForge Repository](https://sourceforge.net/projects/cadcdev/) (Main repository)
- [GitHub Repository](https://github.com/KallistiOS/KallistiOS) (Mirror repository)
- [DCEmulation Programming Discussion](http://dcemulation.org/phpBB/viewforum.php?f=29)
