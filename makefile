# ----------------------------
# Makefile Options
# ----------------------------

NAME = CRIMCAS
ICON = icon.png
DESCRIPTION = "TI-84 CE Computer Algebra System"
COMPRESSED = YES
COMPRESSED_MODE = zx7

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# Add this line to link the graphics library
LIBS = -lgfx

# ----------------------------

include $(shell cedev-config --makefile)
