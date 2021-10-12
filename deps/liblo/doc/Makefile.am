## Process this file with automake to produce Makefile.in
MAINTAINERCLEANFILES=Makefile.in 

CLEANFILES=doxygen-build.stamp

DOX=reference.doxygen

EXTRA_DIST=

INSTIMAGES=html/doxygen.png

DOC_STAMPS=html-build.stamp

DOC_DIR=$(HTML_DIR)

all-local: doxygen-build.stamp

doxygen-build.stamp: $(DOX) ../lo/lo.h ../lo/lo_types.h ../lo/lo_lowlevel.h \
	../lo/lo_osc_types.h
	@echo '*** Running doxygen ***'
	doxygen $(DOX)
	touch doxygen-build.stamp

clean-local:
	rm -f *~ *.bak $(DOC_STAMPS) || true
	if test -d html; then rm -fr html; fi
	if test -d latex; then rm -fr latex; fi
	if test -d man; then rm -fr man; fi

distclean-local: clean
	rm -f *.stamp || true
	if test -d html; then rm -rf html; fi
