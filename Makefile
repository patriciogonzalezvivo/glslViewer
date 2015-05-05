EXE = glslViewer

CXX = g++
SOURCES := $(wildcard src/*.cpp)
HEADERS := $(wildcard src/*.h)
OBJECTS := $(SOURCES:.cpp=.o)

UNAME := $(shell uname -s)
MACHINE := $(shell uname -m)
PLATFORM = RPI

INCLUDES +=	-Isrc/ -Iinclude/

CFLAGS += -Wall -g -std=c++0x -fpermissive

# LDFLAGS = 

ifeq ($(UNAME), Darwin)
PLATFORM = OSX

else ifeq ($(MACHINE),i686)
PLATFORM = LINUX

else ifeq ($(MACHINE),x86_64)
PLATFORM = LINUX
endif

ifeq ($(PLATFORM),RPI)
CFLAGS += -DPLATFORM_RPI -Wno-psabi

INCLUDES += -I$(SDKSTAGE)/opt/vc/include/ \
			-I$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads \
			-I$(SDKSTAGE)/opt/vc/include/interface/vmcs_host/linux

LDFLAGS += -L$(SDKSTAGE)/opt/vc/lib/ \
			-lGLESv2 -lEGL \
			-lbcm_host \

else ifeq ($(PLATFORM),LINUX)
CFLAGS += -DPLATFORM_LINUX $(shell pkg-config --cflags glfw3 glu gl) 
LDFLAGS += $(shell pkg-config --libs glfw3 glu gl x11 xrandr xi xxf86vm xcursor xinerama xrender xext xdamage) -lpthread 

else ifeq ($(PLATFORM),OSX)
CFLAGS += -DPLATFORM_OSX -stdlib=libc++ $(shell pkg-config --cflags glfw3) -I/usr/local/include/
INCLUDES += -I//Library/Frameworks/GLUI.framework
LDFLAGS += -framework OpenGL -framework Cocoa -framework CoreVideo -framework IOKit $(shell pkg-config --libs glfw3) -L/usr/local/lib/
ARCH = -arch x86_64
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
	@rm -rvf $(EXE) src/*.o

install:
	@cp $(EXE) /usr/local/bin

uninstall:
	@rm /usr/local/bin/$(EXE)
