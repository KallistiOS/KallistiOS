#
# Dockerfile for Sega Dreamcast Toolchains Maker (dc-chain)
#
# usage:
#   - build one of the images:
#       docker build -t dcchain:latest . 
#       docker build -t dcchain:stable --build-arg dc_chain=stable .
#       docker build -t dcchain:legacy --build-arg dc_chain=legacy . 
#   - create and run a container, eg for latest:
#       docker run -it --name containername dcchain:latest /bin/bash

# Stage 1
FROM alpine:latest as build
MAINTAINER KallistiOS <cadcdev-kallistios@lists.sourceforge.net>

# Installing prerequisites
RUN apk --update add --no-cache \
	build-base \
	patch \
	bash \
	texinfo \
	libjpeg-turbo-dev \
	libpng-dev \
	curl \
	wget \
	git \
	python3 \
	subversion \
	gmp-dev \
	elfutils-dev \
	&& rm -rf /var/cache/apk/*

# Making Sega Dreamcast toolchains
# You may adapt the KallistiOS repository URL if needed
ARG dc_chain=stable
RUN mkdir -p /opt/toolchains/dc \
	&& git clone https://git.code.sf.net/p/cadcdev/kallistios /opt/toolchains/dc/kos \
	&& cd /opt/toolchains/dc/kos/utils/dc-chain \
	&& cp config/config.mk.$dc_chain.sample config.mk \
	&& sed -i -e 's/#use_custom_dependencies=1/use_custom_dependencies=1/g' config.mk \
	&& make && make gdb \
	&& rm -rf /opt/toolchains/dc/kos

# Stage 2
# Optimize image size
FROM scratch
COPY --from=build / /
