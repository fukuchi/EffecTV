###Common configuration file for Makefiles of EffecTV

### Install
## the following lines set destination directory for a compiled program and
## manual page.
prefix = /usr/local
exec_prefix = ${prefix}

bindir = $(DESTDIR)${exec_prefix}/bin
mandir = $(DESTDIR)${prefix}/man

### Architecture
## choose your architecture (only one)
## Linux for intel architecture
ARCH = i686-linux
## Linux for Playstation2
# ARCH = ps2-linux

### NASM
## comment out the next two lines if you want not to use NASM.
USE_NASM = yes

### vloopback
## comment out the next two lines if you want to disable vloopback support.
USE_VLOOPBACK = yes

## choose vloopback version (only one).
## version 0.90 or later (not supported yet)
# VLOOPBACK_VERSION = 90
## version 0.83 or former
VLOOPBACK_VERSION = 83


###############################################################################
### none user configurable settings

## architecture dependent settings
## i686-linux
ifeq ($(ARCH), i686-linux)
CFLAGS.opt = -mpentiumpro -O3 -fomit-frame-pointer -funroll-loops
endif

## Playstaion2
ifeq ($(ARCH), ps2-linux)
CFLAGS.opt = -O3 -fomit-frame-pointer -funroll-loops
USE_NASM = no
CONFIG += -DRGB_BGR_CONVERSION
endif

ifeq ($(USE_NASM), yes)
CONFIG += -DUSE_NASM
endif
ifeq ($(USE_VLOOPBACK), yes)
CONFIG += -DUSE_VLOOPBACK
CONFIG += -DVLOOPBACK_VERSION=$(VLOOPBACK_VERSION)
endif
