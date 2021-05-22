EXE = ./bin/glslViewer

CXX = g++
SOURCES := 	$(wildcard include/*/*.cc) $(wildcard src/*.cpp) $(wildcard src/*/*.cpp) \
			$(wildcard include/oscpack/osc/*.cpp) $(wildcard include/oscpack/ip/posix/*.cpp) \
			$(wildcard include/phonedepth/extract_depthmap.cpp)

HEADERS := 	$(wildcard include/*/*.h) $(wildcard src/*.h) $(wildcard src/*.h) $(wildcard src/*/*.h) \
			$(wildcard include/oscpack/osc/*.h)   $(wildcard include/oscpack/ip/posix/*.h) \
			$(wildcard include/phonedepth/extract_depthmap.h)
			
OBJECTS := $(SOURCES:.cpp=.o)

PLATFORM = $(shell uname)
DRIVER ?= not_defined
LIBAV ?= not_defined

ifneq ("$(wildcard /opt/vc/include/bcm_host.h)","")
	PLATFORM = RPI
	RPI_REVISION = $(shell cat /proc/cpuinfo | grep 'Revision' | awk '{print $$3}')
	ifeq ($(RPI_REVISION),$(filter $(RPI_REVISION),a03111 b03111 b03112 b03114 c03111 c03112 c03114 d03114))
		ifeq ($(DRIVER),not_defined)
			DRIVER = fake_kms
		endif
	else
		ifeq ($(DRIVER),not_defined)
			DRIVER = legacy
		endif
	endif
else
	PLATFORM = $(shell uname)
	ifeq ($(DRIVER),not_defined)
		DRIVER = glfw
	endif
endif

$(info ${PLATFORM} platform with $(DRIVER) drivers)

INCLUDES +=	-Isrc/ -Iinclude/
CFLAGS += -Wall -O3 -std=c++11 -fpermissive

ifeq ($(PLATFORM), RPI)
	CFLAGS += -DPLATFORM_RPI -Wno-psabi 
	CFLAGS += -DUSE_VCHIQ_ARM -DOMX_SKIP64BIT
	CFLAGS += -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST
	CFLAGS += -DHAVE_LIBOPENMAX=2 -DOMX -DUSE_EXTERNAL_OMX 
	
	ifeq ($(DRIVER),legacy)
		CFLAGS += -DGLM_FORCE_CXX14 -DDRIVER_LEGACY
		ifeq ($(shell . /etc/os-release && echo $$PRETTY_NAME),Raspbian GNU/Linux 8 (jessie))
			LDFLAGS += -lGLESv2 -lEGL
		else
			LDFLAGS += -lbrcmGLESv2 -lbrcmEGL
		endif
		ILCLIENT_DIR = /opt/vc/src/hello_pi/libs/ilclient
		ILCLIENT = $(ILCLIENT_DIR)/libilclient.a
		INCLUDES += -I$(ILCLIENT_DIR)
		LDFLAGS += -L$(ILCLIENT_DIR) \
					-lilclient -ldl
		DEPS += $(ILCLIENT)

	else ifeq ($(DRIVER),fake_kms)
		CFLAGS += -DDRIVER_FAKE_KMS
		INCLUDES += -I/usr/include/libdrm \
					-I/usr/include/GLES2
		LDFLAGS +=  -ldrm -lgbm \
					-lGLESv2 -lEGL -ldl
	
	else ifeq ($(DRIVER),glfw)
		CFLAGS += -DDRIVER_GLFW $(shell pkg-config --cflags glfw3 glu gl)
		LDFLAGS += $(shell pkg-config --libs glfw3 glu gl x11 xrandr xi xxf86vm xcursor xinerama xrender xext xdamage) -lEGL -ldl
	endif

	INCLUDES += -I/opt/vc/include/ \
				-I/opt/vc/include/interface/vcos/pthreads \
				-I/opt/vc/include/interface/vmcs_host/linux
	LDFLAGS +=  -L/opt/vc/lib/ \
				-lmmal -lmmal_core -lmmal_util -lmmal_vc_client -lvcos \
				-lopenmaxil -lvchiq_arm \
				-lbcm_host \
				-lpthread -latomic

else ifeq ($(PLATFORM),Linux)
CFLAGS += -DPLATFORM_LINUX -DDRIVER_GLFW $(shell pkg-config --cflags glfw3 glu gl)
LDFLAGS += $(shell pkg-config --libs glfw3 glu gl x11 xrandr xi xxf86vm xcursor xinerama xrender xext xdamage) -lpthread -ldl 
LIBAV = true

else ifeq ($(PLATFORM),Darwin)
CXX = /usr/bin/clang++
ARCH = -arch $(shell uname -m)
CFLAGS += $(ARCH) -DPLATFORM_OSX -DDRIVER_GLFW -stdlib=libc++ $(shell pkg-config --cflags glfw3)
INCLUDES += -I/System/Library/Frameworks/GLUI.framework 
LDFLAGS += $(ARCH) -framework OpenGL -framework Cocoa -framework CoreVideo -framework IOKit $(shell pkg-config --libs glfw3)
LIBAV = true
endif

ifeq ($(LIBAV),true)
CFLAGS += -DSUPPORT_FOR_LIBAV $(shell pkg-config --cflags libavcodec libavformat libavfilter libavdevice libavutil libswscale)
LDFLAGS += $(shell pkg-config --libs libavcodec libavformat libavfilter libavdevice libavutil libswscale)
MINIAUDIO = include/miniaudio/miniaudio.h
HEADERS += ${MINIAUDIO}
DEPS += ${MINIAUDIO}
endif

all: $(EXE)

%.o: %.cpp
	@echo $@
	$(CXX) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations

$(EXE): $(DEPS) $(OBJECTS) $(HEADERS)
	$(CXX) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -rdynamic -o $@

${ILCLIENT}:
	$(MAKE) -C $(ILCLIENT_DIR)

${MINIAUDIO}:
	@echo "fetch miniaudio library"
	@git submodule update --init include/miniaudio/

clean:
	@rm -rvf $(EXE) src/*.o src/*/*.o include/*/*.o include/*/*/*.o include/*/*/*/*.o *.dSYM 
	
install:
	@rm -rf /usr/local/bin/$(EXE)
	@cp $(EXE) /usr/local/bin

uninstall:
	@rm /usr/local/$(EXE)
