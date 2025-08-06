#
# Dockerfile for Sega Dreamcast Toolchains Maker (dc-chain)
#
# usage:
#   - build one of the images:
#       docker build -t dcchain:stable --build-arg profile=stable -f ./Dockerfile $KOS_BASE
#       docker build -t dcchain:9.3.0-legacy --build-arg profile=9.3.0-legacy -f ./Dockerfile $KOS_BASE
#   - create and run a container, e.g. for stable:
#       docker run -it --name containername dcchain:stable /bin/bash

# Stage 1
FROM alpine:latest as build
MAINTAINER KallistiOS <cadcdev-kallistios@lists.sourceforge.net>

# Installing prerequisites
RUN apk --update add --no-cache \
        bison \
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
ARG platform=dreamcast
ARG profile=stable

RUN mkdir -p /opt/toolchains/dc
COPY . /opt/toolchains/dc/kos

RUN cd /opt/toolchains/dc/kos/utils/dc-chain \
	&& make build platform=$platform toolchain_profile=$profile \
	&& rm -rf /opt/toolchains/dc/kos

# Stage 2
# Optimize image size
FROM scratch
COPY --from=build / /
