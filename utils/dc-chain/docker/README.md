# Dockerfile

This directory contains a `Dockerfile` which demonstrates how to build a Docker
image containing the minimal toolchains used for **Sega Dreamcast** development.

The Docker image foundation is based on [Alpine Linux](https://alpinelinux.org/).

Toolchains included in the image are
* An `sh-elf` toolchain, targeting the main CPU, which will be located in
  `/opt/toolchains/dc/sh-elf`
* An `arm-eabi` toolchain, targeting the audio chip, which will be located in
  `/opt/toolchains/dc/arm-eabi`
* The normal host toolchain used for compiling various tools

These images may be used to compile KallistiOS, the open source
**Sega Dreamcast** development library.

To be clear, this `Dockerfile` doesn't build KallistiOS itself, only the
required toolchains. KallistiOS itself is not part of the toolchains. KallistiOS
is updated often so it's better to keep a separate image with the toolchains, as
building them can take hours and the process doesn't change often.

Of course, the Docker image produced here can also be used for CI/CD pipelines!

This Dockerfile builds the `stable` toolchain by default, but can be used to
build the other toolchains like `9.3.0-legacy`, `15.0.1-dev`, etc., as long as
you pass the `dc_chain` argument in the docker command line (see the Dockerfile
for an example of the syntax).
