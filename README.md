KallistiOS
===

**KallistiOS** (**KOS**) is an unofficial development environment for the
**SEGA Dreamcast** (**DC**) game console.

It was developed from scratch over the internet by a group of free software
developers, and has no relation to the official **Sega Katana** or **Microsoft
Windows CE** software development kits (**SDK**).

It's the great way to develop homebrew softwares for the **Sega Dreamcast**,
including commercial indie games, like **Feet of Fury**, **Maquipai**,
**Dynamic Dreams** and many other!

Getting Started
---------------

The main setup steps are:

1. Build an **sh-elf** and **arm-eabi** cross-compilers. To do that, you can
   use the **dc-chain** build system, that you will find in the `utils/dc-chain`
   directory. You can even read the documentation in the `dc-chain` directory to
   learn how to build these cross-compilers for your environment. Supported
   environments are:

	- **BSD**
	- **Cygwin**
	- **GNU/Linux**
	- **macOS**	
	- **MinGW/MSYS**
	- **MinGW-w64/MSYS2** 

2. Build **KallistiOS** itself (`kos`). You will need to set up an environment
   variable file (`environ.sh`) before doing that.

3. Build **KallistiOS** utilities:

		cd utils
		make

3. **Optional:** Build the **KallistiOS ports** `kos-ports`. You need to build
   **KallistiOS** before doing that.

4. **Optional:** If you own a **Coders Cable** and/or a **Broadband Adapter** 
   (`HIT-400`/`HIT-401`) or even a **LAN Adapter** (`HIT-300`), you may install
   `dcload-serial` (for the **Coders Cable**) or `dcload-ip` (for the
   **Broadband Adapter** or **LAN Adapter**). This will help you to debug your
   homebrew programs on the real hardware, with the help of the **GNU Debugger**
   (`gdb`)!

5. **Optional:** Build all the **KallistiOS** examples, in order to verify if
   your environment is fully working:

		cd examples
		make

Documentation
-------------

For all documentation, please look in the `doc` directory.
Please read *at least* the main `README` file before trying to begin!

The documentation there will explain in details all the steps to perform to
setup **KallistiOS**.

Getting Help
------------

A more complete installation script, guide as well as programming tutorials may
be found [in the wiki](http://dcemulation.org/?title=Development).

[Ask for help on the official DCEmulation forums](http://dcemulation.org/phpBB/viewforum.php?f=29).

**IRC** chat:

- `irc.freenode.net`
- `#dreamcastdev`

Useful Links
------------

- [KallistiOS Homepage](http://gamedev.allusion.net/softprj/kos/)
- [SourceForge Repository](https://sourceforge.net/projects/cadcdev/)
- [GitHub Repository](https://github.com/KallistiOS/KallistiOS)
- [DCEmulation Programming Discussion](http://dcemulation.org/phpBB/viewforum.php?f=29)

Please note that the official repositories are synchronized.