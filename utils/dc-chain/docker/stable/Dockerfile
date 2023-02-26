#
# Dockerfile for Sega Dreamcast Toolchains Maker (dc-chain): Stable
#

# Stage 1
FROM alpine:latest as build
MAINTAINER KallistiOS <cadcdev-kallistios@lists.sourceforge.net>

# Installing prerequisites
# Note: libelf-dev is not available in main repository, so we use the v3.9 package
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
	&& apk --update add --no-cache libelf-dev --repository=http://dl-cdn.alpinelinux.org/alpine/v3.9/main \
	&& rm -rf /var/cache/apk/*

# Making Sega Dreamcast toolchains
# You may adapt the KallistiOS repository URL if needed
RUN mkdir -p /opt/toolchains/dc \
	&& git clone https://git.code.sf.net/p/cadcdev/kallistios /opt/toolchains/dc/kos \
	&& cd /opt/toolchains/dc/kos/utils/dc-chain \
	&& cp config.mk.stable.sample config.mk \
	&& ./download.sh && ./unpack.sh && make && make gdb \
	&& rm -rf /opt/toolchains/dc/kos

# Stage 2
# Optimize image size
FROM scratch
COPY --from=build / /
