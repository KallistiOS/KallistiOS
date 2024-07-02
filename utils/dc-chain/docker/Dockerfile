#
# Dockerfile for Sega Dreamcast Toolchains Maker (dc-chain)
#
# usage:
#   - build one of the images:
#       docker build -t dcchain:stable --build-arg dc_chain=stable .
#       docker build -t dcchain:9.3.0-legacy --build-arg dc_chain=9.3.0-legacy --build-arg makejobs=4 .
#   - create and run a container, e.g. for stable:
#       docker run -it --name containername dcchain:stable /bin/bash

# Stage 1
FROM alpine:latest as build
MAINTAINER KallistiOS <cadcdev-kallistios@lists.sourceforge.net>

# Installing prerequisites
RUN apk --update add --no-cache \
        build-base \
        gmp-dev \
        mpfr-dev \
        mpc1-dev \
        patch \
        bash \
        texinfo \
        flex \
        curl \
        wget \
        git \
        subversion \
        elfutils-dev \
        libjpeg-turbo-dev \
        libpng-dev \
        python3 \
        && rm -rf /var/cache/apk/*

# Making Sega Dreamcast toolchains
# You may adapt the KallistiOS repository URL if needed
ARG dc_chain=stable
ARG makejobs=2
RUN mkdir -p /opt/toolchains/dc \
	&& git clone https://git.code.sf.net/p/cadcdev/kallistios /opt/toolchains/dc/kos || git clone https://github.com/KallistiOS/KallistiOS.git /opt/toolchains/dc/kos \
	&& cd /opt/toolchains/dc/kos/utils/dc-chain \
	&& make build toolchain_profile=$dc_chain makejobs=$makejobs \
	&& rm -rf /opt/toolchains/dc/kos

# Stage 2
# Optimize image size
FROM scratch
COPY --from=build / /
