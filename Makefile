EXE = ./bin/glslViewer

CXX = g++
SOURCES := $(wildcard include/*/*.cc) $(wildcard src/*.cpp) $(wildcard src/*/*.cpp)
HEADERS := $(wildcard include/*/*.h) $(wildcard src/*.h) $(wildcard src/*.h) $(wildcard src/*/*.h)
OBJECTS := $(SOURCES:.cpp=.o)

PLATFORM = $(shell uname)
HARDWARE = "N/A"

ifneq ("$(wildcard /etc/os-release)","")
PLATFORM = $(shell . /etc/os-release && echo $$NAME )
endif

ifneq ("$(wildcard /opt/vc/include/bcm_host.h)","")
	PLATFORM = $(shell . /etc/os-release && echo $$PRETTY_NAME | awk '{print $$1}' )
	HARDWARE = $(shell cat /proc/cpuinfo | grep 'Revision' | awk '{print $$3}' )
endif

$(info Platform ${PLATFORM})

INCLUDES +=	-Isrc/ -Iinclude/
CFLAGS += -Wall -O3 -std=c++11 -fpermissive

ifeq ($(PLATFORM),Raspbian)
	CFLAGS += -DGLM_FORCE_CXX98

ifeq ($(HARDWARE),c03111)
	CFLAGS += -DPLATFORM_RPI4
	INCLUDES += -I/opt/vc/include/ \
				-I/usr/include/libdrm \
				-I/usr/include/GLES2
	LDFLAGS += -L/opt/vc/lib/ \
				-lGLESv2 -lEGL \
				-ldrm -lgbm \
				-lpthread
else
	CFLAGS += -DPLATFORM_RPI
	INCLUDES += -I/opt/vc/include/ \
				-I/opt/vc/include/interface/vcos/pthreads \
				-I/opt/vc/include/interface/vmcs_host/linux
	LDFLAGS += -lbrcmGLESv2 -lbrcmEGL \
				-lbcm_host \
				-lpthread
endif
	

else ifeq ($(shell uname),Linux)
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
