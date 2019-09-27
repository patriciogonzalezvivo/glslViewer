EXE = ./bin/glslViewer

CXX = g++
SOURCES := $(wildcard include/*/*.cc) $(wildcard src/*.cpp) $(wildcard src/*/*.cpp)
HEADERS := $(wildcard include/*/*.h) $(wildcard src/*.h) $(wildcard src/*.h) $(wildcard src/*/*.h)
OBJECTS := $(SOURCES:.cpp=.o)

PLATFORM = $(shell uname)
DRIVERS ?= no_defined

ifeq ($(DRIVERS),no_defined)
	ifneq ("$(wildcard /opt/vc/include/bcm_host.h)","")
		ifeq ($(shell cat /proc/cpuinfo | grep 'Revision' | awk '{print $$3}' ), c03111)
			PLATFORM = RPI4
			DRIVERS = kms
		else
			PLATFORM = RPI
			DRIVERS = vc
		endif
	else
		PLATFORM = $(shell uname)
		DRIVERS = glfw
	endif
endif

$(info Platform ${PLATFORM})

INCLUDES +=	-Isrc/ -Iinclude/
CFLAGS += -Wall -O3 -std=c++11 -fpermissive

ifeq ($(DRIVERS),vc)
	CFLAGS += -DGLM_FORCE_CXX98 -DPLATFORM_RPI
	INCLUDES += -I/opt/vc/include/ \
				-I/opt/vc/include/interface/vcos/pthreads \
				-I/opt/vc/include/interface/vmcs_host/linux
	LDFLAGS +=  -L/opt/vc/lib/ \
				-lbcm_host \
				-lpthread

	ifeq ($(shell . /etc/os-release && echo $$PRETTY_NAME'),Raspbian GNU/Linux 8 (jessie))
	-	LDFLAGS += -lGLESv2 -lEGL
	else
		LDFLAGS += -lbrcmGLESv2 -lbrcmEGL
	endif

else ifeq ($(DRIVERS),kms)
	CFLAGS += -DGLM_FORCE_CXX98 -DPLATFORM_RPI4
	INCLUDES += -I/usr/include/libdrm \
				-I/usr/include/GLES2
	LDFLAGS +=  -lGLESv2 -lEGL \
				-ldrm -lgbm \
				-lpthread
	
else ifeq ($(PLATFORM),Linux)
CFLAGS += -DPLATFORM_LINUX $(shell pkg-config --cflags glfw3 glu gl)
LDFLAGS += $(shell pkg-config --libs glfw3 glu gl x11 xrandr xi xxf86vm xcursor xinerama xrender xext xdamage) -lpthread -ldl

else ifeq ($(PLATFORM),Darwin)
CXX = /usr/bin/clang++
ARCH = -arch x86_64
CFLAGS += $(ARCH) -DPLATFORM_OSX -stdlib=libc++ $(shell pkg-config --cflags glfw3)
INCLUDES += -I/System/Library/Frameworks/GLUI.framework
LDFLAGS += $(ARCH) -framework OpenGL -framework Cocoa -framework CoreVideo -framework IOKit $(shell pkg-config --libs glfw3)

endif

all: $(EXE)

%.o: %.cpp
	@echo $@
	$(CXX) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations

$(EXE): $(OBJECTS) $(HEADERS)
	$(CXX) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -rdynamic -o $@

clean:
	@rm -rvf $(EXE) src/*.o src/*/*.o include/*/*.o *.dSYM
	
install:
	@cp $(EXE) /usr/local/bin
	@cp bin/glslLoader /usr/local/bin

install_python:
	@python setup.py install
	@python3 setup.py install

clean_python:
	@rm -rvf build
	@rm -rvf dist]
	@rm -rvf python/glslviewer.egg-info

uninstall:
	@rm /usr/local/$(EXE)
	@rm /usr/local/bin/glslLoader
