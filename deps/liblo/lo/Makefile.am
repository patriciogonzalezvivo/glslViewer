libloincludedir = $(includedir)/lo

libloinclude_HEADERS = lo_errors.h lo_lowlevel.h lo_throw.h lo_types.h \
	lo_osc_types.h lo_macros.h lo_cpp.h

nodist_libloinclude_HEADERS = lo.h lo_endian.h

if ENABLE_THREADS
libloinclude_HEADERS += lo_serverthread.h
endif
