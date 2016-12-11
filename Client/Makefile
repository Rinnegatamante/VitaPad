ifeq ($(OS),Windows_NT)
all: windows
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
all: linux
    endif
endif

windows:
	windres icon.rc -O coff -o icon.res
	g++ -static-libgcc -static-libstdc++ -o VitaPad main.cpp tinyxml2.cpp icon.res -static -lpthread -lws2_32 -fpermissive

linux:
	g++ -static-libgcc -static-libstdc++ -o VitaPad main.cpp tinyxml2.cpp -lX11 -fpermissive
