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

ViGEmClient.o: ViGEmClient/src/ViGEmClient.cpp
	g++ -c \
		-static -static-libgcc -static-libstdc++ \
		-I ViGEmClient/include \
		-municode -DERROR_INVALID_DEVICE_OBJECT_PARAMETER=650L\
		ViGEmClient/src/ViGEmClient.cpp

tinyxml2.o: tinyxml2.cpp
	g++ -c \
		-static -static-libgcc -static-libstdc++ \
		tinyxml2.cpp

icon.res: icon.rc
	windres icon.rc -O coff -o icon.res

vJoyInterface.dll: $(VJOY_LIB_PATH)/vJoyInterface.dll
# use powershell to handle Linux-style file path, cmd will not work
	powershell -Command Copy-Item -Path $(VJOY_LIB_PATH)/vJoyInterface.dll -Destination vJoyInterface.dll

windows: ViGEmClient.o tinyxml2.o icon.res vJoyInterface.dll
	g++ -static -static-libgcc -static-libstdc++ \
		-I VJoySDK/inc -I ViGEmClient/include \
		-o VitaPad \
		main.cpp ViGEm.cpp ViGEmClient.o tinyxml2.o icon.res \
		 $(VJOY_LIB_PATH)/vJoyInterface.lib -lpthread -lws2_32 -lsetupapi

linux: tinyxml2.o
	g++ -static-libgcc -static-libstdc++ -o VitaPad main.cpp tinyxml2.o -lX11 -lXtst
