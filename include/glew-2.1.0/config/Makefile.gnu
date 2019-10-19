NAME = $(GLEW_NAME)
CC = cc
LD = cc
LDFLAGS.EXTRA = -L/usr/X11R6/lib
LDFLAGS.GL = -lGL -lX11
LDFLAGS.STATIC = -Wl,-Bstatic
LDFLAGS.DYNAMIC = -Wl,-Bdynamic
NAME = GLEW
WARN = -Wall -W
POPT = -O2
CFLAGS.EXTRA += -fPIC
BIN.SUFFIX =
LIB.SONAME    = lib$(NAME).so.$(SO_MAJOR)
LIB.DEVLNK    = lib$(NAME).so
LIB.SHARED    = lib$(NAME).so.$(SO_VERSION)
LIB.STATIC    = lib$(NAME).a
LDFLAGS.SO    = -shared -Wl,-soname=$(LIB.SONAME)
