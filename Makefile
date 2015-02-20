EXE = piFrag

SOURCES := $(wildcard src/*.cpp)
HEADERS := $(wildcard src/*.h)
OBJECTS := $(SOURCES:.cpp=.o)

UNAME := $(shell uname -s)

INCLUDES +=	-Isrc/

CFLAGS += -Wall -g -std=c++0x -Wno-psabi -fpermissive

LDFLAGS += -lfreeimage

ifeq ($(UNAME), Linux)
CFLAGS += -DPLATFORM_RPI

INCLUDES += -I$(SDKSTAGE)/opt/vc/include/ \
			-I$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads \
			-I$(SDKSTAGE)/opt/vc/include/interface/vmcs_host/linux 

LDFLAGS += -L$(SDKSTAGE)/opt/vc/lib/ \
			-lGLESv2 -lEGL \
			-lbcm_host
endif

ifeq ($(UNAME_S),Darwin)
CFLAGS += -DPLATFORM_OSX
endif

all: $(EXE)

%.o: %.cpp
	@echo $@
	$(CXX) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations

$(EXE): $(OBJECTS) $(HEADERS)
	$(CXX) -o $@ -Wl,--whole-archive $(OBJECTS) $(LDFLAGS) -lfreeimage -Wl,--no-whole-archive -rdynamic

clean:
	@rm -rvf $(EXE) src/*.o

install:
	@cp $(EXE) /usr/local/bin

uninstall:
	@rm /usr/local/bin/$(EXE)
