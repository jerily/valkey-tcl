ifndef NAVISERVER
    NAVISERVER  = /usr/local/ns
endif

#
# Module name
#
MOD      =  libtclvalkey.so

#
# Objects to build.
#
MODOBJS     = src/library.o src/tclvalkeyCmdSub.o src/tclvalkeyCtx.o src/tclvalkeyReply.o

#MODLIBS  +=

CFLAGS += -DUSE_NAVISERVER

include  $(NAVISERVER)/include/Makefile.module