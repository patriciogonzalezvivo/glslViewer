EXE = glslViewer

CXX = g++
SOURCES := $(wildcard include/*/*.cc) $(wildcard src/*.cpp) $(wildcard src/*/*.cpp)
HEADERS := $(wildcard include/*/*.h) $(wildcard src/*.h) $(wildcard src/*.h) $(wildcard src/*/*.h)
OBJECTS := $(SOURCES:.cpp=.o)

UNAME := $(shell uname -s)
MACHINE := $(shell uname -m)
PLATFORM = RPI

INCLUDES +=	-Isrc/ -Iinclude/
CFLAGS += -Wall -O3 -std=c++0x -fpermissive

ifeq ($(UNAME), Darwin)
PLATFORM = OSX

else ifeq ($(MACHINE),i686)
PLATFORM = LINUX

else ifeq ($(MACHINE),x86_64)
PLATFORM = LINUX
endif

ifeq ($(PLATFORM),RPI)
CFLAGS += -DGLM_FORCE_CXX98 -DPLATFORM_RPI 

INCLUDES += -I$(SDKSTAGE)/opt/vc/include/ \
			-I$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads \
			-I$(SDKSTAGE)/opt/vc/include/interface/vmcs_host/linux

LDFLAGS += -L$(SDKSTAGE)/opt/vc/lib/ \
			-lGLESv2 -lEGL \
			-lbcm_host

else ifeq ($(PLATFORM),LINUX)
CFLAGS += -DPLATFORM_LINUX $(shell pkg-config --cflags glfw3 glu gl) 
LDFLAGS += $(shell pkg-config --libs glfw3 glu gl x11 xrandr xi xxf86vm xcursor xinerama xrender xext xdamage) -lpthread 

else ifeq ($(PLATFORM),OSX)
CXX = /usr/bin/clang++
ARCH = -arch x86_64
CFLAGS += $(ARCH) -DPLATFORM_OSX -stdlib=libc++ $(shell pkg-config --cflags glfw3)
INCLUDES += -I/Library/Frameworks/GLUI.framework
LDFLAGS += $(ARCH) -framework OpenGL -framework Cocoa -framework CoreVideo -framework IOKit $(shell pkg-config --libs glfw3)
endif

all: $(EXE)

%.o: %.cpp
	@echo $@
	$(CXX) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations

ifeq ($(PLATFORM), RPI)
$(EXE): $(OBJECTS) $(HEADERS)
	$(CXX) -o $@ -Wl,--whole-archive $(OBJECTS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

else ifeq ($(PLATFORM), LINUX)
$(EXE): $(OBJECTS) $(HEADERS)
	$(CXX) -o $@ -Wl,--whole-archive $(OBJECTS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

else ifeq ($(PLATFORM),OSX)
$(EXE): $(OBJECTS) $(HEADERS)
	$(CXX) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $@
endif

clean:
	@rm -rvf $(EXE) src/*.o src/*/*.o include/*/*.o *.dSYM

install:
	@cp $(EXE) /usr/local/bin

uninstall:
	@rm /usr/local/bin/$(EXE)
