AUTOMAKE_OPTIONS = foreign

SUBDIRS = .

bin_PROGRAMS = oscsend oscdump oscsendfile
oscsend_SOURCES = oscsend.c
oscsend_CFLAGS = $(LIBLO_CFLAGS) -I$(top_srcdir)
oscsend_LDADD = ../liblo.la
oscdump_SOURCES = oscdump.c
oscdump_CFLAGS = $(LIBLO_CFLAGS) -I$(top_srcdir)
oscdump_LDADD = ../liblo.la
oscsendfile_SOURCES = oscsendfile.c
oscsendfile_CFLAGS = $(LIBLO_CFLAGS) -I$(top_srcdir)
oscsendfile_LDADD = ../liblo.la
