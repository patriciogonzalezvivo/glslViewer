EXE = glslViewer

CXX=g++
SOURCES := $(wildcard src/*.cpp)
HEADERS := $(wildcard src/*.h)
OBJECTS := $(SOURCES:.cpp=.o)

UNAME := $(shell uname -s)

INCLUDES +=	-Isrc/

CFLAGS += -Wall -g -std=c++0x -fpermissive

LDFLAGS += -lfreeimage

ifeq ($(UNAME), Linux)
CFLAGS += -DPLATFORM_RPI -Wno-psabi

INCLUDES += -I$(SDKSTAGE)/opt/vc/include/ \
			-I$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads \
			-I$(SDKSTAGE)/opt/vc/include/interface/vmcs_host/linux

LDFLAGS += -L$(SDKSTAGE)/opt/vc/lib/ \
			-lGLESv2 -lEGL \
			-lbcm_host \
			-lfreeimage
endif

ifeq ($(UNAME),Darwin)
CFLAGS += -DPLATFORM_OSX -stdlib=libc++ $(shell pkg-config --cflags glfw3) -I/usr/local/include/
INCLUDES += -I//Library/Frameworks/GLUI.framework
LDFLAGS += -framework OpenGL $(shell pkg-config --libs glfw3) -L/usr/local/lib/
ARCH = -arch x86_64
endif

all: $(EXE)

%.o: %.cpp
	@echo $@
	$(CXX) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations

ifeq ($(UNAME), Linux)
$(EXE): $(OBJECTS) $(HEADERS)
	$(CXX) -o $@ -Wl,--whole-archive $(OBJECTS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic
endif

ifeq ($(UNAME),Darwin)
$(EXE): $(OBJECTS) $(HEADERS)
	$(CXX) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $@
endif

clean:
	@rm -rvf $(EXE) src/*.o

install:
	@cp $(EXE) /usr/local/bin

uninstall:
	@rm /usr/local/bin/$(EXE)
