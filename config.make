## Platform configurations


# Mingw-w64 64bit
# make PLATFORM=mingw-w64-x86_64
ifeq ($(PLATFORM), mingw-w64-x86_64)
TARGET=Windows
export PATH := /mingw64/bin:$(PATH)
CC=cc
endif


# Mingw-w64 32bit
# make PLATFORM=mingw-w64-i686
ifeq ($(PLATFORM), mingw-w64-i686)
TARGET=Windows
export PATH := /mingw32/bin:$(PATH)
CC=cc
endif


# Mingw-w64 64bit, cross compile (e.g. from MacPorts)
# make PLATFORM=mingw-w64-x86_64-cross
ifeq ($(PLATFORM), mingw-w64-x86_64-cross)
TARGET=Windows
CC=x86_64-w64-mingw32-gcc
AR=x86_64-w64-mingw32-ar
LD=x86_64-w64-mingw32-ld
STRIP=x86_64-w64-mingw32-strip
endif


# Mingw-w64 32bit, cross compile (e.g. from MacPorts)
# make PLATFORM=mingw-w64-i686-cross
ifeq ($(PLATFORM), mingw-w64-i686-cross)
TARGET=Windows
CC=i686-w64-mingw32-gcc
AR=i686-w64-mingw32-ar
LD=i686-w64-mingw32-ld
STRIP=i686-w64-mingw32-strip
endif


####
