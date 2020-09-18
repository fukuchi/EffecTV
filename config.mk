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
## Linux for PlayStation2
# ARCH = ps2-linux

### vloopback
## comment out next line if you want to disable vloopback support.
USE_VLOOPBACK = no

### Default settings
## Set a default device file name of the video input.
DEFAULT_VIDEO_DEVICE = "/dev/video0"

###############################################################################
### none user configurable settings

## architecture dependent settings
## i686-linux
ifeq ($(ARCH), i686-linux)
CONFIG.arch = -DI686
CFLAGS.opt = -O3 -fomit-frame-pointer -funroll-loops
endif

ifeq ($(USE_VLOOPBACK), yes)
CONFIG += -DUSE_VLOOPBACK
endif

CONFIG += -DDEFAULT_VIDEO_DEVICE=\"$(DEFAULT_VIDEO_DEVICE)\"
