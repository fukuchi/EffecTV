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

### Multimedia extension code set
## MMX
## comment out next line if you want not to enable MMX operation.
USE_MMX = yes

### NASM
## comment out next line if you want not to use NASM.
USE_NASM = yes

### vloopback
## comment out next line if you want to disable vloopback support.
USE_VLOOPBACK = yes

## choose vloopback version (only one).
# version 0.91 or later
VLOOPBACK_VERSION = 91
## version 0.83 or former (obsolete!!)
# VLOOPBACK_VERSION = 83

### Default settings
## Set a default device file name of the video input.
DEFAULT_VIDEO_DEVICE = "/dev/video0"


### Memory debug
#MEM_DEBUG = yes

###############################################################################
### none user configurable settings

## architecture dependent settings
## i686-linux
ifeq ($(ARCH), i686-linux)
CONFIG.arch = -DI686
CFLAGS.opt = -march=pentiumpro -O3 -fomit-frame-pointer -funroll-loops
endif

## PlayStaion2
ifeq ($(ARCH), ps2-linux)
CONFIG.arch = -DPS2
CFLAGS.opt = -O3 -fomit-frame-pointer -funroll-loops
USE_NASM = no
USE_MMX = no
USE_VLOOPBACK = no
CONFIG += -DRGB_BGR_CONVERSION
LIBS.extra = -ldl -L/usr/X11R6/lib -lX11 -lXext
endif

ifeq ($(USE_NASM), yes)
CONFIG += -DUSE_NASM
endif

ifeq ($(USE_MMX), yes)
CONFIG += -DUSE_MMX
endif

ifeq ($(USE_VLOOPBACK), yes)
CONFIG += -DUSE_VLOOPBACK
CONFIG += -DVLOOPBACK_VERSION=$(VLOOPBACK_VERSION)
endif

CONFIG += -DDEFAULT_VIDEO_DEVICE=\"$(DEFAULT_VIDEO_DEVICE)\"

ifeq ($(MEM_DEBUG), yes)
CONFIG += -DMEM_DEBUG
CFLAGS.opt = -g -O2
endif
