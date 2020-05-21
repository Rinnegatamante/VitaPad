ifeq ($(OS),Windows_NT)

ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
	VJOY_LIB_PATH ?= VJoySDK/lib/amd64
endif

ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
	VJOY_LIB_PATH ?= VJoySDK/lib/amd64
else
	VJOY_LIB_PATH ?= VJoySDK/lib
endif

all: windows

else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
all: linux
    endif
endif

windows:
	windres icon.rc -O coff -o icon.res
	g++ -static-libgcc -static-libstdc++ -o VitaPad main.cpp tinyxml2.cpp icon.res -I VJoySDK/inc $(VJOY_LIB_PATH)/vJoyInterface.lib -static -lpthread -lws2_32 -fpermissive
# use powershell to handle Linux-style file path, cmd will not work
	powershell -Command Copy-Item -Path $(VJOY_LIB_PATH)/vJoyInterface.dll -Destination vJoyInterface.dll

linux:
	g++ -static-libgcc -static-libstdc++ -o VitaPad main.cpp tinyxml2.cpp -lX11 -lXtst -fpermissive
