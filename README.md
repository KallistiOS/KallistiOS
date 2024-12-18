<!-- PROJECT LOGO -->
<div align="center">
  <h1 align="center"><strong>KallistiOS</strong></h1>
  <p align="center">
    Independent SDK for the Sega Dreamcast
    <br />
    <a href="https://kos-docs.dreamcast.wiki"><strong>Explore the docs »</strong></a>
  </p>
</div>

# Overview

KallistiOS is a development library and operating system for the Sega Dreamcast game console, developed independently from Sega entirely by free software developers. Its flexible permissive license allows both homebrew and commercial use with little restrictions besides proper attribution. As a result, it powers most homebrew and commercial indie releases for the platform. Interfaces and drivers are included for a significant portion of the Dreamcast's hardware capabilities and accessories, including modifications created by hobbyists. 

KallistiOS offers a modern, programmer friendly environment for the vintage Dreamcast system. The distribution includes [scripts for building and installing a cross-compiling toolchain](utils/dc-chain/README.md) using the latest GCC, Binutils, and Newlib. This allows for full support for C17 and C++20 standards and libraries, as well as various POSIX APIs. Preliminary support exists for C23, C++23, and Objective-C.

KallistiOS also features a package manager called [**_kos-ports_**](https://github.com/KallistiOS/kos-ports) which gives developers the power to build and include a rich set of add-on libraries for various common audiovisual formats (jpg, png, mp3, ogg, mpeg), compression formats (libbz2, zlib), scripting languages (Lua, Tcl, MicroPython), gaming APIs (OpenGL, OpenAL, SDL), and many more.

### Core Functionality
* Concurrency with Kernel Threads, C11 Threads, C++11 `std::thread`, POSIX threads
* Virtual Filesystem Abstraction
* IPv4/IPv6 Network Stack
* Dynamically Loaded Libraries and Modules
* GDB Debugger Support

### Dreamcast Hardware Support
* Optical Disc Drive
* Low-level 3D PowerVR Graphics 
* SH4 ASM-Optimized Math Routines
* SH4 SCIF Serial I/O
* DMA Controller 
* FlashROM Filesystem
* AICA SPU Sound Processor Driver
* Cache and Store Queue Management
* Timer Peripherals, Real-Time Clock, Watchdog Timer
* Performance Counters
* MMU Management

### Peripherals and Accessory Support
* Standard controllers, expanded controllers with additional buttons and dual analog sticks 
* Arcade Sticks and Twin Sticks
* Lightguns, Racing Wheels, Fishing Rods, Samba Maracas, Dance Mat
* Visual Memory Unit, Memory Cards, Puru Puru/Jump Vibration Pack, Microphone, Dreameye Webcam
* 33.6K and 56K Dial-up Modems, 100/10Mbps "Broadband Adapter", 10Mbps "LAN Adapter"
* RCA, S-video, RGB, and VGA output
* Serial-based SD card readers

### Hardware Modification Support
* IDE hard drive, 32MB RAM upgrade, custom BIOS flashROMs

# Getting Started 
A beginner's guide to development for the Sega Dreamcast along with detailed instructions for installing KOS and the required toolchains can be found on [dreamcast.wiki](https://dreamcast.wiki/Getting_Started_with_Dreamcast_development). Additional documentation can be found in the docs folder. 

# Licensing
KallistiOS itself is licensed under the BSD-like **KOS License**. **Attribution is not optional**. Additionally, this distribution contains code licensed under various free software licenses.
See [LICENSE.md](doc/LICENSE.md) for more information on licensing, as well as [LICENSE.KOS](doc/license/LICENSE.KOS) for the actual **KOS License** text.

# Examples 
Once you've set up the environment and are ready to begin developing, a good place to start learning is the examples directory, which provides demos for the various KOS APIs and for interacting with the Dreamcast's hardware. Examples include:
- Hello World
- Console Input/Output
- Assertions, stacktraces, threading
- Drawing directly to the framebuffer
- Rendering with OpenGL
- Rendering with KGL
- Rendering with KOS PVR API
- Texturing with libPNG
- Bump maps, modifier volumes, render-to-texture PVR effects
- Audio playback on the ARM SPU
- Audio playback using SDL Audio
- Audio playback using OGG, MP3, and CDDA
- Querying controller input
- Querying keyboard input
- Querying mouse input
- Querying lightgun input
- Accessing the VMU filesystem
- Accessing the SD card filesystem
- Networking with the modem, broadband adapter, and LAN adapter
- Taking pictures with the DreamEye webcam
- Reading and Writing to/from ATA devices
- Testing 32MB RAM hardware mod
- Interactive Lua interpreter terminal

# Resources
[dreamcast.wiki](http://dreamcast.wiki): Large collection of tutorials and articles for beginners  
[Simulant Discord Chat](https://discord.gg/bpDZHT78PA): Home to the official Discord channel of KOS  
[DCEmulation Forums](http://dcemulation.org/phpBB/viewforum.php?f=29): Goldmine of Dreamcast development information and history  
IRC Channel: irc.libera.chat `#dreamcastdev`

