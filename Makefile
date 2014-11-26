EXE = piFrag

SOURCES := $(wildcard src/*.cpp)
HEADERS := $(wildcard src/*.h)
OBJECTS := $(SOURCES:.cpp=.o)

INCLUDES+=-I$(SDKSTAGE)/opt/vc/include/ -I$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads -I$(SDKSTAGE)/opt/vc/include/interface/vmcs_host/linux -I./

CFLAGS+= -DSTANDALONE \
		-D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS \
		-DTARGET_POSIX \
		-D_LINUX -fPIC -DPIC -D_REENTRANT \
		-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
		-U_FORTIFY_SOURCE -Wall -g -DHAVE_LIBOPENMAX=2 \
		-DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX \
		-DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi -fpermissive

LDFLAGS+=-L$(SDKSTAGE)/opt/vc/lib/ \
		-lGLESv2 -lEGL \
		-lopenmaxil \
		-lbcm_host \
		-lvcos \
		-lvchiq_arm \
		-lpthread \
		-lrt

all: $(EXE)

%.o: %.cpp
	@echo $@
	$(CXX) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations

$(EXE): $(OBJECTS) $(HEADERS)
	$(CXX) -o $@ -Wl,--whole-archive $(OBJECTS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

clean:
	@rm -rvf $(EXE) src/*.o

install:
	@cp $(EXE) /usr/local/bin

uninstall:
	@rm /usr/local/bin/$(EXE)
