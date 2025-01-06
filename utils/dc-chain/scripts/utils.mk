# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.

# Web downloaders command-lines
downloaders = curl wget
wget_cmd = wget -c $(if $(2),-O $(2)) '$(1)'
curl_cmd = curl --fail --location  -C - $(if $(2),-o $(2),-O) '$(1)' 

ifneq ($(force_downloader),)
# Check if specified downloader is in supported list
  web_downloader = $(if $(filter $(downloaders),$(force_downloader)),$(force_downloader))
else
# If no downloader specified, check to see if any are detected
  web_downloader = $(if $(shell which curl),curl,$(if $(shell which wget),wget))
endif

# Make sure valid downloader was found
ifeq ($(web_downloader),)
  ifeq ($(force_downloader),)
    $(error No supported downloader was found ($(downloaders)))
  else
    $(error Unsupported downloader ($(force_downloader)), select from ($(downloaders))) 
  endif
endif

$(info Using $(web_downloader) as download tool)
# Function to call downloader
# Args:
# 1 - URL
# 2 - Output File (Optional)
web_download = $(call $(web_downloader)_cmd,$(1),$(2))
