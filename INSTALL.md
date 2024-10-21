
# KallistiOS (KOS) Installation Guide

## Introduction
Welcome to the KallistiOS (KOS) installation guide. This guide will help you set up the KOS development environment on your machine, regardless of whether you're using Linux, macOS, or Windows (via WSL).

The key to setting up KOS is the `dc-chain` script, which compiles the required toolchain for Dreamcast development. Before we proceed with building the toolchain, let's first install the necessary dependencies for each platform.

## Prerequisites
Before starting, make sure you have the following installed:

### General Prerequisites
- Git
- Make
- Compiler (with C and C++ support)

## Dependencies
Before you can build the toolchain, you'll need to grab some dependencies. We've listed the commands for different systems below. Chances are you might already have some of these installed, but we’ve included everything just to be safe!

### macOS 13 Ventura + on an Intel or Apple Silicon Processor
1. **Ensure Xcode and Command Line Tools are installed.**
2. **Install Homebrew**:
    ```bash
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    ```
3. **Install dependencies via Homebrew**:
    ```bash
    brew install wget gettext texinfo gmp mpfr libmpc libelf jpeg-turbo libpng cmake
    ```

4. **Important Note**:  
    Homebrew puts libraries in a spot that the compiler doesn’t automatically check. If you haven’t added them to your ~/.zprofile yet, go ahead and do that now. Then, either restart your terminal session or just run the commands to update your current one.
    - Intel:
        ```bash
        export CPATH=/usr/local/include
        export LIBRARY_PATH=/usr/local/lib
        ```
    - Apple Silicon:
        ```bash
        export CPATH=/opt/homebrew/include
        export LIBRARY_PATH=/opt/homebrew/lib
        ```

### Debian/Ubuntu-based Linux
```bash
sudo apt-get update
sudo apt install gawk patch bzip2 tar make libgmp-dev libmpfr-dev libmpc-dev gettext wget libelf-dev texinfo bison flex sed git build-essential diffutils curl libjpeg-dev libpng-dev python3 pkg-config cmake libisofs-dev
```

### Fedora-based Linux
```bash
sudo dnf install gawk patch bzip2 tar make gmp-devel mpfr-devel libmpc-devel gettext wget elfutils-libelf-devel texinfo bison flex sed git diffutils curl libjpeg-turbo-devel libpng-devel gcc-c++ python3 rubygem-rake
```

### Arch-based Linux
```bash
sudo pacman -S --needed gawk patch bzip2 tar make gmp mpfr libmpc gettext wget libelf texinfo bison flex sed git diffutils curl libjpeg-turbo libpng python3 ruby-rake
```

### Alpine-based Linux
```bash
sudo apk --update add build-base patch bash texinfo gmp-dev libjpeg-turbo-dev libpng-dev elfutils-dev curl wget python3 git ruby-rake
```

## Steps for All Platforms

Once you've installed the dependencies on your platform, the following steps will be the same across all platforms.

### 1. Setup Folders
First, let’s create the directory where the toolchain and KOS will be installed, set the correct permissions, and make sure you own the folder:

```bash
sudo mkdir -p /opt/toolchains/dc
sudo chmod -R 755 /opt/toolchains/dc
sudo chown -R $(id -u):$(id -g) /opt/toolchains/dc
```

### 2. Clone the KOS Repository
```bash
git clone https://github.com/KallistiOS/KallistiOS.git /opt/toolchains/dc/kos
```

### 3. Customize your `dc-chain` Setup
- **Enter the dc-chain directory**
```bash
cd /opt/toolchains/dc/kos/utils/dc-chain
```
- **Customize Makefile.cfg**

To set up, copy or rename Makefile.default.cfg to Makefile.cfg, then edit it with a text editor. You can adjust options like makejobs to match your CPU threads for faster compiling. If you run into issues, try setting makejobs=1.

We’ll stick with the **default profile (GCC 13.2)**, but if you’re an advanced user, you can explore other profiles in the README.md under dc-chain. If you're unsure, leaving the defaults is fine.

### 4. Download and compile the toolchain
 
You can either build just the SH4 CPU compiler or include the ARM compiler for sound processing. The ARM compiler is only needed if you plan to customize the sound driver since KallistiOS includes a prebuilt one.


- **Build SH4:**
```bash
make
```
- **Build SH4 and ARM:**
```bash
make all
```
- **Cleaning up once everything is built:**

```bash
make clean
```

### 5. Configuring and compiling KOS and kos-ports

Now that the SH4 toolchain is installed, you can move on to compiling KOS. 

- **First, go to the KOS directory:**
```bash
cd /opt/toolchains/dc/kos
```

- **Copy the environment script:**
```bash
cp doc/environ.sh.sample environ.sh
```

The default settings should work fine for most users, but if you’re an advanced user, feel free to tweak `environ.sh` for different compile flags or paths.

> **⚠️ Important:**  
> Every time you open a new terminal or before compiling, **you must load the environment settings** by running:
>
> ```bash
> source /opt/toolchains/dc/kos/environ.sh
> ```  
> If you skip this step, the build may fail or KOS might not compile correctly.


- **Building KOS:**

To build KOS, just run:
```bash
make
```
Now KOS is ready.

- **Building kos-ports:**

Clone the kos-ports repository:
```bash
git clone --recursive https://github.com/KallistiOS/kos-ports /opt/toolchains/dc/kos-ports
```

Build all the ports:
```bash
/opt/toolchains/dc/kos-ports/utils/build-all.sh
```
**Note:** `kos-ports` is now built.  
It's perfectly fine if a couple of ports fail to build—this is expected in some cases and shouldn't affect your overall setup.


### 6. Building the KOS examples

Go to the examples directory:
```bash
cd /opt/toolchains/dc/kos/examples/dreamcast
```

Build the example programs:
```bash
make
```
That’s it! All the KallistiOS example programs are now built.

## Troubleshooting
If you encounter any issues during the setup or compilation process, here are some common solutions:

- **Missing dependencies**: Ensure all required packages are installed (refer to the dependencies section above).
- **Permission issues**: Ensure you have the necessary permissions to execute the scripts.

Happy Dreamcast development!
