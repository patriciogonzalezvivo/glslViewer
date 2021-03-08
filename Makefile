EXE = ./bin/glslViewer

CXX = g++
SOURCES := 	$(wildcard include/*/*.cc) $(wildcard src/*.cpp) $(wildcard src/*/*.cpp) \
			$(wildcard include/oscpack/osc/*.cpp) $(wildcard include/oscpack/ip/posix/*.cpp)

HEADERS := 	$(wildcard include/*/*.h) $(wildcard src/*.h) $(wildcard src/*.h) $(wildcard src/*/*.h) \
			$(wildcard include/oscpack/osc/*.h)   $(wildcard include/oscpack/ip/posix/*.h)
			# $(wildcard include/ilclient/*.h)
OBJECTS := $(SOURCES:.cpp=.o)

PLATFORM = $(shell uname)
DRIVER ?= not_defined
LIBAV ?= not_defined

ifneq ("$(wildcard /opt/vc/include/bcm_host.h)","")
	PLATFORM = RPI
	ifeq ($(shell cat /proc/cpuinfo | grep 'Revision' | awk '{print $$3}' ), c03111)
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
	# CFLAGS += -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -Wno-psabi
	
	ifeq ($(DRIVER),legacy)
		CFLAGS += -DGLM_FORCE_CXX14 -DDRIVER_LEGACY
		ifeq ($(shell . /etc/os-release && echo $$PRETTY_NAME),Raspbian GNU/Linux 8 (jessie))
			LDFLAGS += -lGLESv2 -lEGL
		else
			LDFLAGS += -lbrcmGLESv2 -lbrcmEGL
		endif

	else ifeq ($(DRIVER),fake_kms)
		CFLAGS += -DDRIVER_FAKE_KMS
		INCLUDES += -I/usr/include/libdrm \
					-I/usr/include/GLES2
		LDFLAGS +=  -ldrm -lgbm \
					-lGLESv2 -lEGL
	
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
				-lbcm_host\
				-lpthread

else ifeq ($(PLATFORM),Linux)
CFLAGS += -DPLATFORM_LINUX -DDRIVER_GLFW $(shell pkg-config --cflags glfw3 glu gl)
LDFLAGS += $(shell pkg-config --libs glfw3 glu gl x11 xrandr xi xxf86vm xcursor xinerama xrender xext xdamage) -lpthread -ldl 

else ifeq ($(PLATFORM),Darwin)
CXX = /usr/bin/clang++
ARCH = -arch x86_64
CFLAGS += $(ARCH) -DPLATFORM_OSX -DDRIVER_GLFW -stdlib=libc++ $(shell pkg-config --cflags glfw3)
INCLUDES += -I/System/Library/Frameworks/GLUI.framework
LDFLAGS += $(ARCH) -framework OpenGL -framework Cocoa -framework CoreVideo -framework IOKit $(shell pkg-config --libs glfw3)

endif

ifeq ($(LIBAV),true)
LDFLAGS += -lavcodec -lavformat -lavfilter -lavdevice -lavutil -lswscale
CFLAGS += -DSUPPORT_FOR_LIBAV 
endif

all: $(EXE)

%.o: %.cpp
	@echo $@
	$(CXX) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations

$(EXE): $(OBJECTS) $(HEADERS)
	$(CXX) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -rdynamic -o $@

clean:
	@rm -rvf $(EXE) src/*.o src/*/*.o include/*/*.o include/*/*/*.o include/*/*/*/*.o *.dSYM 
	
install:
	@cp bin/glslScreenSaver /usr/local/bin
	@cp bin/glslLoader /usr/local/bin
	@cp $(EXE) /usr/local/bin

install_python:
	@sudo python setup.py install
	@sudo python3 setup.py install

clean_python:
	@rm -rvf build
	@rm -rvf dist]
	@rm -rvf python/glslviewer.egg-info

uninstall:
	@rm /usr/local/$(EXE)
	@rm /usr/local/bin/glslLoader
