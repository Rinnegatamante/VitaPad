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
	gcc -o VitaPad main.c icon.res -lws2_32

linux:
	gcc -o VitaPad main.c -lX11

