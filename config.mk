###Common configuration file for Makefiles of EffecTV

### Install
## the following lines set destination directory for a compiled program and
## manual page.
prefix = /usr/local
exec_prefix = ${prefix}

bindir = $(DESTDIR)${exec_prefix}/bin
mandir = $(DESTDIR)${prefix}/man

### vloopback
## comment out next line if you want to disable vloopback support.
USE_VLOOPBACK = no

### Default settings
## Set a default device file name of the video input.
DEFAULT_VIDEO_DEVICE = "/dev/video0"

###############################################################################
### none user configurable settings

CONFIG.arch = -DI686
CFLAGS.opt = -O3 -fomit-frame-pointer -funroll-loops
CFLAGS.debug = -g #-fsanitize=leak

ifeq ($(USE_VLOOPBACK), yes)
CONFIG += -DUSE_VLOOPBACK
endif

CONFIG += -DDEFAULT_VIDEO_DEVICE=\"$(DEFAULT_VIDEO_DEVICE)\"
