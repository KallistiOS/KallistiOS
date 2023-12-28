# Sega Dreamcast Toolchains Maker (dc-chain)
# This file is part of KallistiOS.
#
# Created by Jim Ursetto (2004)
# Initially adapted from Stalin's build script version 0.3.
#

# If erase=1, erase build directories on the fly.
ifeq (1,$(erase))
  define clean_up
    @echo "+++ Cleaning up $(build)..."
    -rm -rf $(build)
  endef
endif

# If verbose=1, display output to screen as well as log files
ifeq (1,$(verbose))
  to_log = 2>&1 | tee -a $(log) && [ $$PIPESTATUS -eq 0 ]
else
  to_log = >> $(log) 2>&1
endif
