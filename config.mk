# Common configuration file for Makefiles of EffecTV

## NASM
# comment out the next two lines if you want not to use NASM.
CONFIG += -DUSE_NASM
USE_NASM = yes

## vloopback
# comment out the next two lines if you want to disable vloopback support.
CONFIG += -DUSE_VLOOPBACK
USE_VLOOPBACK = yes

ifeq ($(USE_VLOOPBACK), yes)
# choose vloopback version (only one).
#  version 0.90 or later (not supported yet)
#CONFIG += -DVLOOPBACK_VERSION=90 
#  version 0.83 or former
CONFIG += -DVLOOPBACK_VERSION=83
endif
