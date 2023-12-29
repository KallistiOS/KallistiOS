
<!-- PROJECT LOGO -->
<br />
<div align="center">
  <h1 align="center"><strong>KallistiOS</strong></h1>

  <p align="center">
    Independent SDK for the Sega Dreamcast
    <br />
    <a href="https://kos-docs.dreamcast.wiki"><strong>Explore the docs »</strong></a>
  </p>
</div>

# Overview

KOS is an unofficial development kit for the SEGA Dreamcast game console with some support for the NAOMI and NAOMI 2 arcade boards.

KOS was developed from scratch over the internet by a group of free software developers and has no relation to the official Sega Katana or Microsoft Windows CE Dreamcast development kits. This has allowed it to fuel a thriving Dreamcast homebrew scene, powering many commercial releases for the platform over the years. It supports a significant portion of the Dreamcast's hardware capabilities and a wide variety of peripherals, accessories, and add-ons for the console, including custom hardware modifications that have been created by the scene. 

Despite the console's age, KOS offers an extremely modern, programmer-friendly development environment. Using the latest GCC toolchain, it supports the entirety of C17 and C++20 including their standard libraries, along with support for portions of C23, C++23, Objective-C, and various POSIX APIs. Additionally, KOS-ports offers a rich set of add-on libraries such as SDL, OpenGL, OpenAL, and Lua for the platform.

# Features
## Core Functionality
* Concurrency with Kernel Threads, C11 Threads, C++11 `std::thread`, POSIX threads
* Virtual Filesystem Abstraction
* IPv4/IPv6 Network Stack
* Dynamically Loaded Libraries and Modules
* GDB Debugger Support

## Dreamcast Hardware Support
* GD-ROM Optical Drive
* Low-level 3D PowerVR Graphics 
* SH4 ASM-Optimized Math Routines
* SH4 SCIF Serial I/O
* DMA Controller 
* Flashrom Filesystem
* AICA SPU Sound Processor Driver
* Cache and Store Queue Management
* Timer Peripherals, Real-Time Clock, Watchdog Timer
* Performance Counters
* MMU Management
* BIOS Font Rendering

## Peripherals and Accessory Support
* Controller, ASCII Pad
* Arcade Stick, Twin Stick, Mission Stick
* Keyboard
* Mouse
* Visual Memory Unit
* Puru Puru Vibration Pack
* Seaman Microphone
* Dreameye Webcam
* Lightgun 
* Racing Wheel
* Fishing Rod
* Samba De Amigo Maracas
* Dance Mat
* Dial-up Modem
* Broadband Adapter
* LAN Adapter
* VGA Adapter
* SD Card Reader

## Hardware Modification Support
* IDE Hard Drive
* 32MB RAM Upgrade
* Custom BIOS Flashroms

# Getting Started 
A beginner's guide to development for the Sega Dreamcast along with detailed instructions for installing KOS and the required toolchains can be found on the [Dreamcast Wiki](https://dreamcast.wiki/Getting_Started_with_Dreamcast_development). Additional documentation can be found in the docs folder. 

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
[Dreamcast Wiki](http://dreamcast.wiki): Large collection of tutorials and articles for beginners  
[Simulant Discord Chat](https://discord.gg/bpDZHT78PA): Home to the official Discord channel of KOS  
[DCEmulation Forums](http://dcemulation.org/phpBB/viewforum.php?f=29): Goldmine of Dreamcast development information and history  
IRC Channel: irc.libera.chat `#dreamcastdev`

