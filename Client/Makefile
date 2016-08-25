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
	g++ -o VitaPad main.cpp tinyxml2.cpp icon.res -lws2_32 -fpermissive

linux:
	g++ -o VitaPad main.cpp tinyxml2.cpp -lX11 -fpermissive

